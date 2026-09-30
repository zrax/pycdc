#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <unordered_set>
#include <set>
#include <map>
#include "ASTree.h"
#include "FastStack.h"
#include "pyc_numeric.h"
#include "bytecode.h"

/* Construct a name node for compiler-synthesized identifiers (e.g.
   AssertionError, __exit__) that never appear in the constant table. */
static PycRef<ASTNode> make_name(const char* name)
{
    PycString* str = new PycString();
    str->setValue(name);
    return new ASTName(str);
}

// This must be a triple quote (''' or """), to handle interpolated string literals containing the opposite quote style.
// E.g. f'''{"interpolated "123' literal"}'''    -> valid.
// E.g. f"""{"interpolated "123' literal"}"""    -> valid.
// E.g. f'{"interpolated "123' literal"}'        -> invalid, unescaped quotes in literal.
// E.g. f'{"interpolated \"123\' literal"}'      -> invalid, f-string expression does not allow backslash.
// NOTE: Nested f-strings not supported.
#define F_STRING_QUOTE "'''"

static void append_to_chain_store(const PycRef<ASTNode>& chainStore,
        PycRef<ASTNode> item, FastStack& stack, const PycRef<ASTBlock>& curblock);

/* Use this to determine if an error occurred (and therefore, if we should
 * avoid cleaning the output tree) */
static bool cleanBuild;

/* Use this to prevent printing return keywords and newlines in lambdas. */
static bool inLambda = false;

/* Use this to keep track of whether we need to print out any docstring and
 * the list of global variables that we are using (such as inside a function). */
static bool printDocstringAndGlobals = false;

/* Use this to keep track of whether we need to print a class or module docstring */
static bool printClassDocstring = true;

// shortcut for all top/pop calls
static PycRef<ASTNode> StackPopTop(FastStack& stack)
{
    const auto node(stack.top());
    stack.pop();
    return node;
}

/* compiler generates very, VERY similar byte code for if/else statement block and if-expression
 *  statement
 *      if a: b = 1
 *      else: b = 2
 *  expression:
 *      b = 1 if a else 2
 *  (see for instance https://stackoverflow.com/a/52202007)
 *  here, try to guess if just finished else statement is part of if-expression (ternary operator)
 *  if it is, remove statements from the block and put a ternary node on top of stack
 */
static void CheckIfExpr(FastStack& stack, PycRef<ASTBlock> curblock)
{
    if (stack.empty())
        return;
    if (curblock->nodes().size() < 2)
        return;
    auto rit = curblock->nodes().crbegin();
    // the last is "else" block, the one before should be "if" (could be "for", ...)
    if ((*rit)->type() != ASTNode::NODE_BLOCK ||
        (*rit).cast<ASTBlock>()->blktype() != ASTBlock::BLK_ELSE)
        return;
    ++rit;
    if ((*rit)->type() != ASTNode::NODE_BLOCK ||
        (*rit).cast<ASTBlock>()->blktype() != ASTBlock::BLK_IF)
        return;
    auto else_expr = StackPopTop(stack);
    curblock->removeLast();
    auto if_block = curblock->nodes().back();
    auto if_expr = StackPopTop(stack);
    curblock->removeLast();
    stack.push(new ASTTernary(std::move(if_block), std::move(if_expr), std::move(else_expr)));
}

/* Python 3.12+ inlines list/set/dict comprehensions into the enclosing
   code object (PEP 709).  When such a comprehension needs exception
   cleanup, the compiler emits a dead handler region that restores the
   comprehension's saved locals and re-raises.  Such regions only ever
   contain this restricted opcode set. */
static bool isCompRestoreOp(int opcode)
{
    switch (opcode) {
        case Pyc::SWAP_A:
        case Pyc::POP_TOP:
        case Pyc::POP_ITER:
        case Pyc::END_FOR:
        case Pyc::RERAISE_A:
        case Pyc::STORE_FAST_A:
        case Pyc::STORE_FAST_STORE_FAST_A:
        case Pyc::STORE_FAST_LOAD_FAST_A:
        case Pyc::CACHE:
        case Pyc::JUMP_BACKWARD_NO_INTERRUPT_A:
            return true;
        default:
            return false;
    }
}

/* Python 3.12-3.14: a try/except/finally compiles the finally into a
   propagation region guarded by a self-chained exception-table entry
   (target == end) whose region starts with PUSH_EXC_INFO.  Locate the one
   belonging to the try whose handler begins at `handler` so its container
   can be flagged as having a finally (the container is created long before
   the region is reached, and EXCEPT-arm closing must know not to pop it).
   `source` is passed by value: its cursor sits at `pos`, and candidate
   regions start at or beyond the handler, i.e. ahead of the cursor. */
static int find_finally_start(const std::vector<PycExceptionTableEntry>& entries,
                              PycBuffer source, PycModule* mod,
                              int handler, int pos)
{
    int opcode = 0, oper = 0, apos = pos;
    for (const auto& e : entries) {
        if (e.target != e.end_offset || e.start_offset < handler)
            continue;
        while (apos < e.start_offset && !source.atEof())
            bc_next(source, mod, opcode, oper, apos);
        if (apos != e.start_offset)
            continue;   /* not an instruction boundary */
        bc_next(source, mod, opcode, oper, apos);
        if (opcode != Pyc::PUSH_EXC_INFO)
            continue;
        /* A genuine finally body follows the PUSH_EXC_INFO; a compiler
           artifact epilogue (dead except-var cleanup tail) is
           PUSH_EXC_INFO immediately followed by RERAISE 0. */
        int nstart = apos;
        int nop = 0, noper = 0;
        while (nstart < e.end_offset && !source.atEof()) {
            bc_next(source, mod, nop, noper, nstart);
            if (nop == Pyc::CACHE)
                continue;
            if (nop == Pyc::RERAISE_A)
                break;   /* artifact: no finally body */
            return e.start_offset;
        }
    }
    return 0;
}

PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod)
{
    PycBuffer source(code->code()->value(), code->code()->length());

    FastStack stack((mod->majorVer() == 1) ? 20 : code->stackSize());
    stackhist_t stack_hist;

    std::stack<PycRef<ASTBlock> > blocks;
    PycRef<ASTBlock> defblock = new ASTBlock(ASTBlock::BLK_MAIN);
    defblock->init();
    PycRef<ASTBlock> curblock = defblock;
    blocks.push(defblock);

    int opcode = Pyc::NOP, operand = 0;
    /* Opcode immediately preceding the one being processed; used to
       recognize the `COPY 1; STORE` idiom of named expressions. */
    int last_op = Pyc::NOP, last_operand = 0;
    int curpos = 0;
    int pos = 0;
    int unpack = 0;
    /* Inline-comprehension state (PEP 709, Python 3.12+):
     * comp_saved    - number of LOAD_FAST_AND_CLEAR locals whose old values
     *                 still sit below the active comprehension accumulator
     *                 awaiting the epilogue restores
     * comp_restoring- set once the epilogue is reached (END_FOR/POP_ITER);
     *                 the following STORE_FASTs silently restore the saved
     *                 locals instead of emitting assignments
     * comp_kind     - accumulator kind of the most recent prologue */
    int comp_saved = 0;
    bool comp_restoring = false;
    ASTComprehension::CompKind comp_kind = ASTComprehension::COMP_LIST;
    bool else_pop = false;
    bool need_try = false;
    bool variable_annotations = false;
    std::vector<PycExceptionTableEntry> exception_entries;
    size_t next_exception_entry = 0;
    /* In-loop try/except: when the handler region lives past the rest of
       the loop body, the container is appended into its parent (mounted)
       when the protected region ends, and is re-activated on the block
       stack once the handler offset is reached, so the except clauses are
       decoded into it.  Nested in-loop tries keep a stack of pending
       containers: the inner handler offset is always reached first. */
    std::vector<PycRef<ASTContainerBlock>> pending_conts;

    /* Python 3.8+ dropped SETUP_LOOP, and `while True:` loops carry no
       conditional jump at their head, so the structural while-recognition
       (hooked on POP_JUMP) cannot see them.  Pre-scan the bytecode: every
       JUMP_BACKWARD target that is neither a FOR_ITER head nor the target
       of any conditional jump marks the head of an unconditional loop. */
    std::set<int> uncond_loop_heads;
    /* exit offset -> head of a `while True:` closed via a conditional
       exit instead of a trailing JUMP_BACKWARD (continue-style loop). */
    std::map<int, int> uncond_loop_ends;
    std::vector<std::pair<int, int>> back_edges;
    /* Set when a `while True:` loop's structural exit is being decoded as
       the else arm of its trailing if (Python 3.14 tail-optimized
       if/else); the loop block then closes after that arm finishes. */
    bool while_else_exit = false;
    if (mod->verCompare(3, 11) >= 0) {
        exception_entries = code->exceptionTableEntries();
    }
    if (mod->verCompare(3, 8) >= 0) {
        PycBuffer probe = source;
        int ppos = 0, pop_ = 0, poper_ = 0;
        std::set<int> cond_heads, iter_heads, back_heads, jump_positions, back_srcs;
        while (!probe.atEof()) {
            int start = ppos;
            bc_next(probe, mod, pop_, poper_, ppos);
            switch (pop_) {
            case Pyc::FOR_ITER_A:
                /* The loop head is the FOR_ITER instruction itself. */
                iter_heads.insert(start);
                break;
            case Pyc::POP_JUMP_IF_FALSE_A:
            case Pyc::POP_JUMP_IF_TRUE_A:
            case Pyc::POP_JUMP_IF_NONE_A:
            case Pyc::POP_JUMP_IF_NOT_NONE_A:
            case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
            case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
            case Pyc::POP_JUMP_FORWARD_IF_NOT_NONE_A:
            case Pyc::POP_JUMP_FORWARD_IF_NONE_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_FALSE_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_TRUE_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A: {
                int target = poper_ * 2 + ppos;
                if (mod->verCompare(3, 13) >= 0)
                    target += 2;   /* inline cache slot */
                cond_heads.insert(target);
                jump_positions.insert(start);
                break;
            }
            case Pyc::JUMP_BACKWARD_NO_INTERRUPT_A:
                /* The SEND-resume edge of await/async-for machinery
                   (SEND..YIELD..RESUME..JUMP_BACKWARD_NO_INTERRUPT) is
                   not a structural loop: the main decode treats it as a
                   no-op, so it must not seed a back edge either. */
                break;
            case Pyc::JUMP_BACKWARD_A: {
                int target = ppos - poper_ * 2;
                /* Verified against CPython's dis output: 3.12 and older
                   land on start+2-oparg*2 (`ppos` already sits two bytes
                   past the opcode body, bc_next does not skip caches).
                   In 3.13/3.14 JUMP_BACKWARD carries one inline cache
                   slot, shifting its landing 2 bytes further
                   (start+4-oparg*2), while JUMP_BACKWARD_NO_INTERRUPT
                   (the SEND-resume edge of await machinery) has none. */
                if (mod->verCompare(3, 13) >= 0)
                    target += 2;
                if (target >= 0) {
                    back_heads.insert(target);
                    back_edges.emplace_back(target, start);
                    back_srcs.insert(start);
                }
                jump_positions.insert(start);
                break;
            }
            default:
                break;
            }
        }
for (int t : back_heads) {
            if (cond_heads.find(t) != cond_heads.end()
                    || iter_heads.find(t) != iter_heads.end()
                    || jump_positions.find(t) != jump_positions.end())
                continue;
            /* A `while True:` head is the first statement of its body; a
               backward edge that lands on another jump instruction (e.g.
               the re-evaluated condition of a 3.13 while loop) is not an
               unconditional head.  Likewise, an edge from an exception
               epilogue that jumps back into the *middle* of another
               loop's body (a shared loop tail) must not open a phantom
               loop: a genuinely nested `while True:` head lies strictly
               inside the enclosing loop together with all of its back
               edges, while a mid-body target has a back-edge source that
               lies past the enclosing loop's end. */
            bool mid_body = false;
            for (const auto& e : back_edges) {
                if (e.first >= t || t >= e.second)
                    continue;   /* not an enclosing span */
                for (const auto& e2 : back_edges) {
                    if (e2.first == t && e2.second > e.second) {
                        mid_body = true;
                        break;
                    }
                }
                if (mid_body)
                    break;
            }
            /* A conditional `while cond:` head starts with the condition
               evaluation, and one of the conditional jumps inside the
               loop span exits past the back edge (e.g. Python 3.14 jumps
               straight back to the condition evaluation).  A break-guard
               inside a `while True:` jumps to the break sequence, which
               still lies before the back edge, so genuine unconditional
               heads survive this test. */
            if (!mid_body) {
                int jbsrc = -1;
                for (const auto& e : back_edges) {
                    if (e.first == t && e.second > jbsrc)
                        jbsrc = e.second;
                }
                if (jbsrc > t) {
                    /* Rotated `while cond:` head: when a loop is re-
                       entered from exception-handler land (a `continue`
                       inside except/finally), Python 3.13 emits the
                       handler's back edge to the condition evaluation
                       itself.  The head's own conditional exit then lies
                       INSIDE the span - past the loop body's complete
                       back edge, but before the handler's.  A genuine
                       `while True:` never exits before its last back
                       edge (a break always lands past every continue),
                       and an internal if/else target never skips a
                       complete nested cycle, so this shape uniquely
                       marks a conditional head.  Only a condition
                       evaluation directly at the head counts: past a
                       nested back edge, any conditional jump belongs to
                       an inner structure. */
                    PycBuffer probe3 = source;
                    int p3 = 0;
                    while (p3 < t && !probe3.atEof()) {
                        int d1 = 0, d2 = 0;
                        bc_next(probe3, mod, d1, d2, p3);
                    }
                    int tt = -1, fs = -1;
                    for (const auto& e : back_edges) {
                        if (e.second > t && (fs < 0 || e.second < fs))
                            fs = e.second;
                    }
                    while (p3 < jbsrc && !probe3.atEof()) {
                        int st3 = p3, o3 = 0, o3p = 0;
                        bc_next(probe3, mod, o3, o3p, p3);
                        if ((st3 != t && back_heads.count(st3))
                                || back_srcs.count(st3))
                            break;
                        switch (o3) {
                        case Pyc::POP_JUMP_IF_FALSE_A:
                        case Pyc::POP_JUMP_IF_TRUE_A:
                        case Pyc::POP_JUMP_IF_NONE_A:
                        case Pyc::POP_JUMP_IF_NOT_NONE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_NONE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_NOT_NONE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_FALSE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_TRUE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A:
                        case Pyc::JUMP_IF_FALSE_OR_POP_A:
                        case Pyc::JUMP_IF_TRUE_OR_POP_A:
                            tt = o3p * 2 + p3;
                            if (mod->verCompare(3, 13) >= 0)
                                tt += 2;   /* inline cache slot */
                            break;
                        default:
                            break;
                        }
                        if (tt >= 0)
                            break;
                    }
                    if (tt >= 0 && tt <= jbsrc && fs > t && tt > fs) {
                        /* ... but only when the enclosing cycle is a
                           protected region whose handler re-enters the
                           loop (a `continue` inside except/finally): a
                           rotated INNER while loop inside a genuine
                           `while True:` produces the same local shape
                           without any exception entry. */
                        bool protected_cycle = false;
                        for (const auto& ee : exception_entries) {
                            if (ee.start_offset < fs
                                    && ee.end_offset > t
                                    && ee.target > fs) {
                                protected_cycle = true;
                                break;
                            }
                        }
                        if (protected_cycle) {
                            for (const auto& e : back_edges) {
                                if (e.first != t && e.first > t && e.first < tt) {
                                    mid_body = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (!mid_body) {
                    /* bc_next reads from the buffer cursor, so walk it
                       forward from 0 in lockstep with the offset first. */
                    PycBuffer probe2 = source;
                    int p2 = 0;
                    while (p2 < t && !probe2.atEof()) {
                        int d1 = 0, d2 = 0;
                        bc_next(probe2, mod, d1, d2, p2);
                    }
                    while (p2 < jbsrc && !probe2.atEof()) {
                        int st = p2, o2 = 0, o2p = 0;
                        bc_next(probe2, mod, o2, o2p, p2);
                        switch (o2) {
                        case Pyc::POP_JUMP_IF_FALSE_A:
                        case Pyc::POP_JUMP_IF_TRUE_A:
                        case Pyc::POP_JUMP_IF_NONE_A:
                        case Pyc::POP_JUMP_IF_NOT_NONE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_NONE_A:
                        case Pyc::POP_JUMP_FORWARD_IF_NOT_NONE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_FALSE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_TRUE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A:
                        case Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A:
                        case Pyc::JUMP_IF_FALSE_OR_POP_A:
                        case Pyc::JUMP_IF_TRUE_OR_POP_A: {
                            int tgt = o2p * 2 + p2;
                            if (mod->verCompare(3, 13) >= 0)
                                tgt += 2;   /* inline cache slot */
                            if (tgt > jbsrc) {
                                /* Conditional exit found.  A genuine
                                   `while True:` whose body contains an
                                   inner loop (e.g. ...; if c: continue;
                                   return) also exits past the back edge,
                                   but then another back edge targets the
                                   middle of the span.  Only a pure
                                   condition evaluation (no nested back
                                   edges before the exit jump) marks a
                                   conditional while head. */
                                bool nested_edge = false;
                                for (int bt : back_heads) {
                                    if (bt > t && bt < st) {
                                        nested_edge = true;
                                        break;
                                    }
                                }
                                if (!nested_edge) {
                                    mid_body = true;
                                } else {
                                    /* Genuine `while True:` exiting via
                                       the conditional: record the exit
                                       so the block can be closed there. */
                                    uncond_loop_ends[tgt] = t;
                                }
                            }
                            break;
                        }
                        default:
                            break;
                        }
                        if (mid_body)
                            break;
                    }
                }
            }
            if (!mid_body)
                uncond_loop_heads.insert(t);
        }
    }


        /* Python 3.11+: after a `with` statement body, the compiler emits an
       inline epilogue (the __exit__ call) followed by dead exception-handler
       tail code (PUSH_EXC_INFO, WITH_EXCEPT_START, RERAISE, ...).  Handlers
       that restore a non-empty stack (or push the lasti) never correspond
       to source-level `try:` blocks; their regions are skipped as dead
       code so they do not pollute the decompiled output. */
    auto isWithCleanupOp = [](int op) -> bool {
        switch (op) {
        case Pyc::POP_EXCEPT:
        case Pyc::POP_TOP:
        case Pyc::COPY_A:
        case Pyc::RERAISE_A:
        case Pyc::LOAD_CONST_A:
        case Pyc::RETURN_VALUE:
        case Pyc::RETURN_CONST_A:
        case Pyc::JUMP_FORWARD_A:
        case Pyc::JUMP_BACKWARD_A:
        case Pyc::JUMP_BACKWARD_NO_INTERRUPT_A:
        case Pyc::NOT_TAKEN:
        case Pyc::PUSH_EXC_INFO:
        case Pyc::WITH_EXCEPT_START:
        case Pyc::TO_BOOL:
        case Pyc::LOAD_FAST_CHECK_A:
        case Pyc::LOAD_FAST_BORROW_A:
        case Pyc::LOAD_SPECIAL_A:
        case Pyc::SWAP_A:
        case Pyc::POP_ITER:
        case Pyc::CACHE:
            return true;
        default:
            return false;
        }
    };
    auto isPathTerminator = [](int op) -> bool {
        return op == Pyc::RERAISE_A || op == Pyc::RETURN_VALUE
                || op == Pyc::RETURN_CONST_A || op == Pyc::JUMP_FORWARD_A;
    };
    /* Skip the exception-handler region that starts at entry_start, plus
       any chained handler regions (regions targeted by another region's
       target), plus the trailing cleanup instructions. */
    auto skipExceptionRegion = [&](int entry_start) {
        int stop = entry_start;
        int target = -1;
        bool extended = true;
        while (extended) {
            extended = false;
            for (const auto& ent : exception_entries) {
                if (ent.start_offset < entry_start)
                    continue;
                /* Grow the window over every region it already touches,
                   and over regions chained through a target link. */
                if (((ent.start_offset <= stop && ent.end_offset > stop)
                            || (target >= 0 && ent.start_offset == target))
                        && ent.end_offset > stop) {
                    stop = ent.end_offset;
                    target = ent.target;
                    extended = true;
                }
            }
        }

        /* Skip instructions up to the end of the handler region. */
        while (pos < stop && !source.atEof()) {
            bc_next(source, mod, opcode, operand, pos);
            while (next_exception_entry < exception_entries.size()
                    && exception_entries[next_exception_entry].start_offset < pos) {
                next_exception_entry++;
            }
        }

        /* Consume the remaining exception-cleanup instructions (handler
           success paths and final RERAISE tails).  Boundaries are detected
           by peeking at the raw opcode byte so no real instruction is ever
           consumed past the dead tail.  Consumption stops at the first
           path terminator so following real code is preserved. */
        bool terminated = false;
        while (!source.atEof() && !terminated) {
            PycBuffer peek = source;
            int nextop = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), peek.getByte());
            if (!isWithCleanupOp(nextop)) {
                /* When __exit__ suppresses an exception, control resumes
                   after the `with`, so the compiler may duplicate the tail
                   of the surrounding function (e.g. a second `return`)
                   after the handler cleanup.  That duplicate is dead at
                   the source level (recompiling the `with` regenerates
                   it).  Consume the rest of the code object only when it
                   matches the shape "expression run, one RETURN, cleanup
                   run": a value-producing sequence with no jumps, a
                   single return terminator, then nothing but with-cleanup
                   opcodes.  Anything else is treated as live code. */
                PycBuffer scan = source;
                int scanpos = pos;
                int stage = 0; /* 0: expression, 1: seen return, 2: cleanup */
                bool matches = true;
                while (!scan.atEof()) {
                    int sop, sopc;
                    bc_next(scan, mod, sop, sopc, scanpos);
                    if (stage == 0) {
                        if (sop == Pyc::RETURN_VALUE || sop == Pyc::RETURN_CONST_A)
                            stage = 1;
                        else if (sop == Pyc::JUMP_FORWARD_A || sop == Pyc::JUMP_BACKWARD_A
                                || sop == Pyc::JUMP_BACKWARD_NO_INTERRUPT_A
                                || sop == Pyc::RERAISE_A) {
                            matches = false;
                            break;
                        }
                    } else if (stage == 1) {
                        if (!isWithCleanupOp(sop)) {
                            matches = false;
                            break;
                        }
                        if (sop != Pyc::RERAISE_A)
                            stage = 2;
                    } else {
                        if (!isWithCleanupOp(sop) && sop != Pyc::RERAISE_A) {
                            matches = false;
                            break;
                        }
                    }
                }
                if (matches && stage >= 1) {
                    while (pos < scanpos)
                        bc_next(source, mod, opcode, operand, pos);
                }
                break;
            }
            terminated = isPathTerminator(nextop);
            bc_next(source, mod, opcode, operand, pos);
        }
    };

    while (!source.atEof()) {
#if defined(BLOCK_DEBUG) || defined(STACK_DEBUG)
        fprintf(stderr, "%-7d", pos);
    #ifdef STACK_DEBUG
        fprintf(stderr, "%-5d", (unsigned int)stack_hist.size() + 1);
    #endif
    #ifdef BLOCK_DEBUG
        for (unsigned int i = 0; i < blocks.size(); i++)
            fprintf(stderr, "    ");
        fprintf(stderr, "%s (%d)", curblock->type_str(), curblock->end());
    #endif
        fprintf(stderr, "\n");
#endif

        while (next_exception_entry < exception_entries.size()
                && exception_entries[next_exception_entry].start_offset < pos) {
            next_exception_entry++;
        }

        if (next_exception_entry < exception_entries.size()) {
            const auto& entry = exception_entries[next_exception_entry];
            if (entry.start_offset == pos
                    && entry.stack_depth == 0
                    && !entry.push_lasti) {
                /* Python 3.11+ re-raise propagation regions (e.g.
                   `PUSH_EXC_INFO; RERAISE` in a finally chain, or
                   `COPY n; POP_EXCEPT; RERAISE`) are synthesized tails of
                   an except/finally construct, not source-level `try`
                   blocks.  Detect them by scanning for a RERAISE that is
                   reached before any real handler code. */
                bool is_reraise_region = false;
                {
                    PycBuffer scan = source;
                    int scanpos = pos;
                    int sop, sopc;
                    while (!scan.atEof() && scanpos < entry.end_offset) {
                        bc_next(scan, mod, sop, sopc, scanpos);
                        if (sop == Pyc::RERAISE || sop == Pyc::RERAISE_A) {
                            is_reraise_region = true;
                            break;
                        }
                        if (!(sop == Pyc::PUSH_EXC_INFO || sop == Pyc::NOP
                                || sop == Pyc::CACHE || sop == Pyc::COPY_A
                                || sop == Pyc::POP_EXCEPT
                                || sop == Pyc::LOAD_CONST_A))
                            break;
                    }
                }
                if (is_reraise_region) {
                    next_exception_entry++;
                    while (pos < entry.end_offset && !source.atEof()) {
                        bc_next(source, mod, opcode, operand, pos);
                        while (next_exception_entry < exception_entries.size()
                                && exception_entries[next_exception_entry].start_offset <= pos) {
                            next_exception_entry++;
                        }
                    }
                    continue;
                }
                /* A `continue` (or `break`) out of an except arm inside
                   a loop emits a tiny exception-table region covering
                   just the arm-closing POP_EXCEPT, targeting the finally
                   propagation handler.  It is compiler cleanup, not a
                   source-level try; skip it so the arm stays open and
                   the finally body decoded after it stays reachable. */
                if (curblock->blktype() == ASTBlock::BLK_EXCEPT
                        && entry.end_offset == entry.start_offset + 2) {
                    int aop = 0, ac = 0, apos = pos;
                    PycBuffer asc = source;
                    while (apos < entry.start_offset && !asc.atEof())
                        bc_next(asc, mod, aop, ac, apos);
                    if (apos == entry.start_offset) {
                        bc_next(asc, mod, aop, ac, apos);
                        if (aop != Pyc::POP_EXCEPT)
                            apos = entry.start_offset - 1;
                    }
                    if (aop == Pyc::POP_EXCEPT && apos == entry.end_offset) {
                        next_exception_entry++;
                        continue;
                    }
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    auto cont_ = curblock.cast<ASTContainerBlock>();
                    cont_->setExcept(entry.target);
                    if (!cont_->hasFinally()) {
                        int fin = find_finally_start(exception_entries, source, mod,
                                                     entry.target, pos);
                        if (fin)
                            cont_->setFinally(fin);
                    }
                } else {
                    int fin = find_finally_start(exception_entries, source, mod,
                                                 entry.target, pos);
                    PycRef<ASTBlock> next = new ASTContainerBlock(fin, entry.target);
                    blocks.push(next.cast<ASTBlock>());
                    curblock = blocks.top();
                }

                /* A try inside a while loop: the handler region is
                   emitted after the loop tail, so a back edge jumping to
                   a target at or before the protected region start (the
                   loop head) lies in the gap between region end and
                   handler.  End the try at the region end so the tail
                   decodes into the loop; the deferred-container logic
                   re-activates the handler when decoding reaches it. */
                int try_end = entry.target;
                if (entry.target > entry.end_offset) {
                    bool defer = false;
                    for (const auto& e : back_edges) {
                        if (e.second >= entry.end_offset && e.second < entry.target
                                && e.first <= entry.start_offset) {
                            defer = true;
                            break;
                        }
                    }
                    if (defer) {
                        /* Only defer when the handler region is a real
                           except arm (PUSH_EXC_INFO then CHECK_EXC_MATCH);
                           synthesized finally/reraise targets keep the
                           full extent to the handler. */
                        bool real_handler = false;
                        PycBuffer scan = source;
                        int scanpos = pos;
                        while (scanpos < entry.target && !scan.atEof()) {
                            int so, sc;
                            bc_next(scan, mod, so, sc, scanpos);
                        }
                        int sop, sopc, steps = 0;
                        while (!scan.atEof() && steps < 24) {
                            bc_next(scan, mod, sop, sopc, scanpos);
                            steps++;
                            if (sop == Pyc::CHECK_EXC_MATCH
                                    || sop == Pyc::CHECK_EG_MATCH) {
                                real_handler = true;
                                break;
                            }
                            if (sop == Pyc::RERAISE || sop == Pyc::RERAISE_A
                                    || sop == Pyc::WITH_EXCEPT_START)
                                break;
                        }
                        if (real_handler)
                            try_end = entry.end_offset;
                    }
                }

                stack_hist.push(stack);
                PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY, try_end, true);
                blocks.push(tryblock.cast<ASTBlock>());
                curblock = blocks.top();
                next_exception_entry++;
            } else if (entry.start_offset == pos
                    && (entry.stack_depth != 0 || entry.push_lasti)) {
                /* Python 3.11+: exception entries that restore a non-empty
                   stack or push the lasti never begin a source-level `try`
                   block.  With-statement epilogues (regions containing
                   WITH_EXCEPT_START) are unreachable inline cleanup and are
                   skipped, together with any chained handler regions.
                   Real `except` handler regions (CHECK_EXC_MATCH ...) are
                   reachable through the try block's handler logic and are
                   left to the normal flow. */
                bool is_with_epilogue = false;
                {
                    PycBuffer scan = source;
                    int scanpos = pos;
                    while (!scan.atEof() && scanpos < entry.end_offset) {
                        int sop, sopc;
                        bc_next(scan, mod, sop, sopc, scanpos);
                        if (sop == Pyc::WITH_EXCEPT_START) {
                            is_with_epilogue = true;
                            break;
                        }
                    }
                }
                if (is_with_epilogue) {
                    int entry_start = entry.start_offset;
                    next_exception_entry++;
                    skipExceptionRegion(entry_start);
                    continue;
                }

                /* A self-chained entry (target == region end) that is
                   reached while its container is the current block is
                   the finally-propagation handler: the finally body
                   re-run with the exception alive, ending in RERAISE.
                   Decode it as the container's FINALLY arm. */
                if (entry.target == entry.end_offset
                        && curblock->blktype() == ASTBlock::BLK_CONTAINER
                        && curblock.cast<ASTContainerBlock>()->hasFinally()
                        && curblock.cast<ASTContainerBlock>()->except() < entry.start_offset) {
                    stack_hist.push(stack);
                    PycRef<ASTBlock> final = new ASTBlock(ASTBlock::BLK_FINALLY, 0, true);
                    blocks.push(final.cast<ASTBlock>());
                    curblock = blocks.top();
                    next_exception_entry++;
                } else if (!entry.push_lasti && entry.stack_depth > 0) {
                    bool real_handler = false;
                    {
                        /* Scan the handler at the entry target: a real
                           except handler starts with PUSH_EXC_INFO and
                           reaches CHECK_EXC_MATCH before any synthesized
                           RERAISE / WITH_EXCEPT_START. */
                        PycBuffer scan = source;
                        int scanpos = pos;
                        while (scanpos < entry.target && !scan.atEof()) {
                            int so, sc;
                            bc_next(scan, mod, so, sc, scanpos);
                        }
                        int sop, sopc, steps = 0;
                        while (!scan.atEof() && steps < 24) {
                            bc_next(scan, mod, sop, sopc, scanpos);
                            steps++;
                            if (sop == Pyc::CHECK_EXC_MATCH
                                    || sop == Pyc::CHECK_EG_MATCH) {
                                real_handler = true;
                                break;
                            }
                            if (sop == Pyc::RERAISE || sop == Pyc::RERAISE_A
                                    || sop == Pyc::WITH_EXCEPT_START) {
                                break;
                            }
                        }
                    }
                    /* An entry whose target is a handler already pending
                       as a deferred in-loop container is a propagation
                       path into that outer try (e.g. an except-body tail
                       falling through to the outer handler), not the start
                       of a new source-level try. */
                    bool propagate_to_pending = false;
                    for (const auto& pc : pending_conts) {
                        if (pc->except() == entry.target) {
                            propagate_to_pending = true;
                            break;
                        }
                    }
                    if (real_handler && !propagate_to_pending) {
                        if (curblock->blktype() == ASTBlock::BLK_CONTAINER
                                && !curblock.cast<ASTContainerBlock>()->hasExcept()) {
                            auto cont_ = curblock.cast<ASTContainerBlock>();
                            cont_->setExcept(entry.target);
                            if (!cont_->hasFinally()) {
                                int fin = find_finally_start(exception_entries, source, mod,
                                                             entry.target, pos);
                                if (fin)
                                    cont_->setFinally(fin);
                            }
                        } else if (curblock->blktype() != ASTBlock::BLK_CONTAINER) {
                            int fin = find_finally_start(exception_entries, source, mod,
                                                         entry.target, pos);
                            PycRef<ASTBlock> next = new ASTContainerBlock(fin, entry.target);
                            blocks.push(next.cast<ASTBlock>());
                            curblock = blocks.top();
                        }

                        stack_hist.push(stack);
                        /* Inside a loop the handler lives past the rest of
                           the loop body, so the try block ends at the end
                           of the protected region, not at the handler. */
                        PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY,
                                entry.end_offset, true);
                        blocks.push(tryblock.cast<ASTBlock>());
                        curblock = blocks.top();
                        next_exception_entry++;
                    } else if (propagate_to_pending) {
                        next_exception_entry++;
                    }
                }
            }
        }

        /* Python 3.12+: PEP 709 inline comprehensions emit dead exception
           handlers that restore the saved locals and re-raise.  Their
           target precedes the entry start, so they are unreachable in the
           linear flow; skip them instead of processing their stores. */
        if (mod->verCompare(3, 12) >= 0) {
            bool skipped_handler = false;
            for (const auto& ent : exception_entries) {
                if (ent.target != pos || ent.start_offset >= pos)
                    continue;
                PycBuffer scan = source;
                int scanpos = pos;
                int sop, sopc;
                int steps = 0;
                bool has_store = false, terminated = false, matches = true;
                while (!scan.atEof() && steps < 12) {
                    bc_next(scan, mod, sop, sopc, scanpos);
                    steps++;
                    if (sop == Pyc::RERAISE_A) {
                        terminated = true;
                        break;
                    }
                    if (sop == Pyc::STORE_FAST_A
                            || sop == Pyc::STORE_FAST_STORE_FAST_A
                            || sop == Pyc::STORE_FAST_LOAD_FAST_A) {
                        has_store = true;
                    }
                    if (!isCompRestoreOp(sop)) {
                        matches = false;
                        break;
                    }
                }
                if (matches && terminated && has_store) {
                    /* Consume the handler up to and including the RERAISE. */
                    while (pos < scanpos && !source.atEof())
                        bc_next(source, mod, opcode, operand, pos);
                    while (next_exception_entry < exception_entries.size()
                            && exception_entries[next_exception_entry].start_offset < pos) {
                        next_exception_entry++;
                    }
                    comp_restoring = false;
                    skipped_handler = true;
                    break;
                }
            }
            if (skipped_handler)
                continue;
        }

        if (curblock->blktype() == ASTBlock::BLK_TRY
                && curblock->end() == pos
                && blocks.size() > 1) {
            PycRef<ASTBlock> prev = curblock;
            blocks.pop();
            curblock = blocks.top();

            if (curblock->blktype() == ASTBlock::BLK_CONTAINER
                    && curblock.cast<ASTContainerBlock>()->hasExcept()) {
                if (curblock.cast<ASTContainerBlock>()->except() == pos) {
                    /* The handler region starts right here: pre-open an
                       except block so the handler's statements land in it. */
                    if (!stack_hist.empty()) {
                        stack = stack_hist.top();
                        stack_hist.pop();
                    }

                    curblock->append(prev.cast<ASTNode>());
                    stack_hist.push(stack);

                    PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, 0, NULL, false);
                    except->init();
                    blocks.push(except);
                    curblock = blocks.top();
                } else {
                    /* In-loop try (3.11+ EH table): the handler lives past
                       the rest of the loop body.  Finish the container now
                       (mounting it into its parent so the loop's remaining
                       statements decode after it) and remember it; the
                       handler region re-activates it at its target offset. */
                    if (!stack_hist.empty()) {
                        stack = stack_hist.top();
                        stack_hist.pop();
                    }

                    curblock->append(prev.cast<ASTNode>());
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    blocks.pop();
                    curblock = blocks.top();
                    /* A mounted container (deferred in-loop handler) is
                       already present in its parent; do not append twice.
                       When this container was created nested inside an
                       already-pending outer container, its parent is that
                       container (inside its last try block), not the
                       block-stack top. */
                    if (!cont->mounted()) {
                        if (!pending_conts.empty()
                                && cont->except() > 0
                                && cont->except() < pending_conts.back()->except()) {
                            PycRef<ASTContainerBlock> outer = pending_conts.back();
                            if (outer->size() > 0
                                    && outer->nodes().back()->type() == ASTNode::NODE_BLOCK
                                    && outer->nodes().back().cast<ASTBlock>()->blktype() == ASTBlock::BLK_TRY) {
                                outer->nodes().back().cast<ASTBlock>()->append(cont.cast<ASTNode>());
                            } else {
                                outer->append(cont.cast<ASTNode>());
                            }
                        } else {
                            curblock->append(cont.cast<ASTNode>());
                        }
                    }
                    cont->setMounted();
                    pending_conts.push_back(cont);
                }
            } else {
                blocks.push(prev);
                curblock = prev;
            }
        }

        /* Re-activate a deferred in-loop handler container once decoding
           reaches its handler offset.  Nested tries: the innermost
           handler offset comes first. */
        for (auto it = pending_conts.begin(); it != pending_conts.end(); ++it) {
            if (pos == (*it)->except()) {
                blocks.push((*it).cast<ASTBlock>());
                curblock = (*it).cast<ASTBlock>();
                pending_conts.erase(it);
                break;
            }
        }

        if (curblock->blktype() == ASTBlock::BLK_EXCEPT
                && curblock->end() == pos
                && blocks.size() > 1) {
            PycRef<ASTBlock> prev = curblock;
            blocks.pop();
            curblock = blocks.top();

            if (!stack_hist.empty()) {
                stack = stack_hist.top();
                stack_hist.pop();
            }

            if (prev->size() != 0) {
                curblock->append(prev.cast<ASTNode>());
            }

            if (curblock->blktype() == ASTBlock::BLK_CONTAINER
                    && !curblock.cast<ASTContainerBlock>()->hasFinally()) {
                PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                blocks.pop();
                curblock = blocks.top();
                /* A mounted container (deferred in-loop handler) is
                   already present in its parent; do not append twice. */
                if (!cont->mounted())
                    curblock->append(cont.cast<ASTNode>());
            }
        }

        if ((curblock->blktype() == ASTBlock::BLK_IF
                || curblock->blktype() == ASTBlock::BLK_ELIF
                || curblock->blktype() == ASTBlock::BLK_ELSE)
                && curblock->end() == pos
                && mod->verCompare(3, 12) >= 0
                && blocks.size() > 1) {
            /* With exact jump targets (3.12+), the false exit of a
               conditional can land exactly on the instruction that
               follows the if-body (e.g. the RETURN_CONST of a
               fallthrough return).  Close the block here rather than
               waiting for a later instruction to pass its end, which
               never happens when this is the last statement.  Older
               versions pad the target past the end of the block, and
               the target instruction may still contribute to the
               block, so they must not take this path. */
            PycRef<ASTBlock> prev = curblock;
            blocks.pop();
            curblock = blocks.top();

            if (!stack_hist.empty()) {
                stack = stack_hist.top();
                stack_hist.pop();
            }

            if (prev->size() != 0) {
                curblock->append(prev.cast<ASTNode>());
            }
        }

        /* A pre-scanned `while True:` whose structural exit is a
           conditional jump (continue-style loop: ...; if c: continue;
           return) has no trailing JUMP_BACKWARD after its last body
           statement; close it once the decode reaches the recorded
           exit offset. */
        {
            auto ue = uncond_loop_ends.find(pos);
            if (ue != uncond_loop_ends.end()
                    && curblock->blktype() == ASTBlock::BLK_WHILE
                    && blocks.size() > 1
                    && curblock.cast<ASTCondBlock>()->head() == ue->second) {
                if (!stack_hist.empty()) {
                    stack = stack_hist.top();
                    stack_hist.pop();
                }
                PycRef<ASTBlock> tmp = curblock;
                blocks.pop();
                curblock = blocks.top();
                curblock->append(tmp.cast<ASTNode>());
            }
        }

        /* `while True:` head (pre-scanned): open the loop block right
           before its first body statement is decoded.  The trailing
           JUMP_BACKWARD whose target equals the recorded head closes it. */
        if (uncond_loop_heads.find(pos) != uncond_loop_heads.end()
                && curblock->blktype() != ASTBlock::BLK_WHILE
                && curblock->blktype() != ASTBlock::BLK_FOR
                && curblock->blktype() != ASTBlock::BLK_ASYNCFOR
                && curblock->blktype() != ASTBlock::BLK_TRY
                && curblock->blktype() != ASTBlock::BLK_EXCEPT
                && curblock->blktype() != ASTBlock::BLK_CONTAINER
                && curblock->blktype() != ASTBlock::BLK_FINALLY) {
            stack_hist.push(stack);
            PycRef<ASTCondBlock> wh = new ASTCondBlock(ASTBlock::BLK_WHILE, 0,
                    PycRef<ASTNode>(nullptr), false);
            wh->init();
            wh->setHead(pos);
            blocks.push(wh.cast<ASTBlock>());
            curblock = blocks.top();
        }

        curpos = pos;
        last_op = opcode;
        last_operand = operand;
        bc_next(source, mod, opcode, operand, pos);


        if (need_try && opcode != Pyc::SETUP_EXCEPT_A) {
            need_try = false;

            /* Store the current stack for the except/finally statement(s) */
            stack_hist.push(stack);
            PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY, curblock->end(), true);
            blocks.push(tryblock);
            curblock = blocks.top();
        } else if (else_pop
                && opcode != Pyc::JUMP_FORWARD_A
                && opcode != Pyc::JUMP_IF_FALSE_A
                && opcode != Pyc::JUMP_IF_FALSE_OR_POP_A
                && opcode != Pyc::POP_JUMP_IF_FALSE_A
                && opcode != Pyc::POP_JUMP_FORWARD_IF_FALSE_A
                && opcode != Pyc::JUMP_IF_TRUE_A
                && opcode != Pyc::JUMP_IF_TRUE_OR_POP_A
                && opcode != Pyc::POP_JUMP_IF_TRUE_A
                && opcode != Pyc::POP_JUMP_FORWARD_IF_TRUE_A
                && opcode != Pyc::POP_BLOCK) {
            else_pop = false;

            PycRef<ASTBlock> prev = curblock;
            while (prev->end() < pos
                    && prev->blktype() != ASTBlock::BLK_MAIN) {
                if (prev->blktype() != ASTBlock::BLK_CONTAINER) {
                    if (prev->end() == 0) {
                        break;
                    }

                    /* We want to keep the stack the same, but we need to pop
                     * a level off the history. */
                    //stack = stack_hist.top();
                    if (!stack_hist.empty())
                        stack_hist.pop();
                }
                blocks.pop();

                if (blocks.empty())
                    break;

                curblock = blocks.top();
                curblock->append(prev.cast<ASTNode>());

                prev = curblock;

                CheckIfExpr(stack, curblock);
            }
        }

        switch (opcode) {
        case Pyc::BINARY_OP_A:
            {
                /* Python 3.11+: NB_SUBSCR with a slice operand (a TYPE_SLICE
                   const in 3.14, or a BUILD_SLICE result in 3.11 - 3.13)
                   must produce a slice expression, not a binary op.  The
                   NB_SUBSCR operand number is 25 up to 3.12 and 26 from
                   3.13 onwards. */
                bool is_subscr_op = (operand == 26 && mod->verCompare(3, 13) >= 0)
                        || (operand == 25 && mod->verCompare(3, 11) >= 0 && mod->verCompare(3, 13) < 0);
                if (is_subscr_op) {
                    PycRef<ASTNode> right = stack.top();
                    stack.pop();
                    PycRef<ASTNode> left = stack.top();
                    stack.pop();
                    PycRef<ASTNode> slice = nullptr;
                    if (right.type() == ASTNode::NODE_OBJECT
                            && right.cast<ASTObject>()->object().type() == PycObject::TYPE_SLICE) {
                        PycRef<PycSlice> sl = right.cast<ASTObject>()->object().cast<PycSlice>();
                        auto part = [](PycRef<PycObject> o) -> PycRef<ASTNode> {
                            if (o == nullptr || o == Pyc_None)
                                return nullptr;
                            return new ASTObject(o);
                        };
                        PycRef<ASTNode> start = part(sl->start());
                        PycRef<ASTNode> stop = part(sl->stop());
                        PycRef<ASTNode> step = part(sl->step());
                        if (start == nullptr && stop == nullptr) {
                            slice = new ASTSlice(ASTSlice::SLICE0);
                        } else if (start == nullptr) {
                            slice = new ASTSlice(ASTSlice::SLICE2, start, stop);
                        } else if (stop == nullptr) {
                            slice = new ASTSlice(ASTSlice::SLICE1, start, stop);
                        } else {
                            slice = new ASTSlice(ASTSlice::SLICE3, start, stop);
                        }
                        if (step != nullptr)
                            slice = new ASTSlice(ASTSlice::SLICE3, slice, step);
                    }
                    if (slice != nullptr)
                        stack.push(new ASTSubscr(left, slice));
                    else
                        stack.push(new ASTSubscr(left, right));
                    break;
                }

                ASTBinary::BinOp op = ASTBinary::from_binary_op(operand);
                if (op == ASTBinary::BIN_INVALID)
                    fprintf(stderr, "Unsupported `BINARY_OP` operand value: %d\n", operand);
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, op));
            }
            break;
        case Pyc::BINARY_ADD:
        case Pyc::BINARY_AND:
        case Pyc::BINARY_DIVIDE:
        case Pyc::BINARY_FLOOR_DIVIDE:
        case Pyc::BINARY_LSHIFT:
        case Pyc::BINARY_MODULO:
        case Pyc::BINARY_MULTIPLY:
        case Pyc::BINARY_OR:
        case Pyc::BINARY_POWER:
        case Pyc::BINARY_RSHIFT:
        case Pyc::BINARY_SUBTRACT:
        case Pyc::BINARY_TRUE_DIVIDE:
        case Pyc::BINARY_XOR:
        case Pyc::BINARY_MATRIX_MULTIPLY:
        case Pyc::INPLACE_ADD:
        case Pyc::INPLACE_AND:
        case Pyc::INPLACE_DIVIDE:
        case Pyc::INPLACE_FLOOR_DIVIDE:
        case Pyc::INPLACE_LSHIFT:
        case Pyc::INPLACE_MODULO:
        case Pyc::INPLACE_MULTIPLY:
        case Pyc::INPLACE_OR:
        case Pyc::INPLACE_POWER:
        case Pyc::INPLACE_RSHIFT:
        case Pyc::INPLACE_SUBTRACT:
        case Pyc::INPLACE_TRUE_DIVIDE:
        case Pyc::INPLACE_XOR:
        case Pyc::INPLACE_MATRIX_MULTIPLY:
            {
                ASTBinary::BinOp op = ASTBinary::from_opcode(opcode);
                if (op == ASTBinary::BIN_INVALID)
                    throw std::runtime_error("Unhandled opcode from ASTBinary::from_opcode");
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, op));
            }
            break;
        case Pyc::BINARY_SUBSCR:
            {
                PycRef<ASTNode> subscr = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTSubscr(src, subscr));
            }
            break;
        case Pyc::BREAK_LOOP:
            curblock->append(new ASTKeyword(ASTKeyword::KW_BREAK));
            break;
        case Pyc::BUILD_CLASS:
            {
                PycRef<ASTNode> class_code = stack.top();
                stack.pop();
                PycRef<ASTNode> bases = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTClass(class_code, bases, name));
            }
            break;
        case Pyc::BUILD_FUNCTION:
            {
                PycRef<ASTNode> fun_code = stack.top();
                stack.pop();
                stack.push(new ASTFunction(fun_code, {}, {}));
            }
            break;
        case Pyc::BUILD_LIST_A:
            {
                ASTList::value_t values;
                for (int i=0; i<operand; i++) {
                    values.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTList(values));
            }
            break;
        case Pyc::BUILD_SET_A:
            {
                ASTSet::value_t values;
                for (int i=0; i<operand; i++) {
                    values.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTSet(values));
            }
            break;
        case Pyc::BUILD_MAP_A:
            if (mod->verCompare(3, 5) >= 0) {
                auto map = new ASTMap;
                /* The stack holds key1, value1, ..., keyN, valueN; popping
                   from the top yields the pairs in reverse order, so collect
                   them first and add them front-to-back (Python 3.14 builds
                   dict literals with BUILD_MAP). */
                std::vector<std::pair<PycRef<ASTNode>, PycRef<ASTNode>>> pairs;
                for (int i = 0; i < operand; ++i) {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    pairs.push_back({ key, value });
                }
                for (auto it = pairs.rbegin(); it != pairs.rend(); ++it)
                    map->add(it->first, it->second);
                stack.push(map);
            } else {
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                stack.push(new ASTMap());
            }
            break;
        case Pyc::DICT_MERGE_A:
        case Pyc::DICT_UPDATE_A:
            {
                /* Python 3.9+: merge/update the map below TOS with TOS.
                   A non-map update is a `**expr` unpacking; it is recorded
                   as a pair with a null key (printed as `**expr`). */
                PycRef<ASTNode> update = stack.top();
                stack.pop();
                PycRef<ASTNode> base = stack.top();
                if (base.type() == ASTNode::NODE_MAP) {
                    if (update.type() == ASTNode::NODE_MAP) {
                        for (const auto& kv : update.cast<ASTMap>()->values())
                            base.cast<ASTMap>()->add(kv.first, kv.second);
                    } else {
                        base.cast<ASTMap>()->add(nullptr, update);
                    }
                }
            }
            break;
        case Pyc::BUILD_CONST_KEY_MAP_A:
            // Top of stack will be a tuple of keys.
            // Values will start at TOS - 1.
            {
                PycRef<ASTNode> keys = stack.top();
                stack.pop();

                ASTConstMap::values_t values;
                values.reserve(operand);
                for (int i = 0; i < operand; ++i) {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    values.push_back(value);
                }

                stack.push(new ASTConstMap(keys, values));
            }
            break;
        case Pyc::STORE_MAP:
            {
                PycRef<ASTNode> key = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTMap> map = stack.top().cast<ASTMap>();
                map->add(key, value);
            }
            break;
        case Pyc::BUILD_SLICE_A:
            {
                if (operand == 2) {
                    PycRef<ASTNode> end = stack.top();
                    stack.pop();
                    PycRef<ASTNode> start = stack.top();
                    stack.pop();

                    if (start.type() == ASTNode::NODE_OBJECT
                            && start.cast<ASTObject>()->object() == Pyc_None) {
                        start = NULL;
                    }

                    if (end.type() == ASTNode::NODE_OBJECT
                            && end.cast<ASTObject>()->object() == Pyc_None) {
                        end = NULL;
                    }

                    if (start == NULL && end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE0));
                    } else if (start == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
                    } else if (end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, start, end));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, start, end));
                    }
                } else if (operand == 3) {
                    PycRef<ASTNode> step = stack.top();
                    stack.pop();
                    PycRef<ASTNode> end = stack.top();
                    stack.pop();
                    PycRef<ASTNode> start = stack.top();
                    stack.pop();

                    if (start.type() == ASTNode::NODE_OBJECT
                            && start.cast<ASTObject>()->object() == Pyc_None) {
                        start = NULL;
                    }

                    if (end.type() == ASTNode::NODE_OBJECT
                            && end.cast<ASTObject>()->object() == Pyc_None) {
                        end = NULL;
                    }

                    if (step.type() == ASTNode::NODE_OBJECT
                            && step.cast<ASTObject>()->object() == Pyc_None) {
                        step = NULL;
                    }

                    /* We have to do this as a slice where one side is another slice */
                    /* [[a:b]:c] */

                    if (start == NULL && end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE0));
                    } else if (start == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
                    } else if (end == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, start, end));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, start, end));
                    }

                    PycRef<ASTNode> lhs = stack.top();
                    stack.pop();

                    if (step == NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, lhs, step));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, lhs, step));
                    }
                }
            }
            break;
        case Pyc::BUILD_STRING_A:
            {
                // Nearly identical logic to BUILD_LIST
                ASTList::value_t values;
                for (int i = 0; i < operand; i++) {
                    values.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTJoinedStr(values));
            }
            break;
        case Pyc::BUILD_TUPLE_A:
            {
                // if class is a closure code, ignore this tuple
                PycRef<ASTNode> tos = stack.top();
                if (tos && tos->type() == ASTNode::NODE_LOADBUILDCLASS) {
                    break;
                }

                ASTTuple::value_t values;
                values.resize(operand);
                for (int i=0; i<operand; i++) {
                    values[operand-i-1] = stack.top();
                    stack.pop();
                }
                stack.push(new ASTTuple(values));
            }
            break;
        case Pyc::KW_NAMES_A:
            {

                int kwparams = code->getConst(operand).cast<PycTuple>()->size();
                ASTKwNamesMap kwparamList;
                std::vector<PycRef<PycObject>> keys = code->getConst(operand).cast<PycSimpleSequence>()->values();
                for (int i = 0; i < kwparams; i++) {
                    kwparamList.add(new ASTObject(keys[kwparams - i - 1]), stack.top());
                    stack.pop();
                }
                stack.push(new ASTKwNamesMap(kwparamList));
            }
            break;
        case Pyc::CALL_KW_A:
        case Pyc::INSTRUMENTED_CALL_KW_A:
            {
                /* Python 3.13+: CALL_KW carries the keyword-names tuple on
                   the stack (replacing the separate KW_NAMES opcode).  Pop
                   it, fold the keyword values into an ASTKwNamesMap and
                   reuse the shared CALL path below. */
                PycRef<ASTNode> kwnames_node = stack.top();
                stack.pop();
                int kwcount = 0;
                ASTKwNamesMap kw_names_map;
                PycRef<PycObject> kwnames_obj;
                if (kwnames_node.type() == ASTNode::NODE_OBJECT)
                    kwnames_obj = kwnames_node.cast<ASTObject>()->object();
                if (kwnames_obj != nullptr
                        && (kwnames_obj.type() == PycObject::TYPE_TUPLE
                            || kwnames_obj.type() == PycObject::TYPE_SMALL_TUPLE)) {
                    std::vector<PycRef<PycObject>> keys = kwnames_obj.cast<PycSimpleSequence>()->values();
                    kwcount = (int)keys.size();
                    for (int i = 0; i < kwcount; i++) {
                        kw_names_map.add(new ASTObject(keys[kwcount - i - 1]), stack.top());
                        stack.pop();
                    }
                }
                stack.push(new ASTKwNamesMap(kw_names_map));
                opcode = Pyc::CALL_A;
            }
            /* fall through */
        case Pyc::CALL_A:
        case Pyc::CALL_FUNCTION_A:
        case Pyc::INSTRUMENTED_CALL_A:
            {
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;

                /* Test for the load build class function */
                stack_hist.push(stack);
                int basecnt = 0;
                ASTTuple::value_t bases;
                bases.resize(basecnt);
                PycRef<ASTNode> TOS = stack.top();
                int TOS_type = TOS.type();
                // bases are NODE_NAME and NODE_BINARY at TOS
                while (TOS_type == ASTNode::NODE_NAME || TOS_type == ASTNode::NODE_BINARY) {
                    bases.resize(basecnt + 1);
                    bases[basecnt] = TOS;
                    basecnt++;
                    stack.pop();
                    TOS = stack.top();
                    TOS_type = TOS.type();
                }
                // qualified name is PycString at TOS
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                PycRef<ASTNode> function = stack.top();
                stack.pop();
                /* Python 3.13+ pushes the NULL between __build_class__ and
                   its arguments (LOAD_BUILD_CLASS; PUSH_NULL; ...); skip it
                   before testing for the load-build-class marker. */
                if (mod->verCompare(3, 13) >= 0 && stack.top() == nullptr)
                    stack.pop();
                PycRef<ASTNode> loadbuild = stack.top();
                stack.pop();
                int loadbuild_type = loadbuild.type();
                if (loadbuild_type == ASTNode::NODE_LOADBUILDCLASS) {
                    PycRef<ASTNode> call = new ASTCall(function, pparamList, kwparamList);
                    stack.push(new ASTClass(call, new ASTTuple(bases), name));
                    stack_hist.pop();
                    break;
                }
                else
                {
                    stack = stack_hist.top();
                    stack_hist.pop();
                }

                /* Python 3.11+: implicit `with` epilogue call to
                   __exit__/__aexit__.  The exit callable was left on the
                   stack by BEFORE_WITH / LOAD_SPECIAL; the Nones above it
                   are the arguments of the epilogue call. */
                {
                    PycRef<ASTNode> exitFn;
                    int exitDepth = 0;
                    for (int i = 1; i <= 6; ++i) {
                        PycRef<ASTNode> n = stack.top(i);
                        if (n.type() == ASTNode::NODE_BINARY
                                && n.cast<ASTBinary>()->op() == ASTBinary::BIN_ATTR
                                && n.cast<ASTBinary>()->right().type() == ASTNode::NODE_NAME) {
                            PycRef<PycString> nm = n.cast<ASTBinary>()->right().cast<ASTName>()->name();
                            if (nm->isEqual("__exit__") || nm->isEqual("__aexit__")) {
                                exitFn = n;
                                exitDepth = i;
                                break;
                            }
                        }
                    }
                    if (exitFn != nullptr && exitDepth >= 3) {
                        /* Everything above the marker must be the None
                           arguments of the epilogue call. */
                        bool onlyNones = true;
                        for (int i = 1; i < exitDepth; ++i) {
                            if (stack.top(i) != nullptr) {
                                onlyNones = false;
                                break;
                            }
                        }
                        if (onlyNones) {
                            bool asyncExit = exitFn.cast<ASTBinary>()->right()
                                    .cast<ASTName>()->name()->isEqual("__aexit__");
                            /* Undo the speculative stack snapshot taken for
                               the load-build-class test above. */
                            if (!stack_hist.empty()) {
                                stack = stack_hist.top();
                                stack_hist.pop();
                            }
                            /* Pop only the None arguments; keep the (NULL,
                               exit-mark) pair below so that duplicate exit
                               sites (break, return, continue and fall-through
                               paths each emit their own epilogue call) are
                               still recognized. */
                            for (int i = 1; i < exitDepth; ++i)
                                stack.pop();
                            if (asyncExit) {
                                /* Skip the await loop of the async epilogue:
                                   GET_AWAITABLE; LOAD_CONST None; SEND;
                                   YIELD; RESUME; JUMP_BACKWARD_NO_INTERRUPT;
                                   END_SEND (plus CACHE entries). */
                                do {
                                    bc_next(source, mod, opcode, operand, pos);
                                } while (opcode != Pyc::END_SEND && !source.atEof());
                            }
                            stack.push(new ASTCall(exitFn, {}, {}));
                            break;
                        }
                    }
                }

                /* Python 3.11 - 3.13: duplicate `with` epilogue site.  One
                   epilogue call is emitted per exit path (break, return,
                   continue, fall-through); on later paths the exit marker
                   may already have been consumed, so recognize the shape
                   structurally: a CALL whose argument and self-or-NULL
                   slots are all NULLs (three LOAD_CONST None pushes). */
                if (opcode == Pyc::CALL_A && operand >= 2 && operand <= 3) {
                    bool allNull = true;
                    for (int i = 1; i <= operand + 1; ++i) {
                        if (stack.top(i) != nullptr) {
                            allNull = false;
                            break;
                        }
                    }
                    if (allNull) {
                        if (!stack_hist.empty()) {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }
                        /* Pop the None arguments plus the NULL self slot
                           (at most); the empty-stack top() is a no-op. */
                        for (int i = 0; i < operand + 1 && stack.top(1) == nullptr; ++i)
                            stack.pop();
                        stack.push(new ASTCall(new ASTBinary(nullptr,
                                make_name("__exit__"), ASTBinary::BIN_ATTR),
                                {}, {}));
                        break;
                    }
                }

                /*
                KW_NAMES(i)
                    Stores a reference to co_consts[consti] into an internal variable for use by CALL.
                    co_consts[consti] must be a tuple of strings.
                    New in version 3.11.
                */
                if (mod->verCompare(3, 11) >= 0) {
                    PycRef<ASTNode> object_or_map = stack.top();
                    if (object_or_map.type() == ASTNode::NODE_KW_NAMES_MAP) {
                        stack.pop();
                        PycRef<ASTKwNamesMap> kwparams_map = object_or_map.cast<ASTKwNamesMap>();
                        for (ASTKwNamesMap::map_t::const_iterator it = kwparams_map->values().begin(); it != kwparams_map->values().end(); it++) {
                            kwparamList.push_front(std::make_pair(it->first, it->second));
                            pparams -= 1;
                        }
                    }
                }
                else {
                    for (int i = 0; i < kwparams; i++) {
                        PycRef<ASTNode> val = stack.top();
                        stack.pop();
                        PycRef<ASTNode> key = stack.top();
                        stack.pop();
                        kwparamList.push_front(std::make_pair(key, val));
                    }
                }
                for (int i=0; i<pparams; i++) {
                    PycRef<ASTNode> param = stack.top();
                    stack.pop();
                    if (param.type() == ASTNode::NODE_FUNCTION) {
                        PycRef<ASTNode> fun_code = param.cast<ASTFunction>()->code();
                        PycRef<PycCode> code_src = fun_code.cast<ASTObject>()->object().cast<PycCode>();
                        PycRef<PycString> function_name = code_src->name();
                        if (function_name->isEqual("<lambda>")) {
                            pparamList.push_front(param);
                        } else {
                            // Decorator used
                            PycRef<ASTNode> decor_name = new ASTName(function_name);
                            curblock->append(new ASTStore(param, decor_name));

                            pparamList.push_front(decor_name);
                        }
                    } else {
                        pparamList.push_front(param);
                    }
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                if (opcode == Pyc::CALL_A || opcode == Pyc::INSTRUMENTED_CALL_A) {
                    if (func == nullptr && stack.top() != nullptr) {
                        /* Python 3.13+: the NULL sits between the callable
                           and its arguments (LOAD f; PUSH_NULL; args...). */
                        func = stack.top();
                        stack.pop();
                    } else if (stack.top() == nullptr) {
                        /* Python 3.11 - 3.12: PUSH_NULL; LOAD f; args... */
                        stack.pop();
                    }
                }

                stack.push(new ASTCall(func, pparamList, kwparamList));
            }
            break;
        case Pyc::CALL_FUNCTION_VAR_A:
            {
                PycRef<ASTNode> var = stack.top();
                stack.pop();
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;
                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    pparamList.push_front(stack.top());
                    stack.pop();
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();

                PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
                call.cast<ASTCall>()->setVar(var);
                stack.push(call);
            }
            break;
        case Pyc::CALL_FUNCTION_EX:
        case Pyc::CALL_FUNCTION_EX_A:
        case Pyc::INSTRUMENTED_CALL_FUNCTION_EX_A:
            {
                /* Python 3.14 dropped the flags operand: the calling
                   convention is always [func, args_tuple, kwargs_map].
                   Python 3.6 - 3.13 use a flags operand where bit 0 marks
                   the presence of a kwargs map. */
                PycRef<ASTNode> kw = nullptr;
                if (opcode == Pyc::CALL_FUNCTION_EX) {
                    /* Python 3.14 has no flags operand: when keyword
                       arguments are present, TOS is the merged kwargs map
                       (built by DICT_MERGE). */
                    if (stack.top().type() == ASTNode::NODE_MAP) {
                        kw = stack.top();
                        stack.pop();
                    }
                } else if (operand & 1) {
                    kw = stack.top();
                    stack.pop();
                }
                PycRef<ASTNode> var = stack.top();
                stack.pop();
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                if (mod->verCompare(3, 11) >= 0 && stack.top() == nullptr)
                    stack.pop();

                PycRef<ASTNode> call = new ASTCall(func, {}, {});
                /* A map holding a single **expr unpack is displayed bare. */
                if (kw != nullptr && kw.type() == ASTNode::NODE_MAP) {
                    const auto& vals = kw.cast<ASTMap>()->values();
                    if (vals.size() == 1 && vals.front().first == nullptr)
                        kw = vals.front().second;
                }
                if (var != nullptr)
                    call.cast<ASTCall>()->setVar(var);
                if (kw != nullptr)
                    call.cast<ASTCall>()->setKW(kw);
                stack.push(call);
            }
            break;
        case Pyc::CALL_FUNCTION_KW_A:
            {
                PycRef<ASTNode> kw = stack.top();
                stack.pop();
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;
                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    pparamList.push_front(stack.top());
                    stack.pop();
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();

                PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
                call.cast<ASTCall>()->setKW(kw);
                stack.push(call);
            }
            break;
        case Pyc::CALL_FUNCTION_VAR_KW_A:
            {
                PycRef<ASTNode> kw = stack.top();
                stack.pop();
                PycRef<ASTNode> var = stack.top();
                stack.pop();
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;
                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    pparamList.push_front(stack.top());
                    stack.pop();
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();

                PycRef<ASTNode> call = new ASTCall(func, pparamList, kwparamList);
                call.cast<ASTCall>()->setKW(kw);
                call.cast<ASTCall>()->setVar(var);
                stack.push(call);
            }
            break;
        case Pyc::CALL_METHOD_A:
            {
                ASTCall::pparam_t pparamList;
                for (int i = 0; i < operand; i++) {
                    PycRef<ASTNode> param = stack.top();
                    stack.pop();
                    if (param.type() == ASTNode::NODE_FUNCTION) {
                        PycRef<ASTNode> fun_code = param.cast<ASTFunction>()->code();
                        PycRef<PycCode> code_src = fun_code.cast<ASTObject>()->object().cast<PycCode>();
                        PycRef<PycString> function_name = code_src->name();
                        if (function_name->isEqual("<lambda>")) {
                            pparamList.push_front(param);
                        } else {
                            // Decorator used
                            PycRef<ASTNode> decor_name = new ASTName(function_name);
                            curblock->append(new ASTStore(param, decor_name));

                            pparamList.push_front(decor_name);
                        }
                    } else {
                        pparamList.push_front(param);
                    }
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                stack.push(new ASTCall(func, pparamList, ASTCall::kwparam_t()));
            }
            break;
        case Pyc::CONTINUE_LOOP_A:
            curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
            break;
        case Pyc::COMPARE_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                auto arg = operand;
                if (mod->verCompare(3, 12) == 0)
                    arg >>= 4; // changed under GH-100923
                else if (mod->verCompare(3, 13) >= 0)
                    arg >>= 5;
                stack.push(new ASTCompare(left, right, arg));
            }
            break;
        case Pyc::CONTAINS_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                // The operand will be 0 for 'in' and 1 for 'not in'.
                stack.push(new ASTCompare(left, right, operand ? ASTCompare::CMP_NOT_IN : ASTCompare::CMP_IN));
            }
            break;
        case Pyc::DELETE_ATTR_A:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                curblock->append(new ASTDelete(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR)));
            }
            break;
        case Pyc::DELETE_GLOBAL_A:
            code->markGlobal(code->getName(operand));
            /* Fall through */
        case Pyc::DELETE_NAME_A:
            {
                PycRef<PycString> varname = code->getName(operand);

                if (varname->length() >= 2 && varname->value()[0] == '_'
                        && varname->value()[1] == '[') {
                    /* Don't show deletes that are a result of list comps. */
                    break;
                }

                PycRef<ASTNode> name = new ASTName(varname);
                curblock->append(new ASTDelete(name));
            }
            break;
        case Pyc::DELETE_FAST_A:
            {
                PycRef<ASTNode> name;

                if (mod->verCompare(1, 3) < 0)
                    name = new ASTName(code->getName(operand));
                else
                    name = new ASTName(code->getLocal(operand));

                if (name.cast<ASTName>()->name()->value()[0] == '_'
                        && name.cast<ASTName>()->name()->value()[1] == '[') {
                    /* Don't show deletes that are a result of list comps. */
                    break;
                }

                curblock->append(new ASTDelete(name));
            }
            break;
        case Pyc::DELETE_SLICE_0:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE0))));
            }
            break;
        case Pyc::DELETE_SLICE_1:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE1, upper))));
            }
            break;
        case Pyc::DELETE_SLICE_2:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE2, NULL, lower))));
            }
            break;
        case Pyc::DELETE_SLICE_3:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE3, upper, lower))));
            }
            break;
        case Pyc::DELETE_SUBSCR:
            {
                PycRef<ASTNode> key = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                curblock->append(new ASTDelete(new ASTSubscr(name, key)));
            }
            break;
        case Pyc::DUP_TOP:
            {
                if (stack.top().type() == PycObject::TYPE_NULL) {
                    stack.push(stack.top());
                } else if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    auto chainstore = stack.top();
                    stack.pop();
                    stack.push(stack.top());
                    stack.push(chainstore);
                } else {
                    stack.push(stack.top());
                    ASTNodeList::list_t targets;
                    stack.push(new ASTChainStore(targets, stack.top()));
                }
            }
            break;
        case Pyc::DUP_TOP_TWO:
            {
                PycRef<ASTNode> first = stack.top();
                stack.pop();
                PycRef<ASTNode> second = stack.top();

                stack.push(first);
                stack.push(second);
                stack.push(first);
            }
            break;
        case Pyc::DUP_TOPX_A:
            {
                std::stack<PycRef<ASTNode> > first;
                std::stack<PycRef<ASTNode> > second;

                for (int i = 0; i < operand; i++) {
                    PycRef<ASTNode> node = stack.top();
                    stack.pop();
                    first.push(node);
                    second.push(node);
                }

                while (first.size()) {
                    stack.push(first.top());
                    first.pop();
                }

                while (second.size()) {
                    stack.push(second.top());
                    second.pop();
                }
            }
            break;
        case Pyc::END_FINALLY:
            {
                bool isFinally = false;
                if (curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    PycRef<ASTBlock> final = curblock;
                    blocks.pop();

                    stack = stack_hist.top();
                    stack_hist.pop();

                    curblock = blocks.top();
                    curblock->append(final.cast<ASTNode>());
                    isFinally = true;
                } else if (curblock->blktype() == ASTBlock::BLK_EXCEPT) {
                    blocks.pop();
                    PycRef<ASTBlock> prev = curblock;

                    bool isUninitAsyncFor = false;
                    if (blocks.top()->blktype() == ASTBlock::BLK_CONTAINER) {
                        auto container = blocks.top();
                        blocks.pop();
                        auto asyncForBlock = blocks.top();
                        isUninitAsyncFor = asyncForBlock->blktype() == ASTBlock::BLK_ASYNCFOR && !asyncForBlock->inited();
                        if (isUninitAsyncFor) {
                            auto tryBlock = container->nodes().front().cast<ASTBlock>();
                            if (!tryBlock->nodes().empty() && tryBlock->blktype() == ASTBlock::BLK_TRY) {
                                auto store = tryBlock->nodes().front().try_cast<ASTStore>();
                                if (store) {
                                    asyncForBlock.cast<ASTIterBlock>()->setIndex(store->dest());
                                }
                            }
                            curblock = blocks.top();
                            stack = stack_hist.top();
                            stack_hist.pop();
                            if (!curblock->inited())
                                fprintf(stderr, "Error when decompiling 'async for'.\n");
                        } else {
                            blocks.push(container);
                        }
                    }

                    if (!isUninitAsyncFor) {
                        if (curblock->size() != 0) {
                            blocks.top()->append(curblock.cast<ASTNode>());
                        }

                        curblock = blocks.top();

                        /* Turn it into an else statement. */
                        if (curblock->end() != pos || curblock.cast<ASTContainerBlock>()->hasFinally()) {
                            PycRef<ASTBlock> elseblk = new ASTBlock(ASTBlock::BLK_ELSE, prev->end());
                            elseblk->init();
                            blocks.push(elseblk);
                            curblock = blocks.top();
                        }
                        else {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }
                    }
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    /* This marks the end of the except block(s). */
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    if (!cont->hasFinally() || isFinally) {
                        /* If there's no finally block, pop the container. */
                        blocks.pop();
                        curblock = blocks.top();
                        curblock->append(cont.cast<ASTNode>());
                    }
                }
            }
            break;
        case Pyc::EXEC_STMT:
            {
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> loc = stack.top();
                stack.pop();
                PycRef<ASTNode> glob = stack.top();
                stack.pop();
                PycRef<ASTNode> stmt = stack.top();
                stack.pop();

                curblock->append(new ASTExec(stmt, glob, loc));
            }
            break;
        case Pyc::FOR_ITER_A:
        case Pyc::INSTRUMENTED_FOR_ITER_A:
            {
                PycRef<ASTNode> iter = stack.top(); // Iterable
                if (mod->verCompare(3, 12) < 0) {
                    // Do not pop the iterator for py 3.12+
                    stack.pop();
                }
                /* Pop it? Don't pop it? */

                int end;
                bool comprehension = false;

                // before 3.8, there is a SETUP_LOOP instruction with block start and end position,
                //    the operand is usually a jump to a POP_BLOCK instruction
                // after 3.8, block extent has to be inferred implicitly; the operand is a jump to a position after the for block
                if (mod->majorVer() == 3 && mod->minorVer() >= 8) {
                    end = operand;
                    if (mod->verCompare(3, 10) >= 0)
                        end *= sizeof(uint16_t); // // BPO-27129
                    end += pos;
                    if (mod->verCompare(3, 12) >= 0) {
                        /* FOR_ITER has had one inline cache entry since
                         * 3.12: the jump base is the end of the four-byte
                         * instruction (opcode + operand + cache slot). */
                        end += 2 * sizeof(uint16_t);
                    }
                    comprehension = strcmp(code->name()->value(), "<listcomp>") == 0
                            || comp_saved > 0;
                } else {
                    PycRef<ASTBlock> top = blocks.top();
                    end = top->end(); // block end position from SETUP_LOOP
                    if (top->blktype() == ASTBlock::BLK_WHILE) {
                        blocks.pop();
                    } else {
                        comprehension = true;
                    }
                }

                PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, curpos, end, iter);
                forblk->setComprehension(comprehension);
                blocks.push(forblk.cast<ASTBlock>());
                curblock = blocks.top();

                /* Entering a loop body: any pending epilogue restores from
                   an inner (nested) comprehension are done with. */
                comp_restoring = false;

                stack.push(NULL);
            }
            break;
        case Pyc::FOR_LOOP_A:
            {
                PycRef<ASTNode> curidx = stack.top(); // Current index
                stack.pop();
                PycRef<ASTNode> iter = stack.top(); // Iterable
                stack.pop();

                bool comprehension = false;
                PycRef<ASTBlock> top = blocks.top();
                if (top->blktype() == ASTBlock::BLK_WHILE) {
                    blocks.pop();
                } else {
                    comprehension = true;
                }
                PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, curpos, top->end(), iter);
                forblk->setComprehension(comprehension);
                blocks.push(forblk.cast<ASTBlock>());
                curblock = blocks.top();

                /* Python Docs say:
                      "push the sequence, the incremented counter,
                       and the current item onto the stack." */
                stack.push(iter);
                stack.push(curidx);
                stack.push(NULL); // We can totally hack this >_>
            }
            break;
        case Pyc::GET_AITER:
            {
                // Logic similar to FOR_ITER_A
                PycRef<ASTNode> iter = stack.top(); // Iterable
                stack.pop();

                PycRef<ASTBlock> top = blocks.top();
                if (top->blktype() == ASTBlock::BLK_WHILE) {
                    blocks.pop();
                    PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_ASYNCFOR, curpos, top->end(), iter);
                    blocks.push(forblk.cast<ASTBlock>());
                    curblock = blocks.top();
                    stack.push(nullptr);
                } else {
                     fprintf(stderr, "Unsupported use of GET_AITER outside of SETUP_LOOP\n");
                }
            }
            break;
        case Pyc::GET_ANEXT:
            break;
        case Pyc::FORMAT_VALUE_A:
            {
                auto conversion_flag = static_cast<ASTFormattedValue::ConversionFlag>(operand);
                PycRef<ASTNode> format_spec = nullptr;
                if (conversion_flag & ASTFormattedValue::HAVE_FMT_SPEC) {
                    format_spec = stack.top();
                    stack.pop();
                }
                auto val = stack.top();
                stack.pop();
                stack.push(new ASTFormattedValue(val, conversion_flag, format_spec));
            }
            break;
        case Pyc::CONVERT_VALUE_A:
            {
                /* Python 3.13+: apply str/repr/ascii conversion before
                   formatting.  Modeled as a (possibly no-op) formatted
                   value; FORMAT_SIMPLE / FORMAT_WITH_SPEC finalize it. */
                auto conv = static_cast<ASTFormattedValue::ConversionFlag>(operand & ASTFormattedValue::CONVERSION_MASK);
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                stack.push(new ASTFormattedValue(value, conv, nullptr));
            }
            break;
        case Pyc::FORMAT_SIMPLE:
            /* Python 3.13+: formatting without an explicit format spec;
               the converted value is already on the stack. */
            break;
        case Pyc::FORMAT_WITH_SPEC:
            {
                /* Python 3.13+: attach the format spec to the converted
                   value left by CONVERT_VALUE. */
                PycRef<ASTNode> spec = stack.top();
                stack.pop();
                PycRef<ASTNode> fv = stack.top();
                stack.pop();
                if (fv.type() == ASTNode::NODE_FORMATTEDVALUE) {
                    PycRef<ASTFormattedValue> fmt = fv.cast<ASTFormattedValue>();
                    stack.push(new ASTFormattedValue(fmt->val(), fmt->conversion(), spec));
                } else {
                    stack.push(new ASTFormattedValue(fv, ASTFormattedValue::NONE, spec));
                }
            }
            break;
        case Pyc::GET_AWAITABLE:
        case Pyc::GET_AWAITABLE_A:
            {
                PycRef<ASTNode> object = stack.top();
                stack.pop();
                stack.push(new ASTAwaitable(object));
            }
            break;
        case Pyc::GET_ITER:
        case Pyc::GET_YIELD_FROM_ITER:
            /* We just entirely ignore this */
            break;
        case Pyc::IMPORT_NAME_A:
            if (mod->majorVer() == 1) {
                stack.push(new ASTImport(new ASTName(code->getName(operand)), NULL));
            } else {
                PycRef<ASTNode> fromlist = stack.top();
                stack.pop();
                if (mod->verCompare(2, 5) >= 0)
                    stack.pop();    // Level -- we don't care
                stack.push(new ASTImport(new ASTName(code->getName(operand)), fromlist));
            }
            break;
        case Pyc::IMPORT_FROM_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::IMPORT_STAR:
            {
                PycRef<ASTNode> import = stack.top();
                stack.pop();
                curblock->append(new ASTStore(import, NULL));
            }
            break;
        case Pyc::IS_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                // The operand will be 0 for 'is' and 1 for 'is not'.
                stack.push(new ASTCompare(left, right, operand ? ASTCompare::CMP_IS_NOT : ASTCompare::CMP_IS));
            }
            break;
        case Pyc::JUMP_IF_FALSE_A:
        case Pyc::JUMP_IF_TRUE_A:
        case Pyc::JUMP_IF_FALSE_OR_POP_A:
        case Pyc::JUMP_IF_TRUE_OR_POP_A:
        case Pyc::POP_JUMP_IF_FALSE_A:
        case Pyc::POP_JUMP_IF_TRUE_A:
        case Pyc::POP_JUMP_IF_NONE_A:
        case Pyc::POP_JUMP_IF_NOT_NONE_A:
        case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
        case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
        case Pyc::INSTRUMENTED_POP_JUMP_IF_FALSE_A:
        case Pyc::INSTRUMENTED_POP_JUMP_IF_TRUE_A:
        case Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A:
        case Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A:
            {
                PycRef<ASTNode> cond = stack.top();
                PycRef<ASTCondBlock> ifblk;
                int popped = ASTCondBlock::UNINITED;

                /* Python 3.12+ can invert the branch sense inside loops:
                   when the "then" part ends in an unconditional exit (e.g.
                   a return) and the "else" path simply loops back, the
                   compiler emits
                       POP_JUMP_IF_NOT_NONE  then
                       JUMP_BACKWARD          loop_start
                    then: <then part>
                   The fall-through region is dead code for the if.  Detect
                   that layout and open a forward-conditioned if block at
                   the jump target instead. */
                if (opcode == Pyc::POP_JUMP_IF_NONE_A
                        || opcode == Pyc::POP_JUMP_IF_NOT_NONE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A) {
                    int toffs = operand;
                    if (mod->verCompare(3, 10) >= 0)
                        toffs *= sizeof(uint16_t); // BPO-27129
                    if (mod->verCompare(3, 12) >= 0)
                        toffs += pos;
                    if (mod->verCompare(3, 13) >= 0)
                        toffs += sizeof(uint16_t);

                    if (toffs > pos && curblock->blktype() == ASTBlock::BLK_FOR) {
                        bool pure = true;
                        PycBuffer probe = source;
                        int ppos = pos, pop_, poper_;
                        while (ppos < toffs && pure && !probe.atEof()) {
                            bc_next(probe, mod, pop_, poper_, ppos);
                            if (pop_ != Pyc::JUMP_BACKWARD_A
                                    && pop_ != Pyc::JUMP_BACKWARD_NO_INTERRUPT_A
                                    && pop_ != Pyc::NOT_TAKEN
                                    && pop_ != Pyc::POP_TOP
                                    && pop_ != Pyc::POP_ITER
                                    && pop_ != Pyc::CACHE) {
                                pure = false;
                            }
                        }

                        if (pure && ppos >= toffs) {
                            stack.pop(); /* the tested value */
                            bool jump_if_none = opcode == Pyc::POP_JUMP_IF_NONE_A
                                    || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A;
                            PycRef<ASTNode> newcond = new ASTCompare(cond,
                                    PycRef<ASTNode>(nullptr),
                                    jump_if_none ? ASTCompare::CMP_IS
                                                 : ASTCompare::CMP_IS_NOT);

                            /* Snap-shot the stack so the terminating
                               statement closes this block. */
                            stack_hist.push(stack);

                            /* The then part extends to the end of the
                               enclosing loop; a terminating statement
                               (return/raise) inside closes the block. */
                            int for_end = curblock->end();
                            ifblk = new ASTCondBlock(ASTBlock::BLK_IF, for_end,
                                    newcond, false);
                            blocks.push(ifblk.cast<ASTBlock>());
                            curblock = ifblk;

                            /* Skip the dead loop-back region; the then part
                               at the jump target is decoded inside the new
                               if block. */
                            while (pos < toffs && !source.atEof())
                                bc_next(source, mod, opcode, operand, pos);
                            break;
                        }
                    }
                }

                /* Python 3.12+: POP_JUMP_IF_NONE / NOT_NONE are
                   specializations that hide the comparison; rebuild it as
                   the equivalent (NOT-)FALSE jump on an `is [not] None`
                   condition. */
                if (opcode == Pyc::POP_JUMP_IF_NONE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A
                        || opcode == Pyc::POP_JUMP_IF_NOT_NONE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A) {
                    stack.pop();
                    bool jump_if_none = opcode == Pyc::POP_JUMP_IF_NONE_A
                            || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A;
                    cond = new ASTCompare(cond, PycRef<ASTNode>(nullptr),
                            jump_if_none ? ASTCompare::CMP_IS_NOT : ASTCompare::CMP_IS);
                    stack.push(cond);
                }

                if (opcode == Pyc::POP_JUMP_IF_FALSE_A
                        || opcode == Pyc::POP_JUMP_IF_TRUE_A
                        || opcode == Pyc::POP_JUMP_IF_NONE_A
                        || opcode == Pyc::POP_JUMP_IF_NOT_NONE_A
                        || opcode == Pyc::POP_JUMP_FORWARD_IF_FALSE_A
                        || opcode == Pyc::POP_JUMP_FORWARD_IF_TRUE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_FALSE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_TRUE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A) {
                    /* Pop condition before the jump */
                    stack.pop();
                    popped = ASTCondBlock::PRE_POPPED;
                }

                /* Store the current stack for the else statement(s) */
                stack_hist.push(stack);

                if (opcode == Pyc::JUMP_IF_FALSE_OR_POP_A
                        || opcode == Pyc::JUMP_IF_TRUE_OR_POP_A) {
                    /* Pop condition only if condition is met */
                    stack.pop();
                    popped = ASTCondBlock::POPPED;
                }

                /* "Jump if true" means "Jump if not false" */
                bool neg = opcode == Pyc::JUMP_IF_TRUE_A
                        || opcode == Pyc::JUMP_IF_TRUE_OR_POP_A
                        || opcode == Pyc::POP_JUMP_IF_TRUE_A
                        || opcode == Pyc::POP_JUMP_FORWARD_IF_TRUE_A
                        || opcode == Pyc::INSTRUMENTED_POP_JUMP_IF_TRUE_A;

                int offs = operand;
                if (mod->verCompare(3, 10) >= 0)
                    offs *= sizeof(uint16_t); // // BPO-27129
                if (mod->verCompare(3, 12) >= 0
                        || opcode == Pyc::JUMP_IF_FALSE_A
                        || opcode == Pyc::JUMP_IF_TRUE_A
                        || opcode == Pyc::POP_JUMP_FORWARD_IF_TRUE_A
                        || opcode == Pyc::POP_JUMP_FORWARD_IF_FALSE_A) {
                    /* Offset is relative in these cases */
                    offs += pos;
                }
                if (mod->verCompare(3, 13) >= 0) {
                    /* Python 3.13+ conditional jumps carry one inline cache
                     * entry (the branch counter).  `pos` already skips the
                     * two-byte instruction body, so exactly one extra
                     * cache slot (two bytes) must be added: the target is
                     * curpos + 4 + 2*operand == pos + 2 + 2*operand. */
                    offs += sizeof(uint16_t);
                }

                /* Python 3.8+ dropped SETUP_LOOP, so while loops must be
                   recognized structurally: the body ends with a backward
                   jump to the condition start, and the conditional jump
                   leaves the loop.  Scan the region between here and the
                   jump target; when its final instruction jumps back to
                   before this condition, this is a while loop.  The loop
                   head may sit inside any non-exception block: nested
                   layouts like for->while or for->if->while must be
                   recognized too, not only MAIN/WHILE. */
                if (offs > pos
                        && (curblock->blktype() == ASTBlock::BLK_MAIN
                            || curblock->blktype() == ASTBlock::BLK_WHILE
                            || curblock->blktype() == ASTBlock::BLK_FOR
                            || curblock->blktype() == ASTBlock::BLK_ASYNCFOR
                            || curblock->blktype() == ASTBlock::BLK_IF
                            || curblock->blktype() == ASTBlock::BLK_ELSE
                            || curblock->blktype() == ASTBlock::BLK_ELIF
                            /* A while may equally live inside a try,
                               except, finally or with body; the back-edge
                               and purity checks below still guard against
                               false positives. */
                            || curblock->blktype() == ASTBlock::BLK_TRY
                            || curblock->blktype() == ASTBlock::BLK_EXCEPT
                            || curblock->blktype() == ASTBlock::BLK_FINALLY
                            || curblock->blktype() == ASTBlock::BLK_WITH)) {
                    PycBuffer probe = source;
                    int ppos = pos, pop_ = 0, poper_ = 0;
                    int back = -1, exit_edge = -1;
                    while (ppos < offs && !probe.atEof()) {
                        bc_next(probe, mod, pop_, poper_, ppos);
                        if (pop_ == Pyc::JUMP_BACKWARD_A
                                || pop_ == Pyc::JUMP_BACKWARD_NO_INTERRUPT_A) {
                            int t = poper_;
                            if (mod->verCompare(3, 10) >= 0)
                                t *= sizeof(uint16_t); // BPO-27129
                            /* Same version-dependent landing rule as in
                               the pre-scan above (ppos excludes caches):
                               3.13+ targets one cache slot further back. */
                            if (mod->verCompare(3, 13) >= 0)
                                t -= sizeof(uint16_t);
                            back = ppos - t;
                            exit_edge = back;
                        }
                    }

                    /* Guard against false positives: ... */
                    if (back >= 0 && back <= curpos && back < pos) {
                        PycBuffer vprobe = source;
                        vprobe.setPos(back);
                        int vpos = back, vop = 0, voper = 0;
                        bool pure = true;
                        /* The window stops at this conditional jump: a
                           top-condition `while` legitimately contains its
                           own POP_JUMP right before curpos. */
                        int head_start = (mod->verCompare(3, 6) >= 0)
                                ? curpos - 2 : curpos;
                        while (vpos < head_start && pure && !vprobe.atEof()) {
                            bc_next(vprobe, mod, vop, voper, vpos);
                            switch (vop) {
                            case Pyc::NOT_TAKEN:
                            case Pyc::CACHE:
                                break;
                            case Pyc::JUMP_FORWARD_A:
                            case Pyc::JUMP_IF_FALSE_A:
                            case Pyc::JUMP_IF_TRUE_A:
                            case Pyc::JUMP_IF_FALSE_OR_POP_A:
                            case Pyc::JUMP_IF_TRUE_OR_POP_A:
                            case Pyc::JUMP_ABSOLUTE_A:
                            case Pyc::POP_JUMP_IF_FALSE_A:
                            case Pyc::POP_JUMP_IF_TRUE_A:
                            case Pyc::POP_JUMP_IF_NONE_A:
                            case Pyc::POP_JUMP_IF_NOT_NONE_A:
                            case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
                            case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
                            case Pyc::POP_JUMP_FORWARD_IF_NONE_A:
                            case Pyc::POP_JUMP_FORWARD_IF_NOT_NONE_A:
                            case Pyc::POP_JUMP_BACKWARD_IF_FALSE_A:
                            case Pyc::POP_JUMP_BACKWARD_IF_TRUE_A:
                            case Pyc::POP_JUMP_BACKWARD_IF_NONE_A:
                            case Pyc::POP_JUMP_BACKWARD_IF_NOT_NONE_A:
                            case Pyc::JUMP_BACKWARD_A:
                            case Pyc::JUMP_BACKWARD_NO_INTERRUPT_A:
                            case Pyc::FOR_ITER_A:
                            case Pyc::RETURN_VALUE:
                            case Pyc::RETURN_GENERATOR:
                            case Pyc::RETURN_CONST_A:
                            case Pyc::RAISE_VARARGS_A:
                            case Pyc::RERAISE_A:
                            case Pyc::END_FOR:
                                pure = false;
                                break;
                            default:
                                break;
                            }
                        }
                        if (!pure) {
                            back = -1;
                        }
                    }

                    /* Python 3.14 tail-optimized while loop: the true edge
                       of the condition enters the body and the false path
                       falls through NOT_TAKEN to a backward jump out of the
                       loop (continuing the enclosing iteration), so the
                       scan above only finds that exit edge and purity
                       correctly rejects it.  A genuine loop still owns a
                       back edge to the condition start at the end of its
                       body; a plain if with the same shape never does,
                       because every back edge inside its region targets an
                       enclosing loop head, which is strictly before this
                       condition. */
                    if (back < 0
                            && mod->verCompare(3, 14) >= 0
                            && (opcode == Pyc::POP_JUMP_IF_TRUE_A
                                || opcode == Pyc::POP_JUMP_FORWARD_IF_TRUE_A)
                            && exit_edge >= 0 && exit_edge < curpos) {
                        PycBuffer bprobe = source;
                        bprobe.setPos(offs);
                        int bpos = offs, bop = 0, boper = 0, hit = -1, hit_target = -1;
                        while (bpos < pos + 0x10000 && hit < 0 && !bprobe.atEof()) {
                            bc_next(bprobe, mod, bop, boper, bpos);
                            if (bop == Pyc::JUMP_BACKWARD_A
                                    || bop == Pyc::JUMP_BACKWARD_NO_INTERRUPT_A) {
                                int t = boper;
                                if (mod->verCompare(3, 10) >= 0)
                                    t *= sizeof(uint16_t); // BPO-27129
                                if (mod->verCompare(3, 13) >= 0)
                                    t -= sizeof(uint16_t); // 3.13+ jump cache slot
                                t = bpos - t;
                                /* The loop head sits between the enclosing
                                   loop head (exit_edge) and the body start
                                   (offs); a `continue` of the enclosing
                                   loop targets exit_edge itself and inner
                                   loop heads lie at or after offs. */
                                if (t < offs && t > exit_edge) {
                                    hit = bpos;
                                    hit_target = t;
                                }
                            } else if (bop == Pyc::RETURN_VALUE
                                    || bop == Pyc::RETURN_CONST_A) {
                                break;
                            }
                        }
                        if (hit >= 0) {
                            /* The body's trailing back edge closes the
                               block (JUMP_BACKWARD handler), so the block
                               end just marks the end of the body. */
                            ifblk = new ASTCondBlock(ASTBlock::BLK_WHILE,
                                    hit + 2 * (int)sizeof(uint16_t), cond, !neg);
                            ifblk->setHead(hit_target);
                            ifblk->init();
                            blocks.push(ifblk.cast<ASTBlock>());
                            curblock = ifblk;
                            break;
                        }
                    }

                    /* Python 3.13 continue-shaped while loop: `while cond:`
                       whose false path continues an enclosing loop.  The
                       true edge enters the body (offs) and the false path
                       falls through to a backward jump to the enclosing
                       head (exit_edge), so the body owns the back edge to
                       offs itself.  A mid-body `continue` also targets
                       offs, but its guard conditional's false path stays
                       inside the body.  The closing edge is recognized by
                       the re-evaluated condition before it, whose false
                       path (prev_target) leaves the loop through a
                       backward jump to the same enclosing head the
                       entry's false path used. */
                    if (back < 0
                            && mod->verCompare(3, 13) >= 0
                            && (opcode == Pyc::POP_JUMP_IF_TRUE_A
                                || opcode == Pyc::POP_JUMP_FORWARD_IF_TRUE_A)
                            && exit_edge >= 0 && exit_edge < curpos) {
                        PycBuffer bprobe = source;
                        bprobe.setPos(offs);
                        int bpos = offs, bop = 0, boper = 0;
                        int prev_target = -1;
                        int hit = -1, hit_end = -1;
                        while (bpos < pos + 0x10000 && hit < 0 && !bprobe.atEof()) {
                            int istart = bpos;
                            bc_next(bprobe, mod, bop, boper, bpos);
                            if (bop == Pyc::JUMP_BACKWARD_A
                                    || bop == Pyc::JUMP_BACKWARD_NO_INTERRUPT_A) {
                                int t = boper;
                                if (mod->verCompare(3, 10) >= 0)
                                    t *= sizeof(uint16_t); // BPO-27129
                                if (mod->verCompare(3, 13) >= 0)
                                    t -= sizeof(uint16_t); // 3.13+ jump cache slot
                                t = bpos - t;
                                /* Candidate closing edge: returns to the
                                   body start (offs) right after a bottom
                                   re-evaluation whose false path
                                   (prev_target) leaves the loop. */
                                if (t == offs && prev_target > istart) {
                                    PycBuffer eprobe = source;
                                    eprobe.setPos(prev_target);
                                    int epos = prev_target, eop = 0, eoper = 0;
                                    bc_next(eprobe, mod, eop, eoper, epos);
                                    if (eop == Pyc::JUMP_BACKWARD_A
                                            || eop == Pyc::JUMP_BACKWARD_NO_INTERRUPT_A) {
                                        int et = eoper;
                                        if (mod->verCompare(3, 10) >= 0)
                                            et *= sizeof(uint16_t); // BPO-27129
                                        if (mod->verCompare(3, 13) >= 0)
                                            et -= sizeof(uint16_t); // 3.13+ jump cache slot
                                        et = epos - et;
                                        if (et == exit_edge) {
                                            hit = istart;
                                            hit_end = prev_target;
                                        }
                                    }
                                }
                                prev_target = -1;
                            } else if (bop == Pyc::RETURN_VALUE
                                    || bop == Pyc::RETURN_CONST_A) {
                                break;
                            } else if (bop != Pyc::CACHE) {
                                /* CACHE filler slots (inline cache entries)
                                   must not disturb prev_target: a conditional
                                   jump is immediately followed by its cache
                                   slot, and treating it as a statement would
                                   erase the jump's recorded target. */
                                prev_target = -1;
                                switch (bop) {
                                case Pyc::POP_JUMP_IF_FALSE_A:
                                case Pyc::POP_JUMP_IF_TRUE_A:
                                case Pyc::POP_JUMP_IF_NONE_A:
                                case Pyc::POP_JUMP_IF_NOT_NONE_A:
                                case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
                                case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
                                case Pyc::POP_JUMP_FORWARD_IF_NONE_A:
                                case Pyc::POP_JUMP_FORWARD_IF_NOT_NONE_A: {
                                    int t2 = boper;
                                    if (mod->verCompare(3, 10) >= 0)
                                        t2 *= sizeof(uint16_t); // BPO-27129
                                    if (mod->verCompare(3, 13) >= 0)
                                        t2 += sizeof(uint16_t); // inline cache slot
                                    prev_target = istart + (int)sizeof(uint16_t) + t2;
                                    break;
                                }
                                default:
                                    break;
                                }
                            }
                        }
                        if (hit >= 0) {
                            /* The re-evaluated condition at the bottom is
                               swallowed by the re-emit special case (block
                               end == its false target), and the trailing
                               back edge to offs closes the block. */
                            ifblk = new ASTCondBlock(ASTBlock::BLK_WHILE,
                                    hit_end, cond, !neg);
                            ifblk->setHead(offs);
                            ifblk->init();
                            blocks.push(ifblk.cast<ASTBlock>());
                            curblock = ifblk;
                            break;
                        }
                    }

                    if (back >= 0 && (back <= curpos
                                || back == pos + (int)sizeof(uint16_t))) {
                        /* back <= curpos: the jump returns to the condition
                           (3.14 lays the loop out that way); back just past
                           this instruction: Python 3.13 re-emits the
                           condition at the back edge and jumps back to the
                           body instead. */
                        if (curblock->blktype() == ASTBlock::BLK_WHILE
                                && curblock->end() == offs) {
                            /* Python 3.13 re-emits the loop condition at
                               the back edge (the backward jump targets the
                               body, not the head).  The loop block was
                               already created at the entry condition; the
                               repeated condition leaves no statements, and
                               the trailing backward jump closes the loop. */
                            stack_hist.pop();
                            break;
                        }
                        if (curblock->blktype() == ASTBlock::BLK_WHILE
                                && offs + 4 * (int)sizeof(uint16_t) >= curblock->end()
                                && offs <= curblock->end() + 4 * (int)sizeof(uint16_t)) {
                            /* Variant of the above used when the loop body
                               yields: the exit target is split into a
                               fallthrough copy right after the repeated
                               condition, so the jump target can differ from
                               the loop block end by a RETURN_CONST.  Detect
                               the shape by the backward jump that must
                               follow the repeated condition. */
                            PycBuffer probe = source;
                            int npos = pos, nop, noper;
                            bc_next(probe, mod, nop, noper, npos);
                            while (nop == Pyc::CACHE && !probe.atEof()) {
                                bc_next(probe, mod, nop, noper, npos);
                            }
                            if (nop == Pyc::JUMP_BACKWARD_A
                                    || nop == Pyc::JUMP_BACKWARD_NO_INTERRUPT_A) {
                                stack_hist.pop();
                                break;
                            }
                        }

                        /* It's a while loop: the block already snap-shotted
                           above is restored when the backward jump closes
                           the loop. */
                        ifblk = new ASTCondBlock(ASTBlock::BLK_WHILE, offs, cond, neg);
                        /* The condition is already set, so statements in the
                           body must not be swallowed by the block-init path
                           of the POP_TOP handler (expression statements such
                           as method calls end in POP_TOP). */
                        ifblk->init();
                        blocks.push(ifblk.cast<ASTBlock>());
                        curblock = ifblk;
                        break;
                    }
                }

                if (cond.type() == ASTNode::NODE_COMPARE
                        && cond.cast<ASTCompare>()->op() == ASTCompare::CMP_EXCEPTION) {
                    int except_end = offs;
                    if (curblock->blktype() == ASTBlock::BLK_EXCEPT) {
                        if (curblock.cast<ASTCondBlock>()->cond() == NULL) {
                            except_end = curblock->end();
                            blocks.pop();
                            curblock = blocks.top();

                            stack_hist.pop();
                        } else {
                            /* Chained `except` handlers: close the current
                               handler and start the next one as a sibling. */
                            PycRef<ASTBlock> prev = curblock;
                            blocks.pop();
                            curblock = blocks.top();
                            if (!stack_hist.empty())
                                stack_hist.pop();
                            if (prev->size() != 0)
                                curblock->append(prev.cast<ASTNode>());
                        }
                    }

                    ifblk = new ASTCondBlock(ASTBlock::BLK_EXCEPT, except_end, cond.cast<ASTCompare>()->right(), false);
                } else if (curblock->blktype() == ASTBlock::BLK_ELSE
                           && curblock->size() == 0) {
                    /* Collapse into elif statement */
                    blocks.pop();
                    stack = stack_hist.top();
                    stack_hist.pop();
                    ifblk = new ASTCondBlock(ASTBlock::BLK_ELIF, offs, cond, neg);
                } else if (curblock->size() == 0 && !curblock->inited()
                           && curblock->blktype() == ASTBlock::BLK_WHILE) {
                    /* The condition for a while loop */
                    PycRef<ASTBlock> top = blocks.top();
                    blocks.pop();
                    ifblk = new ASTCondBlock(top->blktype(), offs, cond, neg);

                    /* We don't store the stack for loops! Pop it! */
                    stack_hist.pop();
                } else if (curblock->size() == 0 && curblock->end() <= offs
                           && (curblock->blktype() == ASTBlock::BLK_IF
                           || curblock->blktype() == ASTBlock::BLK_ELIF
                           || curblock->blktype() == ASTBlock::BLK_WHILE)) {
                    PycRef<ASTNode> newcond;
                    PycRef<ASTCondBlock> top = curblock.cast<ASTCondBlock>();
                    PycRef<ASTNode> cond1 = top->cond();
                    blocks.pop();

                    if (curblock->blktype() == ASTBlock::BLK_WHILE) {
                        stack_hist.pop();
                    } else {
                        FastStack s_top = stack_hist.top();
                        stack_hist.pop();
                        stack_hist.pop();
                        stack_hist.push(s_top);
                    }

                    /* Chained comparison (3.12+): the first segment's false
                       target lands on a POP_TOP that discards the chained
                       midpoint.  The segments must combine with AND.  When
                       the cleanup is followed by a jump to the same target
                       as this jump, or when this jump has the opposite
                       polarity of the first segment, the statement body is
                       entered when the chain is false, i.e. the combined
                       condition is negated. */
                    bool chain_mid = false;
                    bool chain_neg = false;
                    if (mod->verCompare(3, 12) >= 0
                            && (curblock->blktype() == ASTBlock::BLK_IF
                                || curblock->blktype() == ASTBlock::BLK_ELIF)
                            && curblock->end() > curpos) {
                        PycBuffer probe = source;
                        int ppos = pos, pop_ = 0, poper_ = 0;
                        /* advance to the block end (the false target) */
                        while (ppos < curblock->end() && !probe.atEof())
                            bc_next(probe, mod, pop_, poper_, ppos);
                        if (ppos == curblock->end()) {
                            bc_next(probe, mod, pop_, poper_, ppos);
                            while (pop_ == Pyc::CACHE && !probe.atEof())
                                bc_next(probe, mod, pop_, poper_, ppos);
                        }
                        if (pop_ == Pyc::POP_TOP) {
                            bc_next(probe, mod, pop_, poper_, ppos);
                            while (pop_ == Pyc::CACHE && !probe.atEof())
                                bc_next(probe, mod, pop_, poper_, ppos);
                            if (pop_ == Pyc::JUMP_FORWARD_A) {
                                /* bc_next already advanced past the operand,
                                   so the target is ppos + 2*arg */
                                int t = ppos + 2 * poper_;
                                if (t == offs)
                                    chain_mid = chain_neg = true;
                            } else if (neg != top->negative()) {
                                /* mixed polarity: the body runs when the
                                   chain is false (e.g. `if not a < b < c`) */
                                chain_mid = chain_neg = true;
                            } else if (pop_ == Pyc::LOAD_CONST_A
                                    || pop_ == Pyc::RETURN_CONST_A
                                    || pop_ == Pyc::RETURN_VALUE) {
                                int pop2 = 0, poper2 = 0;
                                if (pop_ == Pyc::LOAD_CONST_A)
                                    bc_next(probe, mod, pop2, poper2, ppos);
                                else
                                    pop2 = pop_;
                                if (pop2 == Pyc::RETURN_CONST_A
                                        || pop2 == Pyc::RETURN_VALUE) {
                                    /* dead exit epilogue (POP_TOP; LOAD_CONST;
                                       RETURN...): the body runs when the
                                       chain is true */
                                    chain_mid = true;
                                }
                            }
                        }
                    }

                    if (chain_mid) {
                        newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_AND);
                        if (chain_neg)
                            newcond = new ASTUnary(newcond, ASTUnary::UN_NOT);
                    } else if (curblock->end() == offs
                            || (curblock->end() == curpos && !top->negative())) {
                        /* if blah and blah */
                        newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_AND);
                    } else {
                        /* if blah or blah */
                        newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_OR);
                    }
                    ifblk = new ASTCondBlock(top->blktype(), offs, newcond, neg);
                } else if (curblock->blktype() == ASTBlock::BLK_FOR
                            && curblock.cast<ASTIterBlock>()->isComprehension()
                            && mod->verCompare(2, 7) >= 0) {
                    /* Comprehension condition */
                    curblock.cast<ASTIterBlock>()->setCondition(cond);
                    stack_hist.pop();
                    // TODO: Handle older python versions, where condition
                    // is laid out a little differently.
                    break;
                } else {
                    /* Plain old if statement */
                    ifblk = new ASTCondBlock(ASTBlock::BLK_IF, offs, cond, neg);
                }

                if (popped)
                    ifblk->init(popped);

                blocks.push(ifblk.cast<ASTBlock>());
                curblock = blocks.top();
            }
            break;
        case Pyc::JUMP_BACKWARD_NO_INTERRUPT_A:
            /* Python 3.11+: backward jump used only to resume a SEND loop
               inside await/async-with/async-for machinery.  Treating it as
               a loop jump would wrap the await internals in a bogus loop
               block, so treat it as a no-op. */
            break;
        case Pyc::SEND_A:
            /* Python 3.11+: send TOS to the coroutine/iterator below it.
               The sent value is consumed by END_SEND; the awaited result is
               represented by the awaitable node already on the stack. */
            break;
        case Pyc::END_SEND:
            /* Python 3.12+: finish a SEND; drop the value that was sent. */
            stack.pop();
            break;
        case Pyc::JUMP_ABSOLUTE_A:
        // bpo-47120: Replaced JUMP_ABSOLUTE by the relative jump JUMP_BACKWARD.
        case Pyc::JUMP_BACKWARD_A:
            {
                int offs = operand;
                if (mod->verCompare(3, 10) >= 0)
                    offs *= sizeof(uint16_t); // // BPO-27129 

                if (offs < pos) {
                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && comp_saved > 0) {
                        /* PEP 709 inline comprehension loop-back (either the
                           append edge or an `if` filter's false edge): the
                           block stays open and is closed by END_FOR, which
                           also folds the generator into the comprehension
                           node still on the stack.  The stack is already in
                           its loop-entry shape here. */
                        break;
                    }

                    if (curblock->blktype() == ASTBlock::BLK_WHILE
                            && mod->verCompare(3, 8) >= 0) {
                        /* Python 3.8+ while loop (no SETUP_LOOP/POP_BLOCK
                           pair): the backward jump at the end of the body
                           closes the loop block created at the conditional
                           jump.  On 3.7 and older the loop block is closed
                           by POP_BLOCK instead. */
                        int head = curblock.cast<ASTCondBlock>()->head();
                        /* offs is the backward *distance* (2*operand); the
                           absolute target is derived from the instruction
                           position, using the same version-dependent cache
                           rule as the pre-scan (3.13 and 3.14 both land
                           one cache slot further than older versions). */
                        int target = pos - offs;
                        if (mod->verCompare(3, 13) >= 0)
                            target += sizeof(uint16_t);
                        if (head < 0 || target == head) {
                            if (!stack_hist.empty()) {
                                stack = stack_hist.top();
                                stack_hist.pop();
                            }
                            PycRef<ASTBlock> tmp = curblock;
                            blocks.pop();
                            curblock = blocks.top();
                            curblock->append(tmp.cast<ASTNode>());
                            break;
                        }
                        /* A head was recorded (3.14 tail-optimized loop):
                           any other backward edge is the false path's exit
                           to the enclosing loop and leaves no statement. */
                        break;
                    }

                    if (curblock->blktype() == ASTBlock::BLK_IF
                            || curblock->blktype() == ASTBlock::BLK_ELIF) {
                        /* Python 3.14 tail-optimized if/else at the end of
                           a `while True:` body: the if arm ends with the
                           loop's back edge (no JUMP_FORWARD hop like older
                           versions) and the if's false target is the loop's
                           structural exit.  Close the if and decode the
                           exit region as its else arm, kept as a child of
                           the loop so the else renders inside the loop. */
                        int target = pos - offs;
                        if (mod->verCompare(3, 13) >= 0)
                            target += sizeof(uint16_t);
                        bool as_else = false;
                        {
                            std::stack<PycRef<ASTBlock>> blocksCopy = blocks;
                            while (blocksCopy.size() > 1) {
                                PycRef<ASTBlock> blk = blocksCopy.top();
                                blocksCopy.pop();
                                if (blk->blktype() != ASTBlock::BLK_WHILE)
                                    continue;
                                int head = blk.cast<ASTCondBlock>()->head();
                                int ifend = curblock->end();
                                auto ue = uncond_loop_ends.find(ifend);
                                if (head >= 0 && target == head
                                        && ue != uncond_loop_ends.end()
                                        && ue->second == head) {
                                    as_else = true;
                                }
                                break;
                            }
                        }
                        if (as_else) {
                            if (!stack_hist.empty()) {
                                stack = stack_hist.top();
                                stack_hist.pop();
                            }
                            PycRef<ASTBlock> tmp = curblock;
                            blocks.pop();
                            curblock = blocks.top();
                            curblock->append(tmp.cast<ASTNode>());
                            PycRef<ASTBlock> elseblk =
                                    new ASTBlock(ASTBlock::BLK_ELSE, 0);
                            blocks.push(elseblk);
                            curblock = blocks.top();
                            while_else_exit = true;
                            break;
                        }
                    }

                    if (curblock->blktype() == ASTBlock::BLK_FOR) {
                        bool is_jump_to_start = offs == curblock.cast<ASTIterBlock>()->start();
                        bool should_pop_for_block = curblock.cast<ASTIterBlock>()->isComprehension();
                        // in v3.8, SETUP_LOOP is deprecated and for blocks aren't terminated by POP_BLOCK, so we add them here
                        bool should_add_for_block = mod->majorVer() == 3 && mod->minorVer() >= 8 && is_jump_to_start && !curblock.cast<ASTIterBlock>()->isComprehension();

                        if (should_pop_for_block || should_add_for_block) {
                            /* For inline comprehensions the node directly on
                               top is the deepest active iterator, not the
                               result; scan a few slots down for the
                               comprehension node. */
                            PycRef<ASTNode> top;
                            for (int depth = 1; depth <= 5; depth++) {
                                PycRef<ASTNode> probe = stack.top(depth);
                                if (probe.type() == ASTNode::NODE_COMPREHENSION) {
                                    top = probe;
                                    break;
                                }
                            }

                            if (top.type() == ASTNode::NODE_COMPREHENSION) {
                                PycRef<ASTComprehension> comp = top.cast<ASTComprehension>();

                                comp->addGenerator(curblock.cast<ASTIterBlock>());
                            }

                            PycRef<ASTBlock> tmp = curblock;
                            blocks.pop();
                            curblock = blocks.top();
                            if (should_add_for_block) {
                                curblock->append(tmp.cast<ASTNode>());
                            }
                        }
                    } else if (curblock->blktype() == ASTBlock::BLK_ELSE) {
                        stack = stack_hist.top();
                        stack_hist.pop();

                        blocks.pop();
                        blocks.top()->append(curblock.cast<ASTNode>());
                        curblock = blocks.top();

                        if (curblock->blktype() == ASTBlock::BLK_CONTAINER
                                && !curblock.cast<ASTContainerBlock>()->hasFinally()) {
                            blocks.pop();
                            blocks.top()->append(curblock.cast<ASTNode>());
                            curblock = blocks.top();
                        }
                    } else {
                        curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
                    }

                    /* We're in a loop, this jumps back to the start */
                    /* I think we'll just ignore this case... */
                    break; // Bad idea? Probably!
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    if (cont->hasExcept() && pos < cont->except()) {
                        PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, 0, NULL, false);
                        except->init();
                        blocks.push(except);
                        curblock = blocks.top();
                    }
                    break;
                }

                if (!stack_hist.empty()) {
                    stack = stack_hist.top();
                    stack_hist.pop();
                } else {
                    fprintf(stderr, "Warning: Stack history is empty, something wrong might have happened\n");
                }

                PycRef<ASTBlock> prev = curblock;
                PycRef<ASTBlock> nil;
                bool push = true;

                do {
                    blocks.pop();

                    blocks.top()->append(prev.cast<ASTNode>());

                    if (prev->blktype() == ASTBlock::BLK_IF
                            || prev->blktype() == ASTBlock::BLK_ELIF) {
                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTBlock(ASTBlock::BLK_ELSE, blocks.top()->end());
                        if (prev->inited() == ASTCondBlock::PRE_POPPED) {
                            next->init(ASTCondBlock::PRE_POPPED);
                        }

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_EXCEPT) {
                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_EXCEPT, blocks.top()->end(), NULL, false);
                        next->init();

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_ELSE) {
                        /* Special case */
                        prev = blocks.top();
                        if (!push) {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }
                        push = false;
                    } else {
                        prev = nil;
                    }

                } while (prev != nil);

                curblock = blocks.top();
            }
            break;
        case Pyc::JUMP_FORWARD_A:
        case Pyc::INSTRUMENTED_JUMP_FORWARD_A:
            {
                int offs = operand;
                if (mod->verCompare(3, 10) >= 0) {
                    offs *= sizeof(uint16_t); // // BPO-27129
                    /* Relative jump: the base is the end of this two-byte
                     * instruction (JUMP_FORWARD carries no inline cache
                     * entry in any version), i.e. curpos + 2 + 2*operand,
                     * which equals pos + 2*operand. */
                    offs += pos;
                }

                /* Dead-code skip inside a conditional block (3.12+): a
                   forward jump whose target lies within the current
                   block — e.g. past the dead exit epilogue of a chained
                   comparison — only moves the instruction pointer. */
                if (mod->verCompare(3, 12) >= 0 && blocks.size() > 1
                        && (curblock->blktype() == ASTBlock::BLK_IF
                            || curblock->blktype() == ASTBlock::BLK_ELIF)
                        && offs > pos && offs < curblock->end()) {
                    while (pos < offs && !source.atEof())
                        bc_next(source, mod, opcode, operand, pos);
                    break;
                }

                /* With BPO-27129 (3.10+) the base is already folded into
                   offs above, so it is the absolute target; older builds
                   keep the raw relative operand and need pos added. */
                int endpos = (mod->verCompare(3, 10) >= 0) ? offs : pos + offs;

                /* Python 3.11+: `break` out of a for loop compiles to a
                   plain JUMP_FORWARD whose target lies beyond the loop's
                   extent (past the END_FOR/POP_ITER epilogue and any
                   for-else arm).  Recognize it and emit the keyword
                   instead of letting the exit region be decoded inside
                   the current block. */
                if (mod->verCompare(3, 11) >= 0 && blocks.size() > 1) {
                    PycRef<ASTBlock> loopblk;
                    {
                        std::stack<PycRef<ASTBlock>> scan = blocks;
                        while (!scan.empty()) {
                            PycRef<ASTBlock> b = scan.top();
                            scan.pop();
                            if (b->blktype() == ASTBlock::BLK_FOR
                                    || b->blktype() == ASTBlock::BLK_ASYNCFOR) {
                                loopblk = b;
                                break;
                            }
                        }
                    }
                    bool unwind_ok = loopblk != nullptr
                            && loopblk->end() > 0
                            && endpos > loopblk->end()
                            && !loopblk.cast<ASTIterBlock>()->isComprehension();
                    if (unwind_ok) {
                        /* Only unwind plain conditional blocks; a break
                           crossing a try boundary has extra epilogue
                           handling that this fast path must not disturb. */
                        std::stack<PycRef<ASTBlock>> scan = blocks;
                        while (scan.top() != loopblk) {
                            ASTBlock::BlkType t = scan.top()->blktype();
                            if (t != ASTBlock::BLK_IF && t != ASTBlock::BLK_ELIF
                                    && t != ASTBlock::BLK_ELSE) {
                                unwind_ok = false;
                                break;
                            }
                            scan.pop();
                        }
                    }
                    if (unwind_ok) {
                        curblock->append(new ASTKeyword(ASTKeyword::KW_BREAK));
                        while (blocks.top() != loopblk) {
                            PycRef<ASTBlock> inner = blocks.top();
                            blocks.pop();
                            blocks.top()->append(inner.cast<ASTNode>());
                        }
                        curblock = blocks.top();
                        curblock.cast<ASTIterBlock>()->setElseEnd(endpos);
                        break;
                    }
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    if (cont->hasExcept()) {
                        stack_hist.push(stack);

                        curblock->setEnd(endpos);
                        PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, endpos, NULL, false);
                        except->init();
                        blocks.push(except);
                        curblock = blocks.top();
                    }
                    break;
                }

                if (!stack_hist.empty()) {
                    if (stack.empty()) // if it's part of if-expression, TOS at the moment is the result of "if" part
                        stack = stack_hist.top();
                    stack_hist.pop();
                }

                PycRef<ASTBlock> prev = curblock;
                PycRef<ASTBlock> nil;
                bool push = true;

                do {
                    blocks.pop();

                    if (!blocks.empty())
                        blocks.top()->append(prev.cast<ASTNode>());

                    if (prev->blktype() == ASTBlock::BLK_IF
                            || prev->blktype() == ASTBlock::BLK_ELIF) {
                        if (offs == 0) {
                            prev = nil;
                            continue;
                        }

                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTBlock(ASTBlock::BLK_ELSE, endpos);
                        if (prev->inited() == ASTCondBlock::PRE_POPPED) {
                            next->init(ASTCondBlock::PRE_POPPED);
                        }

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_EXCEPT) {
                        if (offs == 0) {
                            prev = nil;
                            continue;
                        }

                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_EXCEPT, endpos, NULL, false);
                        next->init();

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_ELSE) {
                        /* Special case */
                        prev = blocks.top();
                        if (!push) {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }
                        push = false;

                        if (prev->blktype() == ASTBlock::BLK_MAIN) {
                            /* Something went out of control! */
                            prev = nil;
                        }
                    } else if (prev->blktype() == ASTBlock::BLK_TRY
                            && prev->end() < pos+offs) {
                        /* Need to add an except/finally block */
                        if (!stack_hist.empty()) {
                            stack = stack_hist.top();
                            stack_hist.pop();
                        }

                        if (blocks.top()->blktype() == ASTBlock::BLK_CONTAINER) {
                            PycRef<ASTContainerBlock> cont = blocks.top().cast<ASTContainerBlock>();
                            if (cont->hasExcept()) {
                                if (push) {
                                    stack_hist.push(stack);
                                }

                                PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, pos+offs, NULL, false);
                                except->init();
                                blocks.push(except);
                            }
                        } else {
                            fprintf(stderr, "Something TERRIBLE happened!!\n");
                        }
                        prev = nil;
                    } else {
                        prev = nil;
                    }

                } while (prev != nil);

                if (!blocks.empty()) {
                    curblock = blocks.top();
                    if (curblock->blktype() == ASTBlock::BLK_EXCEPT)
                        curblock->setEnd(endpos);
                }
            }
            break;
        case Pyc::LIST_APPEND:
        case Pyc::LIST_APPEND_A:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                PycRef<ASTNode> list = stack.top();


                if (curblock->blktype() == ASTBlock::BLK_FOR
                        && curblock.cast<ASTIterBlock>()->isComprehension()) {
                    if (comp_saved > 0) {
                        /* PEP 709 inline comprehension: the operand counts
                           the value plus one iterator slot per `for` clause.
                           The loop state is [.., accumulator, iter] with the
                           iterator on top; replace the accumulator with the
                           result node and put the iterators back on top. */
                        std::vector<PycRef<ASTNode>> iters;
                        for (int i = 0; i < operand - 1; i++) {
                            iters.push_back(stack.top());
                            stack.pop();
                        }
                        stack.pop();
                        stack.push(new ASTComprehension(value, comp_kind));
                        for (int i = (int)iters.size() - 1; i >= 0; i--)
                            stack.push(iters[i]);
                        comp_restoring = false;
                    } else {
                        stack.pop();
                        stack.push(new ASTComprehension(value));
                    }
                } else {
                    stack.push(new ASTSubscr(list, value)); /* Total hack */
                }
            }
            break;
        case Pyc::SET_ADD_A:
            {
                /* Python 3.12+: inline set comprehension (PEP 709). */
                if (curblock->blktype() == ASTBlock::BLK_FOR
                        && curblock.cast<ASTIterBlock>()->isComprehension()
                        && comp_saved > 0) {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    std::vector<PycRef<ASTNode>> iters;
                    for (int i = 0; i < operand - 1; i++) {
                        iters.push_back(stack.top());
                        stack.pop();
                    }
                    stack.pop();
                    stack.push(new ASTComprehension(value, ASTComprehension::COMP_SET));
                    for (int i = (int)iters.size() - 1; i >= 0; i--)
                        stack.push(iters[i]);
                    comp_restoring = false;
                } else {
                    fprintf(stderr, "Unsupported SET_ADD outside of a comprehension\n");
                }
            }
            break;
        case Pyc::MAP_ADD_A:
            {
                /* Python 3.12+: inline dict comprehension (PEP 709).
                   MAP_ADD pops value (TOS) and key (TOS1). */
                if (curblock->blktype() == ASTBlock::BLK_FOR
                        && curblock.cast<ASTIterBlock>()->isComprehension()
                        && comp_saved > 0) {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    std::vector<PycRef<ASTNode>> iters;
                    for (int i = 0; i < operand - 1; i++) {
                        iters.push_back(stack.top());
                        stack.pop();
                    }
                    stack.pop();
                    stack.push(new ASTComprehension(value, ASTComprehension::COMP_MAP, key));
                    for (int i = (int)iters.size() - 1; i >= 0; i--)
                        stack.push(iters[i]);
                    comp_restoring = false;
                } else {
                    fprintf(stderr, "Unsupported MAP_ADD outside of a comprehension\n");
                }
            }
            break;
        case Pyc::SET_UPDATE_A:
            {
                PycRef<ASTNode> rhs = stack.top();
                stack.pop();
                PycRef<ASTSet> lhs = stack.top().cast<ASTSet>();
                stack.pop();

                if (rhs.type() != ASTNode::NODE_OBJECT) {
                    fprintf(stderr, "Unsupported argument found for SET_UPDATE\n");
                    break;
                }

                // I've only ever seen this be a TYPE_FROZENSET, but let's be careful...
                PycRef<PycObject> obj = rhs.cast<ASTObject>()->object();
                if (obj->type() != PycObject::TYPE_FROZENSET) {
                    fprintf(stderr, "Unsupported argument type found for SET_UPDATE\n");
                    break;
                }

                ASTSet::value_t result = lhs->values();
                for (const auto& it : obj.cast<PycSet>()->values()) {
                    result.push_back(new ASTObject(it));
                }

                stack.push(new ASTSet(result));
            }
            break;
        case Pyc::LIST_EXTEND_A:
            {
                PycRef<ASTNode> rhs = stack.top();
                stack.pop();
                PycRef<ASTList> lhs = stack.top().cast<ASTList>();
                stack.pop();

                if (rhs.type() != ASTNode::NODE_OBJECT) {
                    fprintf(stderr, "Unsupported argument found for LIST_EXTEND\n");
                    break;
                }

                // I've only ever seen this be a SMALL_TUPLE, but let's be careful...
                PycRef<PycObject> obj = rhs.cast<ASTObject>()->object();
                if (obj->type() != PycObject::TYPE_TUPLE && obj->type() != PycObject::TYPE_SMALL_TUPLE) {
                    fprintf(stderr, "Unsupported argument type found for LIST_EXTEND\n");
                    break;
                }

                ASTList::value_t result = lhs->values();
                for (const auto& it : obj.cast<PycTuple>()->values()) {
                    result.push_back(new ASTObject(it));
                }

                stack.push(new ASTList(result));
            }
            break;
        case Pyc::LOAD_ATTR_A:
            {
                PycRef<ASTNode> name = stack.top();
                if (name.type() != ASTNode::NODE_IMPORT) {
                    stack.pop();

                    if (mod->verCompare(3, 12) >= 0) {
                        if (operand & 1) {
                            /* Changed in version 3.12:
                            If the low bit of name is set, then a NULL or self is pushed to the stack
                            before the attribute or unbound method respectively. */
                            stack.push(nullptr);
                        }
                        operand >>= 1;
                    }

                    stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
                }
            }
            break;
        case Pyc::LOAD_BUILD_CLASS:
            stack.push(new ASTLoadBuildClass(new PycObject()));
            break;
        case Pyc::LOAD_CLOSURE_A:
            /* Ignore this */
            break;
        case Pyc::LOAD_CONST_A:
            {
                PycRef<ASTObject> t_ob = new ASTObject(code->getConst(operand));

                if ((t_ob->object().type() == PycObject::TYPE_TUPLE ||
                        t_ob->object().type() == PycObject::TYPE_SMALL_TUPLE) &&
                        !t_ob->object().cast<PycTuple>()->values().size()) {
                    ASTTuple::value_t values;
                    stack.push(new ASTTuple(values));
                } else if (t_ob->object().type() == PycObject::TYPE_NONE) {
                    stack.push(NULL);
                } else {
                    stack.push(t_ob.cast<ASTNode>());
                }
            }
            break;
        case Pyc::LOAD_DEREF_A:
        case Pyc::LOAD_CLASSDEREF_A:
            stack.push(new ASTName(code->getCellVar(mod, operand)));
            break;
        case Pyc::LOAD_FAST_A:
            if (mod->verCompare(1, 3) < 0)
                stack.push(new ASTName(code->getName(operand)));
            else
                stack.push(new ASTName(code->getLocal(operand)));
            break;
        case Pyc::LOAD_FAST_LOAD_FAST_A:
            stack.push(new ASTName(code->getLocal(operand >> 4)));
            stack.push(new ASTName(code->getLocal(operand & 0xF)));
            break;
        case Pyc::LOAD_FAST_BORROW_A:
            /* Python 3.14: like LOAD_FAST (a borrow is an optimization
               hint only). */
            stack.push(new ASTName(code->getLocal(operand)));
            break;
        case Pyc::LOAD_FAST_CHECK_A:
            /* Python 3.12+: like LOAD_FAST but checks the local is bound
               (used for the __exit__ lookup in with statements). */
            stack.push(new ASTName(code->getLocal(operand)));
            break;
        case Pyc::LOAD_FAST_BORROW_LOAD_FAST_BORROW_A:
            /* Python 3.14: like LOAD_FAST_LOAD_FAST. */
            stack.push(new ASTName(code->getLocal(operand >> 4)));
            stack.push(new ASTName(code->getLocal(operand & 0xF)));
            break;
        case Pyc::LOAD_FAST_AND_CLEAR_A:
            /* Python 3.12+: comprehension helper - pushes the (possibly
               unset) local; the old value is restored after the loop.  The
               pushed placeholder is NULL; the epilogue's SWAP/STORE_FAST
               pair consumes it again. */
            stack.push(nullptr);
            /* PEP 709: detect the inline-comprehension prologue that
               follows (LOAD_FAST_AND_CLEAR*; SWAP; BUILD_LIST/BUILD_SET/
               BUILD_MAP 0; SWAP; [GET_ITER]; FOR_ITER) so the loop is
               treated as a comprehension rather than a for statement, and
               count the local as awaiting its epilogue restore. */
            {
                PycBuffer scan = source;
                int scanpos = pos;
                int sop, sopc;
                int stage = 0; /* 0: LFACs, 1: SWAP, 2: BUILD_* 0, 3: SWAP */
                bool found = false;
                while (!scan.atEof()) {
                    bc_next(scan, mod, sop, sopc, scanpos);
                    if (stage == 0) {
                        if (sop == Pyc::LOAD_FAST_AND_CLEAR_A)
                            continue;
                        if (sop == Pyc::SWAP_A) {
                            stage = 1;
                            continue;
                        }
                        break;
                    } else if (stage == 1) {
                        if (sopc == 0 && (sop == Pyc::BUILD_LIST_A
                                    || sop == Pyc::BUILD_SET_A
                                    || sop == Pyc::BUILD_MAP_A)) {
                            comp_kind = (sop == Pyc::BUILD_SET_A)
                                    ? ASTComprehension::COMP_SET
                                    : (sop == Pyc::BUILD_MAP_A)
                                    ? ASTComprehension::COMP_MAP
                                    : ASTComprehension::COMP_LIST;
                            stage = 2;
                            continue;
                        }
                        break;
                    } else if (stage == 2) {
                        if (sop == Pyc::SWAP_A) {
                            stage = 3;
                            continue;
                        }
                        break;
                    } else {
                        /* stage 3: an optional duplicate GET_ITER (3.13)
                           may precede the loop's FOR_ITER. */
                        if (sop == Pyc::GET_ITER)
                            continue;
                        found = (sop == Pyc::FOR_ITER_A);
                        break;
                    }
                }
                if (found) {
                    comp_saved++;
                    comp_restoring = false;
                }
            }
            break;
        case Pyc::STORE_FAST_LOAD_FAST_A:
            {
                /* Python 3.13+: store TOS into locals[A>>4], then load
                   locals[A&0xF]. */
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = new ASTName(code->getLocal(operand >> 4));
                if (comp_restoring && comp_saved > 0) {
                    /* PEP 709 epilogue: silent restore of a saved local. */
                    if (--comp_saved <= 0) {
                        comp_saved = 0;
                        comp_restoring = false;
                    }
                } else if (curblock->blktype() == ASTBlock::BLK_FOR
                        && !curblock->inited()) {
                    /* First store of an inline-comprehension loop target. */
                    curblock.cast<ASTIterBlock>()->setIndex(dest);
                } else if (curblock->blktype() == ASTBlock::BLK_WITH
                        && !curblock->inited()) {
                    curblock.cast<ASTWithBlock>()->setExpr(value);
                } else if (value != nullptr && !value->processed()) {
                    curblock->append(new ASTStore(value, dest));
                }
                stack.push(new ASTName(code->getLocal(operand & 0xF)));
            }
            break;
        case Pyc::STORE_FAST_STORE_FAST_A:
            {
                /* Python 3.13+: store TOS into locals[A>>4] and TOS1 into
                   locals[A&0xF].  In inline dict/set comprehensions this
                   unpacks the loop's (key, value) pairs. */
                for (int half = 0; half < 2; half++) {
                    int local = (half == 0) ? (operand >> 4) : (operand & 0xF);

                    if (unpack) {
                        /* Same tuple-unpack protocol as STORE_FAST: the
                           UNPACK_SEQUENCE left a tuple placeholder on the
                           stack; collect both names into it. */
                        PycRef<ASTNode> name = new ASTName(code->getLocal(local));
                        PycRef<ASTNode> tup = stack.top();
                        if (tup.type() == ASTNode::NODE_TUPLE)
                            tup.cast<ASTTuple>()->add(name);
                        else
                            fputs("Something TERRIBLE happened!\n", stderr);

                        if (--unpack <= 0) {
                            stack.pop();
                            PycRef<ASTNode> seq = stack.top();
                            stack.pop(); /* NULL pushed by FOR_ITER */

                            if (curblock->blktype() == ASTBlock::BLK_FOR
                                    && !curblock->inited()) {
                                PycRef<ASTTuple> tuple = tup.try_cast<ASTTuple>();
                                if (tuple != NULL)
                                    tuple->setRequireParens(false);
                                curblock.cast<ASTIterBlock>()->setIndex(tup);
                            } else if (seq && seq.type() == ASTNode::NODE_CHAINSTORE) {
                                append_to_chain_store(seq, tup, stack, curblock);
                            } else {
                                curblock->append(new ASTStore(seq, tup));
                            }
                        }
                        continue;
                    }

                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> name = new ASTName(code->getLocal(local));

                    if (comp_restoring && comp_saved > 0 && value == nullptr) {
                        /* PEP 709 epilogue: silent restore of a saved local. */
                        if (--comp_saved <= 0) {
                            comp_saved = 0;
                            comp_restoring = false;
                        }
                        continue;
                    }

                    if (name.cast<ASTName>()->name()->value()[0] == '_'
                            && name.cast<ASTName>()->name()->value()[1] == '[') {
                        /* Don't show stores of list comp append objects. */
                        continue;
                    }

                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && !curblock->inited()) {
                        curblock.cast<ASTIterBlock>()->setIndex(name);
                    } else if (curblock->blktype() == ASTBlock::BLK_WITH
                            && !curblock->inited()) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                    } else if (value != nullptr && !value->processed()) {
                        curblock->append(new ASTStore(value, name));
                    }
                }
            }
            break;
        case Pyc::LOAD_SMALL_INT_A:
            /* Python 3.14: loads a small integer constant (0..255). */
            stack.push(new ASTObject(new PycInt(operand)));
            break;
        case Pyc::LOAD_COMMON_CONSTANT_A:
            /* Python 3.14: 0 = AssertionError, 1 = NotImplementedError. */
            if (operand == 1)
                stack.push(make_name("NotImplementedError"));
            else
                stack.push(make_name("AssertionError"));
            break;
        case Pyc::LOAD_ASSERTION_ERROR:
            /* Python 3.9+: push AssertionError for bare assert/raise. */
            stack.push(make_name("AssertionError"));
            break;
        case Pyc::LOAD_GLOBAL_A:
            if (mod->verCompare(3, 11) >= 0) {
                // Loads the global named co_names[namei>>1] onto the stack.
                if (operand & 1) {
                    /* Changed in version 3.11: 
                    If the low bit of "NAMEI" (operand) is set, 
                    then a NULL is pushed to the stack before the global variable. */
                    stack.push(nullptr);
                }
                operand >>= 1;
            }
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::LOAD_LOCALS:
            stack.push(new ASTNode(ASTNode::NODE_LOCALS));
            break;
        case Pyc::STORE_LOCALS:
            stack.pop();
            break;
        case Pyc::LOAD_METHOD_A:
            {
                // Behave like LOAD_ATTR
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
            }
            break;
        case Pyc::LOAD_NAME_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::MAKE_CLOSURE_A:
        case Pyc::MAKE_FUNCTION_A:
            {
                PycRef<ASTNode> fun_code = stack.top();
                stack.pop();

                /* Test for the qualified name of the function (at TOS) */
                int tos_type = fun_code.cast<ASTObject>()->object().type();
                if (tos_type != PycObject::TYPE_CODE &&
                    tos_type != PycObject::TYPE_CODE2) {
                    fun_code = stack.top();
                    stack.pop();
                }

                ASTFunction::defarg_t defArgs, kwDefArgs;
                const int defCount = operand & 0xFF;
                const int kwDefCount = (operand >> 8) & 0xFF;
                for (int i = 0; i < defCount; ++i) {
                    defArgs.push_front(stack.top());
                    stack.pop();
                }
                for (int i = 0; i < kwDefCount; ++i) {
                    kwDefArgs.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTFunction(fun_code, defArgs, kwDefArgs));
            }
            break;
        case Pyc::MAKE_FUNCTION:
            {
                /* Python 3.13+: MAKE_FUNCTION takes no operand; defaults
                   and kwdefaults are attached afterwards through
                   SET_FUNCTION_ATTRIBUTE.  The qualified name sits on
                   top of the code object. */
                PycRef<ASTNode> fun_code = stack.top();
                stack.pop();

                int tos_type = fun_code.type() == ASTNode::NODE_OBJECT
                        ? fun_code.cast<ASTObject>()->object().type() : 0;
                if (tos_type != PycObject::TYPE_CODE &&
                    tos_type != PycObject::TYPE_CODE2) {
                    /* Drop the qualified name string. */
                    fun_code = stack.top();
                    stack.pop();
                    tos_type = fun_code.type() == ASTNode::NODE_OBJECT
                            ? fun_code.cast<ASTObject>()->object().type() : 0;
                }

                ASTFunction::defarg_t defArgs, kwDefArgs;
                stack.push(new ASTFunction(fun_code, defArgs, kwDefArgs));
            }
            break;
        case Pyc::SET_FUNCTION_ATTRIBUTE_A:
            {
                /* Python 3.13+: attach an attribute to the function object
                   created by the preceding MAKE_FUNCTION.  Stack shape is
                   [.., attr, func] - the attribute sits UNDER the function.
                     1 = defaults tuple, 2 = kwdefaults map,
                     4 = annotations, 8 = closure. */
                /* NOTE: FastStack::top(1) is TOS itself; top(2) is the
                   element under it. */
                PycRef<ASTNode> func = stack.top();
                PycRef<ASTNode> attr = stack.top(2);
                stack.pop();
                stack.pop();
                stack.push(func);
                if (func.type() == ASTNode::NODE_FUNCTION) {
                    PycRef<ASTFunction> fn = func.cast<ASTFunction>();
                    if (operand == 1) {
                        /* defaults: a tuple constant (LOAD_CONST of tuple)
                           or an ASTTuple built on the stack. */
                        if (attr.type() == ASTNode::NODE_TUPLE) {
                            for (const auto& v : attr.cast<ASTTuple>()->values())
                                fn->addDefault(v);
                        } else if (attr.type() == ASTNode::NODE_OBJECT
                                && (attr.cast<ASTObject>()->object().type() == PycObject::TYPE_TUPLE
                                 || attr.cast<ASTObject>()->object().type() == PycObject::TYPE_SMALL_TUPLE)) {
                            for (const auto& v : attr.cast<ASTObject>()
                                        ->object().cast<PycTuple>()->values())
                                fn->addDefault(new ASTObject(v));
                        } else if (attr != nullptr) {
                            fn->addDefault(attr);
                        }
                    } else if (operand == 2) {
                        /* kwdefaults: BUILD_CONST_KEY_MAP yields an
                           ASTConstMap; a dict constant yields an ASTObject
                           (PycDict) or an ASTMap.  Values align positionally
                           with the keyword-only parameters. */
                        if (attr.type() == ASTNode::NODE_CONST_MAP) {
                            /* BUILD_CONST_KEY_MAP pops values top-down, so
                               they are stored in reverse parameter order. */
                            const auto& vals = attr.cast<ASTConstMap>()->values();
                            for (auto it = vals.rbegin(); it != vals.rend(); ++it)
                                fn->addKWDefault(*it);
                        } else if (attr.type() == ASTNode::NODE_MAP) {
                            /* BUILD_MAP stores pairs in source order,
                               matching the parameter order. */
                            const auto& vals = attr.cast<ASTMap>()->values();
                            for (const auto& kv : vals)
                                fn->addKWDefault(kv.second);
                        } else if (attr.type() == ASTNode::NODE_OBJECT
                                && attr.cast<ASTObject>()->object().type() == PycObject::TYPE_DICT) {
                            for (const auto& kv : attr.cast<ASTObject>()
                                        ->object().cast<PycDict>()->values())
                                fn->addKWDefault(new ASTObject(std::get<1>(kv)));
                        } else if (attr != nullptr) {
                            fn->addKWDefault(attr);
                        }
                    }
                    /* annotations (4) and closure (8) are not represented */
                }
            }
            break;
        case Pyc::NOP:
            break;
        case Pyc::POP_BLOCK:
            {
                if (curblock->blktype() == ASTBlock::BLK_CONTAINER ||
                        curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    /* These should only be popped by an END_FINALLY */
                    break;
                }

                if (curblock->blktype() == ASTBlock::BLK_WITH) {
                    // This should only be popped by a WITH_CLEANUP
                    break;
                }

                if (curblock->nodes().size() &&
                        curblock->nodes().back().type() == ASTNode::NODE_KEYWORD) {
                    curblock->removeLast();
                }

                if (curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELIF
                        || curblock->blktype() == ASTBlock::BLK_ELSE
                        || curblock->blktype() == ASTBlock::BLK_TRY
                        || curblock->blktype() == ASTBlock::BLK_EXCEPT
                        || curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    if (!stack_hist.empty()) {
                        stack = stack_hist.top();
                        stack_hist.pop();
                    } else {
                        fprintf(stderr, "Warning: Stack history is empty, something wrong might have happened\n");
                    }
                }
                PycRef<ASTBlock> tmp = curblock;
                blocks.pop();

                if (!blocks.empty())
                    curblock = blocks.top();

                if (!(tmp->blktype() == ASTBlock::BLK_ELSE
                        && tmp->nodes().size() == 0)) {
                    curblock->append(tmp.cast<ASTNode>());
                }

                if (tmp->blktype() == ASTBlock::BLK_FOR && tmp->end() >= pos) {
                    stack_hist.push(stack);

                    PycRef<ASTBlock> blkelse = new ASTBlock(ASTBlock::BLK_ELSE, tmp->end());
                    blocks.push(blkelse);
                    curblock = blocks.top();
                }

                if (curblock->blktype() == ASTBlock::BLK_TRY
                        && tmp->blktype() != ASTBlock::BLK_FOR
                        && tmp->blktype() != ASTBlock::BLK_ASYNCFOR
                        && tmp->blktype() != ASTBlock::BLK_WHILE) {
                    stack = stack_hist.top();
                    stack_hist.pop();

                    tmp = curblock;
                    blocks.pop();
                    curblock = blocks.top();

                    if (!(tmp->blktype() == ASTBlock::BLK_ELSE
                            && tmp->nodes().size() == 0)) {
                        curblock->append(tmp.cast<ASTNode>());
                    }
                }

                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();

                    if (tmp->blktype() == ASTBlock::BLK_ELSE && !cont->hasFinally()) {

                        /* Pop the container */
                        blocks.pop();
                        curblock = blocks.top();
                        curblock->append(cont.cast<ASTNode>());

                    } else if ((tmp->blktype() == ASTBlock::BLK_ELSE && cont->hasFinally())
                            || (tmp->blktype() == ASTBlock::BLK_TRY && !cont->hasExcept())) {

                        /* Add the finally block */
                        stack_hist.push(stack);

                        PycRef<ASTBlock> final = new ASTBlock(ASTBlock::BLK_FINALLY, 0, true);
                        blocks.push(final);
                        curblock = blocks.top();
                    }
                }

                if ((curblock->blktype() == ASTBlock::BLK_FOR || curblock->blktype() == ASTBlock::BLK_ASYNCFOR)
                        && curblock->end() == pos) {
                    blocks.pop();
                    blocks.top()->append(curblock.cast<ASTNode>());
                    curblock = blocks.top();
                }
            }
            break;
        case Pyc::POP_EXCEPT:
            /* Do nothing. */
            break;
        case Pyc::PUSH_EXC_INFO:
            /* Python 3.11+: pushes exception info tuple. We ignore here to keep decompilation going. */
            break;
        case Pyc::CHECK_EXC_MATCH:
            {
                /* Python 3.11+: compares exception against handler type. */
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTCompare(left, right, ASTCompare::CMP_EXCEPTION));
            }
            break;
        case Pyc::CHECK_EG_MATCH:
            {
                /* Python 3.11+: compares exception group against handler
                   type (except*). */
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTCompare(left, right, ASTCompare::CMP_EXCEPTION));
            }
            break;
        case Pyc::END_FOR:
            {
                stack.pop();

                if ((opcode == Pyc::END_FOR) && (mod->majorVer() == 3) && (mod->minorVer() == 12)
                        && comp_saved == 0) {
                    // one additional pop for python 3.12
                    stack.pop();
                }

                // end for loop here
                /* TODO : Ensure that FOR loop ends here. 
                   Due to CACHE instructions at play, the end indicated in
                   the for loop by pycdas is not correct, it is off by
                   some small amount. */
                if (curblock->blktype() == ASTBlock::BLK_FOR) {
                    PycRef<ASTBlock> prev = blocks.top();
                    blocks.pop();

                    if (prev.cast<ASTIterBlock>()->isComprehension() && comp_saved > 0) {
                        /* PEP 709: fold this loop into the comprehension node
                           that is still on the stack instead of emitting a
                           for statement.  Scan a few slots for it; the top of
                           stack may hold other active iterators. */
                        for (int depth = 1; depth <= 5; depth++) {
                            PycRef<ASTNode> probe = stack.top(depth);
                            if (probe.type() == ASTNode::NODE_COMPREHENSION) {
                                probe.cast<ASTComprehension>()->addGenerator(
                                        prev.cast<ASTIterBlock>());
                                break;
                            }
                        }
                        curblock = blocks.top();
                        /* Epilogue follows: SWAP + STORE_FAST restore the
                           cleared locals; mark that for the STORE handlers. */
                        comp_restoring = true;
                    } else {
                        curblock = blocks.top();
                        curblock->append(prev.cast<ASTNode>());

                        /* A recorded break target beyond the loop end
                           delimits a for-else arm: [end, elseEnd).  Open a
                           block for it so the arm renders attached to the
                           loop instead of running unconditionally after
                           it.  The block closes when decoding reaches
                           elseEnd. */
                        int eend = prev.cast<ASTIterBlock>()->elseEnd();
                        if (eend > pos) {
                            PycRef<ASTBlock> blkelse = new ASTBlock(ASTBlock::BLK_ELSE, eend);
                            blkelse->init();
                            blocks.push(blkelse.cast<ASTBlock>());
                            curblock = blocks.top();
                        }
                    }
                }
                else if (comp_saved > 0) {
                    /* PEP 709 inline comprehension: the loop block was
                       already closed at its JUMP_BACKWARD, or this is an
                       inner `for` clause's iterator being discarded. */
                    comp_restoring = true;
                }
                else {
                    fprintf(stderr, "Wrong block type %i for END_FOR\n", curblock->blktype());
                }
            }
            break;
        case Pyc::POP_TOP:
            {
                if (comp_saved > 0) {
                    /* Python 3.13 inline-comprehension epilogue: END_FOR
                       already discarded the iterator; this POP_TOP is the
                       iterator-slot cleanup and must not eat the result. */
                    break;
                }

                PycRef<ASTNode> value = stack.top();
                stack.pop();

                if (value != nullptr) {
                    /* Python 3.12+: FOR_ITER keeps the loop iterator on
                       the stack until END_FOR.  A POP_TOP inside the loop
                       body that surfaces it (e.g. the dead stack-balance
                       cleanup after a raise that closes an if block and
                       restores an older stack snapshot) must not emit the
                       iterator expression again. */
                    bool isIterator = false;
                    std::stack<PycRef<ASTBlock>> scan = blocks;
                    while (!scan.empty()) {
                        PycRef<ASTBlock> b = scan.top();
                        scan.pop();
                        if (b->blktype() != ASTBlock::BLK_FOR
                                && b->blktype() != ASTBlock::BLK_ASYNCFOR)
                            continue;
                        PycRef<ASTIterBlock> fb = b.try_cast<ASTIterBlock>();
                        if (fb != nullptr && fb->iter() == value) {
                            isIterator = true;
                            break;
                        }
                    }
                    if (isIterator)
                        break;
                }

                if (value.type() == ASTNode::NODE_BINARY
                        && value.cast<ASTBinary>()->op() == ASTBinary::BIN_ATTR
                        && value.cast<ASTBinary>()->right().type() == ASTNode::NODE_NAME) {
                    /* A plain POP_TOP consumed the exit marker of a `with`
                       epilogue (e.g. the loop-iterator unwinding POP of a
                       break path).  Drop the marker and the NULL that was
                       pushed below it. */
                    PycRef<PycString> mn = value.cast<ASTBinary>()->right()
                            .cast<ASTName>()->name();
                    if (mn->isEqual("__exit__") || mn->isEqual("__aexit__")) {
                        break;
                    }
                }

                /* Python 3.11+: end of a `with` body.  The value popped is
                   the marker call for the implicit __exit__ epilogue, which
                   was left by the CALL handler.  The epilogue has no source
                   representation, so it is dropped. */
                if (value.type() == ASTNode::NODE_CALL
                        && value.cast<ASTCall>()->func().type() == ASTNode::NODE_BINARY) {
                    PycRef<ASTBinary> fn = value.cast<ASTCall>()->func().cast<ASTBinary>();
                    if (fn->op() == ASTBinary::BIN_ATTR
                            && fn->right().type() == ASTNode::NODE_NAME) {
                        PycRef<PycString> nm = fn->right().cast<ASTName>()->name();
                        if (nm->isEqual("__exit__") || nm->isEqual("__aexit__")) {
                            /* Close the innermost open `with` block, closing
                               any conditional blocks still open above it (the
                               epilogue belongs to a break/return/continue
                               path, so those inner blocks have ended). */
                            PycRef<ASTBlock> with;
                            {
                                std::stack<PycRef<ASTBlock>> scan = blocks;
                                while (!scan.empty()) {
                                    PycRef<ASTBlock> blk = scan.top();
                                    if (blk->blktype() == ASTBlock::BLK_WITH
                                            && blk->inited()) {
                                        with = blk;
                                        break;
                                    }
                                    scan.pop();
                                }
                            }
                            if (with != nullptr) {
                                while (blocks.top() != with) {
                                    PycRef<ASTBlock> inner = blocks.top();
                                    blocks.pop();
                                    blocks.top()->append(inner.cast<ASTNode>());
                                }
                                blocks.pop();
                                curblock = blocks.top();
                                curblock->append(with.cast<ASTNode>());
                            } else {
                                /* Duplicate exit site of an already-closed
                                   `with`: discard the (NULL, exit-mark) pair
                                   it left on the stack. */
                                PycRef<ASTNode> below = stack.top(1);
                                if (below != nullptr
                                        && below.type() == ASTNode::NODE_BINARY
                                        && below.cast<ASTBinary>()->op() == ASTBinary::BIN_ATTR
                                        && below.cast<ASTBinary>()->right().type() == ASTNode::NODE_NAME) {
                                    PycRef<PycString> mn = below.cast<ASTBinary>()
                                            ->right().cast<ASTName>()->name();
                                    if (mn->isEqual("__exit__") || mn->isEqual("__aexit__")) {
                                        if (stack.top(2) == nullptr)
                                            stack.pop();
                                        stack.pop();
                                    }
                                }
                            }
                            break;
                        }
                    }
                }

                if (!curblock->inited()) {
                    if (curblock->blktype() == ASTBlock::BLK_WITH) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                    } else {
                        curblock->init();
                    }
                    break;
                } else if (value == nullptr || value->processed()) {
                    break;
                }

                curblock->append(value);

                if (curblock->blktype() == ASTBlock::BLK_FOR
                        && curblock.cast<ASTIterBlock>()->isComprehension()) {
                    /* This relies on some really uncertain logic...
                     * If it's a comprehension, the only POP_TOP should be
                     * a call to append the iter to the list.
                     */
                    if (value.type() == ASTNode::NODE_CALL) {
                        auto& pparams = value.cast<ASTCall>()->pparams();
                        if (!pparams.empty()) {
                            PycRef<ASTNode> res = pparams.front();
                            stack.push(new ASTComprehension(res));
                        }
                    }
                }
            }
            break;
        case Pyc::PRINT_ITEM:
            {
                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().try_cast<ASTPrint>();
                if (printNode && printNode->stream() == nullptr && !printNode->eol())
                    printNode->add(stack.top());
                else
                    curblock->append(new ASTPrint(stack.top()));
                stack.pop();
            }
            break;
        case Pyc::PRINT_ITEM_TO:
            {
                PycRef<ASTNode> stream = stack.top();
                stack.pop();

                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().try_cast<ASTPrint>();
                if (printNode && printNode->stream() == stream && !printNode->eol())
                    printNode->add(stack.top());
                else
                    curblock->append(new ASTPrint(stack.top(), stream));
                stack.pop();
                if (stream)
                    stream->setProcessed();
            }
            break;
        case Pyc::PRINT_NEWLINE:
            {
                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().try_cast<ASTPrint>();
                if (printNode && printNode->stream() == nullptr && !printNode->eol())
                    printNode->setEol(true);
                else
                    curblock->append(new ASTPrint(nullptr));
                stack.pop();
            }
            break;
        case Pyc::PRINT_NEWLINE_TO:
            {
                PycRef<ASTNode> stream = stack.top();
                stack.pop();

                PycRef<ASTPrint> printNode;
                if (curblock->size() > 0 && curblock->nodes().back().type() == ASTNode::NODE_PRINT)
                    printNode = curblock->nodes().back().try_cast<ASTPrint>();
                if (printNode && printNode->stream() == stream && !printNode->eol())
                    printNode->setEol(true);
                else
                    curblock->append(new ASTPrint(nullptr, stream));
                stack.pop();
                if (stream)
                    stream->setProcessed();
            }
            break;
        case Pyc::RAISE_VARARGS_A:
            {
                ASTRaise::param_t paramList;
                for (int i = 0; i < operand; i++) {
                    paramList.push_front(stack.top());
                    stack.pop();
                }
                curblock->append(new ASTRaise(paramList));

                if ((curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELSE)
                        && stack_hist.size()
                        && (mod->verCompare(2, 6) >= 0)) {
                    stack = stack_hist.top();
                    stack_hist.pop();

                    PycRef<ASTBlock> prev = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    curblock->append(prev.cast<ASTNode>());
                }
            }
            break;
        case Pyc::RERAISE:
        case Pyc::RERAISE_A:
            /* Python 3.11 cleanup opcode. */
            if (operand == 0 && blocks.size() > 1) {
                /* RERAISE 0 ends an except chain - no handler matched, so
                   the exception propagates.  Close the finished handler
                   (and its container when no finally follows). */
                if (curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    /* The finally-propagation arm ends in RERAISE; the
                       finally arm is the container's last arm, so close
                       the container as well. */
                    PycRef<ASTBlock> prev = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    if (!stack_hist.empty())
                        stack_hist.pop();
                    if (prev->size() != 0)
                        curblock->append(prev.cast<ASTNode>());
                    if (curblock->blktype() == ASTBlock::BLK_CONTAINER
                            && blocks.size() > 1) {
                        PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                        blocks.pop();
                        curblock = blocks.top();
                        if (!stack_hist.empty())
                            stack_hist.pop();
                        if (!cont->mounted())
                            curblock->append(cont.cast<ASTNode>());
                    }
                }
                if (curblock->blktype() == ASTBlock::BLK_EXCEPT) {
                    PycRef<ASTBlock> prev = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    if (!stack_hist.empty())
                        stack_hist.pop();
                    if (prev->size() != 0)
                        curblock->append(prev.cast<ASTNode>());
                }
                if (curblock->blktype() == ASTBlock::BLK_CONTAINER
                        && blocks.size() > 1) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    /* A finally-bearing container stays open for its
                       finally arm - unless the arm's propagation region
                       was already swallowed by the handler skip that ends
                       here, in which case the container is finished. */
                    if (!cont->hasFinally() || pos >= cont->finally()) {
                        blocks.pop();
                        curblock = blocks.top();
                        if (!stack_hist.empty())
                            stack_hist.pop();
                        /* Mounted (deferred in-loop) containers already sit
                           in their parent; appending again would duplicate
                           them. */
                        if (!cont->mounted())
                            curblock->append(cont.cast<ASTNode>());
                    }
                }
            }
            break;
        case Pyc::RETURN_VALUE:
        case Pyc::INSTRUMENTED_RETURN_VALUE_A:
            {
                /* Safety reset: any inline-comprehension bookkeeping must
                   not leak past the end of the code object. */
                comp_saved = 0;
                comp_restoring = false;
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                curblock->append(new ASTReturn(value));

                if ((curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELSE)
                        && stack_hist.size()
                        && (mod->verCompare(2, 6) >= 0)) {
                    stack = stack_hist.top();
                    stack_hist.pop();

                    PycRef<ASTBlock> prev = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    curblock->append(prev.cast<ASTNode>());

                    /* Old bytecode emits a (dead) jump after a return that
                       ends an if/else block; skip it.  When the following
                       instruction is real code (e.g. 3.14 compiles a
                       constant return as LOAD + RETURN_VALUE), keep it. */
                    PycBuffer saved_src = source;
                    int saved_pos = pos;
                    if (!source.atEof()) {
                        bc_next(source, mod, opcode, operand, pos);
                        if (opcode != Pyc::JUMP_FORWARD_A
                                && opcode != Pyc::JUMP_ABSOLUTE_A) {
                            source = saved_src;
                            pos = saved_pos;
                        }
                    }

                    /* A `while True:` whose structural exit was decoded as
                       the trailing if's else arm ends with that arm's
                       terminator; close the loop block here. */
                    if (while_else_exit
                            && curblock->blktype() == ASTBlock::BLK_WHILE
                            && curblock->end() == 0) {
                        PycRef<ASTBlock> tmp = curblock;
                        blocks.pop();
                        curblock = blocks.top();
                        curblock->append(tmp.cast<ASTNode>());
                        while_else_exit = false;
                    }
                }
            }
            break;
        case Pyc::RETURN_CONST_A:
        case Pyc::INSTRUMENTED_RETURN_CONST_A:
            {
                /* Safety reset: any inline-comprehension bookkeeping must
                   not leak past the end of the code object. */
                comp_saved = 0;
                comp_restoring = false;
                PycRef<ASTObject> value = new ASTObject(code->getConst(operand));
                curblock->append(new ASTReturn(value.cast<ASTNode>()));

                if ((curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELSE)
                        && stack_hist.size()
                        && (mod->verCompare(2, 6) >= 0)) {
                    stack = stack_hist.top();
                    stack_hist.pop();

                    PycRef<ASTBlock> prev = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    curblock->append(prev.cast<ASTNode>());

                    /* Old bytecode emits a (dead) jump after a return that
                       ends an if/else block; skip it.  When the following
                       instruction is real code (e.g. the fallthrough
                       return of an if/else in 3.12+), keep it. */
                    PycBuffer saved_src = source;
                    int saved_pos = pos;
                    if (!source.atEof()) {
                        bc_next(source, mod, opcode, operand, pos);
                        if (opcode != Pyc::JUMP_FORWARD_A
                                && opcode != Pyc::JUMP_ABSOLUTE_A) {
                            source = saved_src;
                            pos = saved_pos;
                        }
                    }
                }
            }
            break;
        case Pyc::ROT_TWO:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> two = stack.top();
                stack.pop();

                stack.push(one);
                stack.push(two);
            }
            break;
        case Pyc::ROT_THREE:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
                PycRef<ASTNode> two = stack.top();
                stack.pop();
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> three = stack.top();
                stack.pop();
                stack.push(one);
                stack.push(three);
                stack.push(two);
            }
            break;
        case Pyc::ROT_FOUR:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
                PycRef<ASTNode> two = stack.top();
                stack.pop();
                PycRef<ASTNode> three = stack.top();
                stack.pop();
                if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                    stack.pop();
                }
                PycRef<ASTNode> four = stack.top();
                stack.pop();
                stack.push(one);
                stack.push(four);
                stack.push(three);
                stack.push(two);
            }
            break;
        case Pyc::SET_LINENO_A:
            // Ignore
            break;
        case Pyc::SETUP_WITH_A:
        case Pyc::WITH_EXCEPT_START:
            {
                PycRef<ASTBlock> withblock = new ASTWithBlock(pos+operand);
                blocks.push(withblock);
                curblock = blocks.top();
            }
            break;
        case Pyc::BEFORE_WITH:
            /* Python 3.11 - 3.13: performs __exit__/__enter__ on the context
               manager on TOS.  Model it by opening a with block, leaving a
               bound-__exit__ marker for the epilogue call, and re-pushing
               the context manager for the following STORE/POP_TOP (the
               __enter__ result in the bytecode is the manager object used
               by the STORE). */
            if (mod->verCompare(3, 11) >= 0 && mod->verCompare(3, 14) < 0) {
                PycRef<ASTNode> mgr = stack.top();
                stack.pop();

                PycRef<ASTBlock> withblock = new ASTWithBlock(0);
                blocks.push(withblock.cast<ASTBlock>());
                curblock = blocks.top();

                stack.push(nullptr);
                stack.push(new ASTBinary(mgr, make_name("__exit__"),
                        ASTBinary::BIN_ATTR));
                stack.push(mgr);
            }
            break;
        case Pyc::BEFORE_ASYNC_WITH:
            /* Python 3.11 - 3.13: same as BEFORE_WITH but async: TOS is an
               awaitable of (exit, result) which is immediately awaited. */
            if (mod->verCompare(3, 11) >= 0 && mod->verCompare(3, 14) < 0) {
                PycRef<ASTNode> mgr = stack.top();
                stack.pop();

                PycRef<ASTBlock> withblock = new ASTWithBlock(0);
                blocks.push(withblock.cast<ASTBlock>());
                curblock = blocks.top();

                stack.push(nullptr);
                PycRef<ASTNode> exitmark = new ASTBinary(mgr,
                        make_name("__aexit__"), ASTBinary::BIN_ATTR);
                stack.push(exitmark);

                /* Skip the await loop: GET_AWAITABLE; LOAD_CONST None; SEND;
                   YIELD; RESUME; JUMP_BACKWARD_NO_INTERRUPT; END_SEND. */
                do {
                    bc_next(source, mod, opcode, operand, pos);
                } while (opcode != Pyc::END_SEND && !source.atEof());

                stack.push(mgr);
            }
            break;
        case Pyc::LOAD_SPECIAL_A:
            {
                /* Python 3.14: with-statements no longer use BEFORE_WITH.
                   Entry sequence on [mgr, mgr]:
                     COPY 1; LOAD_SPECIAL 1 (__exit__); SWAP 2; SWAP 3;
                     LOAD_SPECIAL 0/2 (__enter__/__aenter__); CALL 0
                   (async entries are followed by an await loop through
                   END_SEND). */
                if (mod->verCompare(3, 14) >= 0) {
                    if (operand == 1 || operand == 3) {
                        bool async_with = (operand == 3);
                        const char* exitname = async_with ? "__aexit__" : "__exit__";

                        /* The COPY 1 duplicate of the manager is on TOS;
                           discard it and open the with block. */
                        stack.pop();
                        PycRef<ASTNode> mgr = stack.top();
                        stack.pop();

                        PycRef<ASTBlock> withblock = new ASTWithBlock(0);
                        blocks.push(withblock.cast<ASTBlock>());
                        curblock = blocks.top();

                        stack.push(nullptr);
                        stack.push(new ASTBinary(mgr, make_name(exitname),
                                ASTBinary::BIN_ATTR));

                        /* Skip: SWAP 2; SWAP 3; LOAD_SPECIAL __enter__;
                           CALL 0 (plus CACHE entries). */
                        for (int i = 0; i < 4 && !source.atEof(); i++)
                            bc_next(source, mod, opcode, operand, pos);
                        if (async_with) {
                            /* Skip the await loop through END_SEND. */
                            while (opcode != Pyc::END_SEND && !source.atEof())
                                bc_next(source, mod, opcode, operand, pos);
                        }

                        stack.push(mgr);
                    } else {
                        /* __enter__/__aenter__ used outside a with entry. */
                        PycRef<ASTNode> mgr = stack.top();
                        stack.pop();
                        const char* entername = (operand == 2) ? "__aenter__" : "__enter__";
                        stack.push(new ASTBinary(mgr, make_name(entername),
                                ASTBinary::BIN_ATTR));
                    }
                }
            }
            break;
        case Pyc::WITH_CLEANUP:
        case Pyc::WITH_CLEANUP_START:
            {
                // Stack top should be a None. Ignore it.
                PycRef<ASTNode> none = stack.top();
                stack.pop();

                if (none != NULL) {
                    fprintf(stderr, "Something TERRIBLE happened!\n");
                    break;
                }

                if (curblock->blktype() == ASTBlock::BLK_WITH
                        && curblock->end() == curpos) {
                    PycRef<ASTBlock> with = curblock;
                    blocks.pop();
                    curblock = blocks.top();
                    curblock->append(with.cast<ASTNode>());
                }
                else {
                    fprintf(stderr, "Something TERRIBLE happened! No matching with block found for WITH_CLEANUP at %d\n", curpos);
                }
            }
            break;
        case Pyc::WITH_CLEANUP_FINISH:
            /* Ignore this */
            break;
        case Pyc::SETUP_EXCEPT_A:
            {
                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    curblock.cast<ASTContainerBlock>()->setExcept(pos+operand);
                } else {
                    PycRef<ASTBlock> next = new ASTContainerBlock(0, pos+operand);
                    blocks.push(next.cast<ASTBlock>());
                }

                /* Store the current stack for the except/finally statement(s) */
                stack_hist.push(stack);
                PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY, pos+operand, true);
                blocks.push(tryblock.cast<ASTBlock>());
                curblock = blocks.top();

                need_try = false;
            }
            break;
        case Pyc::SETUP_FINALLY_A:
            {
                PycRef<ASTBlock> next = new ASTContainerBlock(pos+operand);
                blocks.push(next.cast<ASTBlock>());
                curblock = blocks.top();

                need_try = true;
            }
            break;
        case Pyc::SETUP_LOOP_A:
            {
                PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_WHILE, pos+operand, NULL, false);
                blocks.push(next.cast<ASTBlock>());
                curblock = blocks.top();
            }
            break;
        case Pyc::SLICE_0:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE0);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::SLICE_1:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE1, lower);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::SLICE_2:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE2, NULL, upper);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::SLICE_3:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE3, lower, upper);
                stack.push(new ASTSubscr(name, slice));
            }
            break;
        case Pyc::STORE_ATTR_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name = stack.top();
                    stack.pop();
                    PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(attr);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();
                        if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> name = stack.top();
                    stack.pop();
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);
                    if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, attr, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, attr));
                    }
                }
            }
            break;
        case Pyc::STORE_DEREF_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name = new ASTName(code->getCellVar(mod, operand));

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> name = new ASTName(code->getCellVar(mod, operand));

                    if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, name));
                    }
                }
            }
            break;
        case Pyc::STORE_FAST_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name;

                    if (mod->verCompare(1, 3) < 0)
                        name = new ASTName(code->getName(operand));
                    else
                        name = new ASTName(code->getLocal(operand));

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            PycRef<ASTTuple> tuple = tup.try_cast<ASTTuple>();
                            if (tuple != NULL)
                                tuple->setRequireParens(false);
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> name;

                    if (mod->verCompare(1, 3) < 0)
                        name = new ASTName(code->getName(operand));
                    else
                        name = new ASTName(code->getLocal(operand));

                    if (name.cast<ASTName>()->name()->value()[0] == '_'
                            && name.cast<ASTName>()->name()->value()[1] == '[') {
                        /* Don't show stores of list comp append objects. */
                        break;
                    }

                    if (comp_restoring && comp_saved > 0 && value == nullptr) {
                        /* PEP 709 inline-comprehension epilogue: silently
                           discard the saved local's NULL placeholder. */
                        if (--comp_saved <= 0) {
                            comp_saved = 0;
                            comp_restoring = false;
                        }
                        break;
                    }

                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && !curblock->inited()) {
                        curblock.cast<ASTIterBlock>()->setIndex(name);
                    } else if (curblock->blktype() == ASTBlock::BLK_WITH
                                   && !curblock->inited()) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                        curblock.cast<ASTWithBlock>()->setVar(name);
                    } else if (mod->verCompare(3, 11) >= 0
                            && value == nullptr
                            && last_op == Pyc::LOAD_CONST_A) {
                        /* Python 3.11+ except-var cleanup: `LOAD_CONST None;
                           STORE_FAST x; DELETE_FAST x` unbinds the exception
                           variable when the handler exits.  Only that exact
                           pair is consumed; a plain `x = None` assignment
                           (no matching delete) keeps its store. */
                        PycBuffer probe = source;
                        int npos = pos, nop, noper;
                        bc_next(probe, mod, nop, noper, npos);
                        if (nop == Pyc::DELETE_FAST_A && noper == operand) {
                            source = probe;
                            pos = npos;
                        } else {
                            curblock->append(new ASTStore(value, name));
                        }
                        break;
                    } else if (curblock->blktype() == ASTBlock::BLK_EXCEPT
                            && curblock->size() == 0 && !unpack
                            && mod->verCompare(3, 11) >= 0
                            && curblock.cast<ASTCondBlock>()->var() == NULL) {
                        /* Python 3.11+ `except ... as e:` - the first store
                           after the handler entry binds the variable. */
                        curblock.cast<ASTCondBlock>()->setVar(name);
                    } else if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else if (comp_saved > 0
                            && curblock->blktype() == ASTBlock::BLK_FOR
                            && curblock.cast<ASTIterBlock>()->isComprehension()) {
                        /* Assignment (walrus) inside an inline
                           comprehension: the stored value was duplicated by
                           COPY and this STORE consumes the duplicate.  Drop
                           the original too and keep the walrus expression on
                           the stack so the append consumes it. */
                        stack.pop();
                        auto store = new ASTStore(value, name);
                        store->setWalrus(true);
                        stack.push(store);
                    } else if (mod->verCompare(3, 12) >= 0
                            && last_op == Pyc::COPY_A && last_operand == 1) {
                        /* Named expression (PEP 572) outside an inlined
                           comprehension: `COPY 1; STORE x` stores the
                           duplicate while the original remains on the
                           stack as the value of `(x := ...)`.  Replace
                           that original with the walrus node. */
                        stack.pop();
                        auto store = new ASTStore(value, name);
                        store->setWalrus(true);
                        stack.push(store);
                    } else {
                        curblock->append(new ASTStore(value, name));
                    }
                }
            }
            break;
        case Pyc::STORE_GLOBAL_A:
            {
                PycRef<ASTNode> name = new ASTName(code->getName(operand));

                if (unpack) {
                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            PycRef<ASTTuple> tuple = tup.try_cast<ASTTuple>();
                            if (tuple != NULL)
                                tuple->setRequireParens(false);
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, name));
                    }
                }

                /* Mark the global as used */
                code->markGlobal(name.cast<ASTName>()->name());
            }
            break;
        case Pyc::STORE_NAME_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name = new ASTName(code->getName(operand));

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(name);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            PycRef<ASTTuple> tuple = tup.try_cast<ASTTuple>();
                            if (tuple != NULL)
                                tuple->setRequireParens(false);
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();

                    PycRef<PycString> varname = code->getName(operand);
                    if (varname->length() >= 2 && varname->value()[0] == '_'
                            && varname->value()[1] == '[') {
                        /* Don't show stores of list comp append objects. */
                        break;
                    }

                    // Return private names back to their original name
                    const std::string class_prefix = std::string("_") + code->name()->strValue();
                    if (varname->startsWith(class_prefix + std::string("__")))
                        varname->setValue(varname->strValue().substr(class_prefix.size()));

                    PycRef<ASTNode> name = new ASTName(varname);

                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && !curblock->inited()) {
                        curblock.cast<ASTIterBlock>()->setIndex(name);
                    } else if (stack.top().type() == ASTNode::NODE_IMPORT) {
                        PycRef<ASTImport> import = stack.top().cast<ASTImport>();

                        import->add_store(new ASTStore(value, name));
                    } else if (curblock->blktype() == ASTBlock::BLK_WITH
                               && !curblock->inited()) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                        curblock.cast<ASTWithBlock>()->setVar(name);
                    } else if (value.type() == ASTNode::NODE_CHAINSTORE) {
                        append_to_chain_store(value, name, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(value, name));

                        if (value.type() == ASTNode::NODE_INVALID)
                            break;
                    }
                }
            }
            break;
        case Pyc::STORE_SLICE_0:
            {
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE0))));
            }
            break;
        case Pyc::STORE_SLICE_1:
            {
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE1, upper))));
            }
            break;
        case Pyc::STORE_SLICE_2:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE2, NULL, lower))));
            }
            break;
        case Pyc::STORE_SLICE_3:
            {
                PycRef<ASTNode> lower = stack.top();
                stack.pop();
                PycRef<ASTNode> upper = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE3, upper, lower))));
            }
            break;
        case Pyc::STORE_SUBSCR:
            {
                if (unpack) {
                    PycRef<ASTNode> subscr = stack.top();
                    stack.pop();
                    PycRef<ASTNode> dest = stack.top();
                    stack.pop();

                    PycRef<ASTNode> save = new ASTSubscr(dest, subscr);

                    PycRef<ASTNode> tup = stack.top();
                    if (tup.type() == ASTNode::NODE_TUPLE)
                        tup.cast<ASTTuple>()->add(save);
                    else
                        fputs("Something TERRIBLE happened!\n", stderr);

                    if (--unpack <= 0) {
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();
                        if (seq.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(seq, tup, stack, curblock);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> subscr = stack.top();
                    stack.pop();
                    PycRef<ASTNode> dest = stack.top();
                    stack.pop();
                    PycRef<ASTNode> src = stack.top();
                    stack.pop();

                    // If variable annotations are enabled, we'll need to check for them here.
                    // Python handles a varaible annotation by setting:
                    // __annotations__['var-name'] = type
                    const bool found_annotated_var = (variable_annotations && dest->type() == ASTNode::Type::NODE_NAME
                                                      && dest.cast<ASTName>()->name()->isEqual("__annotations__"));

                    if (found_annotated_var) {
                        // Annotations can be done alone or as part of an assignment.
                        // In the case of an assignment, we'll see a NODE_STORE on the stack.
                        if (!curblock->nodes().empty() && curblock->nodes().back()->type() == ASTNode::Type::NODE_STORE) {
                            // Replace the existing NODE_STORE with a new one that includes the annotation.
                            PycRef<ASTStore> store = curblock->nodes().back().cast<ASTStore>();
                            curblock->removeLast();
                            curblock->append(new ASTStore(store->src(),
                                                          new ASTAnnotatedVar(subscr, src)));
                        } else {
                            curblock->append(new ASTAnnotatedVar(subscr, src));
                        }
                    } else {
                        if (dest.type() == ASTNode::NODE_MAP) {
                            dest.cast<ASTMap>()->add(subscr, src);
                        } else if (src.type() == ASTNode::NODE_CHAINSTORE) {
                            append_to_chain_store(src, new ASTSubscr(dest, subscr), stack, curblock);
                        } else {
                            curblock->append(new ASTStore(src, new ASTSubscr(dest, subscr)));
                        }
                    }
                }
            }
            break;
        case Pyc::UNARY_CALL:
            {
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                stack.push(new ASTCall(func, ASTCall::pparam_t(), ASTCall::kwparam_t()));
            }
            break;
        case Pyc::UNARY_CONVERT:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTConvert(name));
            }
            break;
        case Pyc::UNARY_INVERT:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_INVERT));
            }
            break;
        case Pyc::UNARY_NEGATIVE:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NEGATIVE));
            }
            break;
        case Pyc::UNARY_NOT:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NOT));
            }
            break;
        case Pyc::UNARY_POSITIVE:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_POSITIVE));
            }
            break;
        case Pyc::UNPACK_LIST_A:
        case Pyc::UNPACK_TUPLE_A:
        case Pyc::UNPACK_SEQUENCE_A:
            {
                unpack = operand;
                if (unpack > 0) {
                    ASTTuple::value_t vals;
                    stack.push(new ASTTuple(vals));
                } else {
                    // Unpack zero values and assign it to top of stack or for loop variable.
                    // E.g. [] = TOS / for [] in X
                    ASTTuple::value_t vals;
                    auto tup = new ASTTuple(vals);
                    if (curblock->blktype() == ASTBlock::BLK_FOR
                        && !curblock->inited()) {
                        tup->setRequireParens(true);
                        curblock.cast<ASTIterBlock>()->setIndex(tup);
                    } else if (stack.top().type() == ASTNode::NODE_CHAINSTORE) {
                        auto chainStore = stack.top();
                        stack.pop();
                        append_to_chain_store(chainStore, tup, stack, curblock);
                    } else {
                        curblock->append(new ASTStore(stack.top(), tup));
                        stack.pop();
                    }
                }
            }
            break;
        case Pyc::YIELD_FROM:
            {
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                // TODO: Support yielding into a non-null destination
                PycRef<ASTNode> value = stack.top();
                if (value) {
                    value->setProcessed();
                    curblock->append(new ASTReturn(value, ASTReturn::YIELD_FROM));
                }
            }
            break;
        case Pyc::YIELD_VALUE:
        case Pyc::YIELD_VALUE_A:
        case Pyc::INSTRUMENTED_YIELD_VALUE_A:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                if (value == nullptr) {
                    /* None sent by a SEND loop inside await machinery;
                       keep it for the following END_SEND. */
                    stack.push(nullptr);
                    break;
                }
                curblock->append(new ASTReturn(value, ASTReturn::YIELD));
                /* Python 3.11+ generators: the sent value (None on entry)
                   is pushed by the interpreter for the RESUME/POP_TOP that
                   follows; the POP_TOP discards it without a source
                   representation.  Older versions have no such pair. */
                if (mod->verCompare(3, 11) >= 0)
                    stack.push(nullptr);
            }
            break;
        case Pyc::SETUP_ANNOTATIONS:
            variable_annotations = true;
            break;
        case Pyc::PRECALL_A:
        case Pyc::RESUME_A:
        case Pyc::INSTRUMENTED_RESUME_A:
            /* We just entirely ignore this / no-op */
            break;
        case Pyc::MAKE_CELL_A:
            /* Python 3.11+: initialises a cell variable before execution;
               no stack effect and no source representation. */
            break;
        case Pyc::COPY_FREE_VARS_A:
            /* Python 3.11+: copies closure variables into the frame; no
               stack effect and no source representation. */
            break;
        case Pyc::RETURN_GENERATOR:
            /* Python 3.11+: generator/async prologue.  Pops the None
               argument and pushes the generator/coroutine object, which
               the immediately following POP_TOP discards; leaving the
               stack untouched has the same net effect. */
            break;
        case Pyc::CALL_INTRINSIC_1_A:
            /* Python 3.12+: runtime helper intrinsics.  Most are only
               reachable from interpreter tails that are never rendered
               (e.g. INTRINSIC_STOPITERATION_ERROR); treat as a value-
               preserving no-op so those tails decompile cleanly. */
            break;
        case Pyc::CLEANUP_THROW:
            /* Python 3.13+: await-machinery handler that unwinds a
               coroutine suspended at a YIELD inside a SEND loop; it is
               always followed by a backward jump merging back into the
               (already emitted) END_SEND.  Dead at the source level. */
            {
                PycBuffer peek = source;
                int peekpos = pos;
                int nop, nopc;
                bc_next(peek, mod, nop, nopc, peekpos);
                if (nop == Pyc::JUMP_BACKWARD_NO_INTERRUPT_A)
                    bc_next(source, mod, opcode, operand, pos);
            }
            break;
        case Pyc::TO_BOOL:
            /* Python 3.13+: coerce TOS to bool for a conditional jump;
               the expression itself is unchanged. */
            break;
        case Pyc::NOT_TAKEN:
            /* Python 3.14+: marker for a not-taken specialized branch. */
            break;
        case Pyc::POP_ITER:
            /* Python 3.14+: discard the iterator after a comprehension or
               for loop. */
            if (comp_saved > 0) {
                /* PEP 709 inline comprehension: END_FOR already discarded
                   the iterator; keep the accumulator on the stack. */
                break;
            }
            stack.pop();
            break;
        case Pyc::CACHE:
            /* These "fake" opcodes are used as placeholders for optimizing
               certain opcodes in Python 3.11+.  Since we have no need for
               that during disassembly/decompilation, we can just treat these
               as no-ops. */
            break;
        case Pyc::PUSH_NULL:
            stack.push(nullptr);
            break;
        case Pyc::GEN_START_A:
            stack.pop();
            break;
        case Pyc::SWAP_A:
            {
                if (comp_saved > 0) {                    /* PEP 709 inline comprehension: SWAP exchanges TOS with
                       the operand-th item below it (SWAP 2 swaps the top
                       two stack entries).  This juggles the saved locals,
                       the accumulator and the iterator slots in place. */
                    PycRef<ASTNode> tos = stack.top();
                    stack.pop();
                    std::vector<PycRef<ASTNode>> middle;
                    for (int i = 0; i < operand - 2; i++) {
                        middle.push_back(stack.top());
                        stack.pop();
                    }
                    PycRef<ASTNode> other = stack.top();
                    stack.pop();
                    stack.push(tos);
                    for (int i = (int)middle.size() - 1; i >= 0; i--)
                        stack.push(middle[i]);
                    stack.push(other);
                    break;
                }
                if (operand == 2) {
                    /* Python 3.12+ chained comparison: "SWAP 2; COPY 2"
                       reorders the first two operands and duplicates the
                       midpoint so each segment compares against it.  The
                       stack ends up as [mid, lhs, mid], ready for the
                       first COMPARE_OP.  Consuming both instructions here
                       avoids misreading them as tuple unpacking. */
                    {
                        PycBuffer probe = source;
                        int npos = pos, nop, noper;
                        bc_next(probe, mod, nop, noper, npos);
                        while (nop == Pyc::CACHE && !probe.atEof())
                            bc_next(probe, mod, nop, noper, npos);
                        if (mod->verCompare(3, 12) >= 0 && nop == Pyc::COPY_A
                                && noper == 2) {
                            PycRef<ASTNode> rhs = stack.top();
                            stack.pop();
                            PycRef<ASTNode> lhs = stack.top();
                            stack.pop();
                            /* SWAP 2 yields [mid, lhs]; COPY 2 duplicates the
                               midpoint below the top, giving [mid, lhs, mid] */
                            stack.push(rhs);
                            stack.push(lhs);
                            stack.push(rhs);
                            source = probe;
                            pos = npos;
                            break;
                        }
                    }
                    /* Python 3.12+ loop-exit idiom: "SWAP 2; POP_TOP"
                       before a RETURN keeps the value being returned while
                       dropping the iterator underneath it.  Consuming both
                       instructions here avoids misreading them as tuple
                       unpacking. */
                    PycBuffer probe = source;
                    int npos = pos, nop, noper;
                    bc_next(probe, mod, nop, noper, npos);
                    if (nop == Pyc::POP_TOP) {
                        PycRef<ASTNode> tos = stack.top();
                        stack.pop();
                        stack.pop();
                        stack.push(tos);
                        source = probe;
                        pos = npos;
                        break;
                    }
                    /* Python 3.11+ except-handler exit: "SWAP 2; POP_EXCEPT"
                       moves the handler body's result above the exception
                       entry and drops the latter.  Consuming both keeps the
                       result for the trailing RETURN_VALUE. */
                    if (mod->verCompare(3, 11) >= 0 && nop == Pyc::POP_EXCEPT) {
                        PycRef<ASTNode> tos = stack.top();
                        stack.pop();
                        stack.pop();
                        stack.push(tos);
                        source = probe;
                        pos = npos;
                        break;
                    }
                }
                unpack = operand;
                ASTTuple::value_t values;
                ASTTuple::value_t next_tuple;
                values.resize(operand);
                for (int i = 0; i < operand; i++) {
                    values[operand - i - 1] = stack.top();
                    stack.pop();
                }
                auto tup = new ASTTuple(values);
                tup->setRequireParens(false);
                auto next_tup = new ASTTuple(next_tuple);
                next_tup->setRequireParens(false);
                stack.push(tup);
                stack.push(next_tup);
            }
            break;
        case Pyc::BINARY_SLICE:
            {
                PycRef<ASTNode> end = stack.top();
                stack.pop();
                PycRef<ASTNode> start = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();

                if (start.type() == ASTNode::NODE_OBJECT
                        && start.cast<ASTObject>()->object() == Pyc_None) {
                    start = NULL;
                }

                if (end.type() == ASTNode::NODE_OBJECT
                        && end.cast<ASTObject>()->object() == Pyc_None) {
                    end = NULL;
                }

                PycRef<ASTNode> slice;
                if (start == NULL && end == NULL) {
                    slice = new ASTSlice(ASTSlice::SLICE0);
                } else if (start == NULL) {
                    slice = new ASTSlice(ASTSlice::SLICE2, start, end);
                } else if (end == NULL) {
                    slice = new ASTSlice(ASTSlice::SLICE1, start, end);
                } else {
                    slice = new ASTSlice(ASTSlice::SLICE3, start, end);
                }
                stack.push(new ASTSubscr(dest, slice));
            }
            break;
        case Pyc::STORE_SLICE:
            {
                PycRef<ASTNode> end = stack.top();
                stack.pop();
                PycRef<ASTNode> start = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> values = stack.top();
                stack.pop();

                if (start.type() == ASTNode::NODE_OBJECT
                        && start.cast<ASTObject>()->object() == Pyc_None) {
                    start = NULL;
                }

                if (end.type() == ASTNode::NODE_OBJECT
                        && end.cast<ASTObject>()->object() == Pyc_None) {
                    end = NULL;
                }

                PycRef<ASTNode> slice;
                if (start == NULL && end == NULL) {
                    slice = new ASTSlice(ASTSlice::SLICE0);
                } else if (start == NULL) {
                    slice = new ASTSlice(ASTSlice::SLICE2, start, end);
                } else if (end == NULL) {
                    slice = new ASTSlice(ASTSlice::SLICE1, start, end);
                } else {
                    slice = new ASTSlice(ASTSlice::SLICE3, start, end);
                }

                curblock->append(new ASTStore(values, new ASTSubscr(dest, slice)));
            }
            break;
        case Pyc::COPY_A:
            {
                PycRef<ASTNode> value = stack.top(operand);
                stack.push(value);
            }
            break;
        default:
            fprintf(stderr, "Unsupported opcode: %s (%d)\n", Pyc::OpcodeName(opcode), opcode);
            cleanBuild = false;
            return new ASTNodeList(defblock->nodes());
        }

        else_pop =  ( (curblock->blktype() == ASTBlock::BLK_ELSE)
                      || (curblock->blktype() == ASTBlock::BLK_IF)
                      || (curblock->blktype() == ASTBlock::BLK_ELIF) )
                 && (curblock->end() == pos);
    }

    if (stack_hist.size()) {
        fputs("Warning: Stack history is not empty!\n", stderr);

        while (stack_hist.size()) {
            stack_hist.pop();
        }
    }

    if (blocks.size() > 1 && !while_else_exit) {
        fputs("Warning: block stack is not empty!\n", stderr);

        while (blocks.size() > 1) {
            PycRef<ASTBlock> tmp = blocks.top();
            blocks.pop();

            /* A mounted container (deferred in-loop handler) is already
               present in its parent block; appending it again would
               duplicate its statements at the function level. */
            if (tmp->blktype() == ASTBlock::BLK_CONTAINER
                    && tmp.cast<ASTContainerBlock>()->mounted())
                continue;

            blocks.top()->append(tmp.cast<ASTNode>());
        }
    }

    cleanBuild = true;
    return new ASTNodeList(defblock->nodes());
}

static void append_to_chain_store(const PycRef<ASTNode> &chainStore,
        PycRef<ASTNode> item, FastStack& stack, const PycRef<ASTBlock>& curblock)
{
    stack.pop();    // ignore identical source object.
    chainStore.cast<ASTChainStore>()->append(item);
    if (stack.top().type() == PycObject::TYPE_NULL) {
        curblock->append(chainStore);
    } else {
        stack.push(chainStore);
    }
}

static int cmp_prec(PycRef<ASTNode> parent, PycRef<ASTNode> child)
{
    /* Determine whether the parent has higher precedence than therefore
       child, so we don't flood the source code with extraneous parens.
       Else we'd have expressions like (((a + b) + c) + d) when therefore
       equivalent, a + b + c + d would suffice. */

    if (parent.type() == ASTNode::NODE_UNARY && parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT)
        return 1;   // Always parenthesize not(x)
    if (child.type() == ASTNode::NODE_BINARY) {
        PycRef<ASTBinary> binChild = child.cast<ASTBinary>();
        if (parent.type() == ASTNode::NODE_BINARY) {
            PycRef<ASTBinary> binParent = parent.cast<ASTBinary>();
            if (binParent->right() == child) {
                if (binParent->op() == ASTBinary::BIN_SUBTRACT &&
                    binChild->op() == ASTBinary::BIN_ADD)
                    return 1;
                else if (binParent->op() == ASTBinary::BIN_DIVIDE &&
                         binChild->op() == ASTBinary::BIN_MULTIPLY)
                    return 1;
            }
            return binChild->op() - binParent->op();
        }
        else if (parent.type() == ASTNode::NODE_COMPARE)
            return (binChild->op() == ASTBinary::BIN_LOG_AND ||
                    binChild->op() == ASTBinary::BIN_LOG_OR) ? 1 : -1;
        else if (parent.type() == ASTNode::NODE_UNARY)
            return (binChild->op() == ASTBinary::BIN_POWER) ? -1 : 1;
    } else if (child.type() == ASTNode::NODE_UNARY) {
        PycRef<ASTUnary> unChild = child.cast<ASTUnary>();
        if (parent.type() == ASTNode::NODE_BINARY) {
            PycRef<ASTBinary> binParent = parent.cast<ASTBinary>();
            if (binParent->op() == ASTBinary::BIN_LOG_AND ||
                binParent->op() == ASTBinary::BIN_LOG_OR)
                return -1;
            else if (unChild->op() == ASTUnary::UN_NOT)
                return 1;
            else if (binParent->op() == ASTBinary::BIN_POWER)
                return 1;
            else
                return -1;
        } else if (parent.type() == ASTNode::NODE_COMPARE) {
            return (unChild->op() == ASTUnary::UN_NOT) ? 1 : -1;
        } else if (parent.type() == ASTNode::NODE_UNARY) {
            return unChild->op() - parent.cast<ASTUnary>()->op();
        }
    } else if (child.type() == ASTNode::NODE_COMPARE) {
        PycRef<ASTCompare> cmpChild = child.cast<ASTCompare>();
        if (parent.type() == ASTNode::NODE_BINARY)
            return (parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_AND ||
                    parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_OR) ? -1 : 1;
        else if (parent.type() == ASTNode::NODE_COMPARE)
            return cmpChild->op() - parent.cast<ASTCompare>()->op();
        else if (parent.type() == ASTNode::NODE_UNARY)
            return (parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT) ? -1 : 1;
    }

    /* For normal nodes, don't parenthesize anything */
    return -1;
}

static void print_ordered(PycRef<ASTNode> parent, PycRef<ASTNode> child,
                          PycModule* mod, std::ostream& pyc_output)
{
    if (child.type() == ASTNode::NODE_BINARY ||
        child.type() == ASTNode::NODE_COMPARE) {
        if (cmp_prec(parent, child) > 0) {
            pyc_output << "(";
            print_src(child, mod, pyc_output);
            pyc_output << ")";
        } else {
            print_src(child, mod, pyc_output);
        }
    } else if (child.type() == ASTNode::NODE_UNARY) {
        if (cmp_prec(parent, child) > 0) {
            pyc_output << "(";
            print_src(child, mod, pyc_output);
            pyc_output << ")";
        } else {
            print_src(child, mod, pyc_output);
        }
    } else {
        print_src(child, mod, pyc_output);
    }
}

static void start_line(int indent, std::ostream& pyc_output)
{
    if (inLambda)
        return;
    for (int i=0; i<indent; i++)
        pyc_output << "    ";
}

static void end_line(std::ostream& pyc_output)
{
    if (inLambda)
        return;
    pyc_output << "\n";
}

int cur_indent = -1;
static void print_block(PycRef<ASTBlock> blk, PycModule* mod,
                        std::ostream& pyc_output)
{
    ASTBlock::list_t lines = blk->nodes();

    if (lines.size() == 0) {
        PycRef<ASTNode> pass = new ASTKeyword(ASTKeyword::KW_PASS);
        start_line(cur_indent, pyc_output);
        print_src(pass, mod, pyc_output);
    }

    for (auto ln = lines.cbegin(); ln != lines.cend();) {
        bool isContainer = (*ln).cast<ASTNode>().type() == ASTNode::NODE_BLOCK
                && (*ln).cast<ASTBlock>()->blktype() == ASTBlock::BLK_CONTAINER;
        /* Separate a try statement from the preceding statement with a
           blank line, but avoid a leading blank line when it opens the
           block.  The container prints its own line starts. */
        if (isContainer) {
            if (ln != lines.cbegin())
                pyc_output << "\n";
        } else if ((*ln).cast<ASTNode>().type() != ASTNode::NODE_NODELIST) {
            start_line(cur_indent, pyc_output);
        }
        print_src(*ln, mod, pyc_output);
        if (++ln != lines.end()) {
            end_line(pyc_output);
        }
    }
}

void print_formatted_value(PycRef<ASTFormattedValue> formatted_value, PycModule* mod,
                           std::ostream& pyc_output)
{
    pyc_output << "{";
    print_src(formatted_value->val(), mod, pyc_output);

    switch (formatted_value->conversion() & ASTFormattedValue::CONVERSION_MASK) {
    case ASTFormattedValue::NONE:
        break;
    case ASTFormattedValue::STR:
        pyc_output << "!s";
        break;
    case ASTFormattedValue::REPR:
        pyc_output << "!r";
        break;
    case ASTFormattedValue::ASCII:
        pyc_output << "!a";
        break;
    }
    if (formatted_value->conversion() & ASTFormattedValue::HAVE_FMT_SPEC) {
        pyc_output << ":" << formatted_value->format_spec().cast<ASTObject>()->object().cast<PycString>()->value();
    }
    pyc_output << "}";
}

static std::unordered_set<ASTNode *> node_seen;

void print_src(PycRef<ASTNode> node, PycModule* mod, std::ostream& pyc_output)
{
    if (node == NULL) {
        pyc_output << "None";
        cleanBuild = true;
        return;
    }

    if (node_seen.find((ASTNode *)node) != node_seen.end()) {
        fputs("WARNING: Circular reference detected\n", stderr);
        return;
    }
    node_seen.insert((ASTNode *)node);

    switch (node->type()) {
    case ASTNode::NODE_BINARY:
    case ASTNode::NODE_COMPARE:
        {
            PycRef<ASTBinary> bin = node.cast<ASTBinary>();
            print_ordered(node, bin->left(), mod, pyc_output);
            pyc_output << bin->op_str();
            print_ordered(node, bin->right(), mod, pyc_output);
        }
        break;
    case ASTNode::NODE_UNARY:
        {
            PycRef<ASTUnary> un = node.cast<ASTUnary>();
            pyc_output << un->op_str();
            print_ordered(node, un->operand(), mod, pyc_output);
        }
        break;
    case ASTNode::NODE_CALL:
        {
            PycRef<ASTCall> call = node.cast<ASTCall>();
            print_src(call->func(), mod, pyc_output);
            pyc_output << "(";
            bool first = true;
            for (const auto& param : call->pparams()) {
                if (!first)
                    pyc_output << ", ";
                print_src(param, mod, pyc_output);
                first = false;
            }
            for (const auto& param : call->kwparams()) {
                if (!first)
                    pyc_output << ", ";
                if (param.first.type() == ASTNode::NODE_NAME) {
                    pyc_output << param.first.cast<ASTName>()->name()->value() << " = ";
                } else {
                    PycRef<PycString> str_name = param.first.cast<ASTObject>()->object().cast<PycString>();
                    pyc_output << str_name->value() << " = ";
                }
                print_src(param.second, mod, pyc_output);
                first = false;
            }
            if (call->hasVar()) {
                if (!first)
                    pyc_output << ", ";
                pyc_output << "*";
                print_src(call->var(), mod, pyc_output);
                first = false;
            }
            if (call->hasKW()) {
                if (!first)
                    pyc_output << ", ";
                pyc_output << "**";
                print_src(call->kw(), mod, pyc_output);
                first = false;
            }
            pyc_output << ")";
        }
        break;
    case ASTNode::NODE_DELETE:
        {
            pyc_output << "del ";
            print_src(node.cast<ASTDelete>()->value(), mod, pyc_output);
        }
        break;
    case ASTNode::NODE_EXEC:
        {
            PycRef<ASTExec> exec = node.cast<ASTExec>();
            pyc_output << "exec ";
            print_src(exec->statement(), mod, pyc_output);

            if (exec->globals() != NULL) {
                pyc_output << " in ";
                print_src(exec->globals(), mod, pyc_output);

                if (exec->locals() != NULL
                        && exec->globals() != exec->locals()) {
                    pyc_output << ", ";
                    print_src(exec->locals(), mod, pyc_output);
                }
            }
        }
        break;
    case ASTNode::NODE_FORMATTEDVALUE:
        pyc_output << "f" F_STRING_QUOTE;
        print_formatted_value(node.cast<ASTFormattedValue>(), mod, pyc_output);
        pyc_output << F_STRING_QUOTE;
        break;
    case ASTNode::NODE_JOINEDSTR:
        pyc_output << "f" F_STRING_QUOTE;
        for (const auto& val : node.cast<ASTJoinedStr>()->values()) {
            switch (val.type()) {
            case ASTNode::NODE_FORMATTEDVALUE:
                print_formatted_value(val.cast<ASTFormattedValue>(), mod, pyc_output);
                break;
            case ASTNode::NODE_OBJECT:
                // When printing a piece of the f-string, keep the quote style consistent.
                // This avoids problems when ''' or """ is part of the string.
                print_const(pyc_output, val.cast<ASTObject>()->object(), mod, F_STRING_QUOTE);
                break;
            default:
                fprintf(stderr, "Unsupported node type %d in NODE_JOINEDSTR\n", val.type());
            }
        }
        pyc_output << F_STRING_QUOTE;
        break;
    case ASTNode::NODE_KEYWORD:
        pyc_output << node.cast<ASTKeyword>()->word_str();
        break;
    case ASTNode::NODE_LIST:
        {
            pyc_output << "[";
            bool first = true;
            cur_indent++;
            for (const auto& val : node.cast<ASTList>()->values()) {
                if (first)
                    pyc_output << "\n";
                else
                    pyc_output << ",\n";
                start_line(cur_indent, pyc_output);
                print_src(val, mod, pyc_output);
                first = false;
            }
            cur_indent--;
            pyc_output << "]";
        }
        break;
    case ASTNode::NODE_SET:
        {
            pyc_output << "{";
            bool first = true;
            cur_indent++;
            for (const auto& val : node.cast<ASTSet>()->values()) {
                if (first)
                    pyc_output << "\n";
                else
                    pyc_output << ",\n";
                start_line(cur_indent, pyc_output);
                print_src(val, mod, pyc_output);
                first = false;
            }
            cur_indent--;
            pyc_output << "}";
        }
        break;
    case ASTNode::NODE_COMPREHENSION:
        {
            PycRef<ASTComprehension> comp = node.cast<ASTComprehension>();

            switch (comp->compKind()) {
                case ASTComprehension::COMP_SET:
                    pyc_output << "{ ";
                    print_src(comp->result(), mod, pyc_output);
                    break;
                case ASTComprehension::COMP_MAP:
                    pyc_output << "{ ";
                    print_src(comp->key(), mod, pyc_output);
                    pyc_output << ": ";
                    print_src(comp->result(), mod, pyc_output);
                    break;
                default:
                    pyc_output << "[ ";
                    print_src(comp->result(), mod, pyc_output);
                    break;
            }

            for (const auto& gen : comp->generators()) {
                pyc_output << " for ";
                print_src(gen->index(), mod, pyc_output);
                pyc_output << " in ";
                print_src(gen->iter(), mod, pyc_output);
                if (gen->condition()) {
                    pyc_output << " if ";
                    print_src(gen->condition(), mod, pyc_output);
                }
            }
            switch (comp->compKind()) {
                case ASTComprehension::COMP_SET:
                case ASTComprehension::COMP_MAP:
                    pyc_output << " }";
                    break;
                default:
                    pyc_output << " ]";
                    break;
            }
        }
        break;
    case ASTNode::NODE_MAP:
        {
            pyc_output << "{";
            bool first = true;
            cur_indent++;
            for (const auto& val : node.cast<ASTMap>()->values()) {
                if (first)
                    pyc_output << "\n";
                else
                    pyc_output << ",\n";
                start_line(cur_indent, pyc_output);
                if (val.first != nullptr) {
                    print_src(val.first, mod, pyc_output);
                    pyc_output << ": ";
                } else {
                    /* null key: dict unpacking (`**expr`) from DICT_MERGE */
                    pyc_output << "**";
                }
                print_src(val.second, mod, pyc_output);
                first = false;
            }
            cur_indent--;
            pyc_output << " }";
        }
        break;
    case ASTNode::NODE_CONST_MAP:
        {
            PycRef<ASTConstMap> const_map = node.cast<ASTConstMap>();
            PycTuple::value_t keys = const_map->keys().cast<ASTObject>()->object().cast<PycTuple>()->values();
            ASTConstMap::values_t values = const_map->values();

            auto map = new ASTMap;
            for (const auto& key : keys) {
                // Values are pushed onto the stack in reverse order.
                PycRef<ASTNode> value = values.back();
                values.pop_back();

                map->add(new ASTObject(key), value);
            }

            print_src(map, mod, pyc_output);
        }
        break;
    case ASTNode::NODE_NAME:
        pyc_output << node.cast<ASTName>()->name()->value();
        break;
    case ASTNode::NODE_NODELIST:
        {
            cur_indent++;
            bool firstNl = true;
            for (const auto& ln : node.cast<ASTNodeList>()->nodes()) {
                bool isContainer = ln.cast<ASTNode>().type() == ASTNode::NODE_BLOCK
                        && ln.cast<ASTBlock>()->blktype() == ASTBlock::BLK_CONTAINER;
                /* Separate a try statement from the preceding statement
                   with a blank line, but avoid a leading blank line when
                   it opens the block.  The container prints its own line
                   starts. */
                if (isContainer) {
                    if (!firstNl)
                        pyc_output << "\n";
                } else if (ln.cast<ASTNode>().type() != ASTNode::NODE_NODELIST) {
                    start_line(cur_indent, pyc_output);
                }
                print_src(ln, mod, pyc_output);
                end_line(pyc_output);
                firstNl = false;
            }
            cur_indent--;
        }
        break;
    case ASTNode::NODE_BLOCK:
        {
            PycRef<ASTBlock> blk = node.cast<ASTBlock>();
            if (blk->blktype() == ASTBlock::BLK_ELSE && blk->size() == 0)
                break;

            if (blk->blktype() == ASTBlock::BLK_CONTAINER) {
                print_block(blk, mod, pyc_output);
                end_line(pyc_output);
                break;
            }

            pyc_output << blk->type_str();
            if (blk->blktype() == ASTBlock::BLK_IF
                    || blk->blktype() == ASTBlock::BLK_ELIF
                    || blk->blktype() == ASTBlock::BLK_WHILE) {
                PycRef<ASTNode> cond = blk.cast<ASTCondBlock>()->cond();
                if (blk->blktype() == ASTBlock::BLK_WHILE && cond == NULL) {
                    /* Pre-scanned unconditional loop (`while True:`). */
                    pyc_output << " True";
                } else {
                    if (blk.cast<ASTCondBlock>()->negative())
                        pyc_output << " not ";
                    else
                        pyc_output << " ";
                    print_src(cond, mod, pyc_output);
                }
            } else if (blk->blktype() == ASTBlock::BLK_FOR || blk->blktype() == ASTBlock::BLK_ASYNCFOR) {
                pyc_output << " ";
                print_src(blk.cast<ASTIterBlock>()->index(), mod, pyc_output);
                pyc_output << " in ";
                print_src(blk.cast<ASTIterBlock>()->iter(), mod, pyc_output);
            } else if (blk->blktype() == ASTBlock::BLK_EXCEPT &&
                    blk.cast<ASTCondBlock>()->cond() != NULL) {
                pyc_output << " ";
                print_src(blk.cast<ASTCondBlock>()->cond(), mod, pyc_output);
                PycRef<ASTNode> var = blk.try_cast<ASTCondBlock>()->var();
                if (var != NULL) {
                    pyc_output << " as ";
                    print_src(var, mod, pyc_output);
                }
            } else if (blk->blktype() == ASTBlock::BLK_WITH) {
                pyc_output << " ";
                print_src(blk.cast<ASTWithBlock>()->expr(), mod, pyc_output);
                PycRef<ASTNode> var = blk.try_cast<ASTWithBlock>()->var();
                if (var != NULL) {
                    pyc_output << " as ";
                    print_src(var, mod, pyc_output);
                }
            }
            pyc_output << ":\n";

            cur_indent++;
            print_block(blk, mod, pyc_output);
            cur_indent--;
        }
        break;
    case ASTNode::NODE_OBJECT:
        {
            PycRef<PycObject> obj = node.cast<ASTObject>()->object();
            if (obj.type() == PycObject::TYPE_CODE) {
                PycRef<PycCode> code = obj.cast<PycCode>();
                decompyle(code, mod, pyc_output);
            } else {
                print_const(pyc_output, obj, mod);
            }
        }
        break;
    case ASTNode::NODE_PRINT:
        {
            pyc_output << "print ";
            bool first = true;
            if (node.cast<ASTPrint>()->stream() != nullptr) {
                pyc_output << ">>";
                print_src(node.cast<ASTPrint>()->stream(), mod, pyc_output);
                first = false;
            }

            for (const auto& val : node.cast<ASTPrint>()->values()) {
                if (!first)
                    pyc_output << ", ";
                print_src(val, mod, pyc_output);
                first = false;
            }
            if (!node.cast<ASTPrint>()->eol())
                pyc_output << ",";
        }
        break;
    case ASTNode::NODE_RAISE:
        {
            PycRef<ASTRaise> raise = node.cast<ASTRaise>();
            pyc_output << "raise ";
            bool first = true;
            for (const auto& param : raise->params()) {
                if (!first)
                    pyc_output << ", ";
                print_src(param, mod, pyc_output);
                first = false;
            }
        }
        break;
    case ASTNode::NODE_RETURN:
        {
            PycRef<ASTReturn> ret = node.cast<ASTReturn>();
            PycRef<ASTNode> value = ret->value();
            if (!inLambda) {
                switch (ret->rettype()) {
                case ASTReturn::RETURN:
                    pyc_output << "return ";
                    break;
                case ASTReturn::YIELD:
                    pyc_output << "yield ";
                    break;
                case ASTReturn::YIELD_FROM:
                    if (value.type() == ASTNode::NODE_AWAITABLE) {
                        pyc_output << "await ";
                        value = value.cast<ASTAwaitable>()->expression();
                    } else {
                        pyc_output << "yield from ";
                    }
                    break;
                }
            }
            print_src(value, mod, pyc_output);
        }
        break;
    case ASTNode::NODE_SLICE:
        {
            PycRef<ASTSlice> slice = node.cast<ASTSlice>();

            if (slice->op() & ASTSlice::SLICE1) {
                print_src(slice->left(), mod, pyc_output);
            }
            pyc_output << ":";
            if (slice->op() & ASTSlice::SLICE2) {
                print_src(slice->right(), mod, pyc_output);
            }
        }
        break;
    case ASTNode::NODE_IMPORT:
        {
            PycRef<ASTImport> import = node.cast<ASTImport>();
            if (import->stores().size()) {
                ASTImport::list_t stores = import->stores();

                pyc_output << "from ";
                if (import->name().type() == ASTNode::NODE_IMPORT)
                    print_src(import->name().cast<ASTImport>()->name(), mod, pyc_output);
                else
                    print_src(import->name(), mod, pyc_output);
                pyc_output << " import ";

                if (stores.size() == 1) {
                    auto src = stores.front()->src();
                    auto dest = stores.front()->dest();
                    print_src(src, mod, pyc_output);

                    if (src.cast<ASTName>()->name()->value() != dest.cast<ASTName>()->name()->value()) {
                        pyc_output << " as ";
                        print_src(dest, mod, pyc_output);
                    }
                } else {
                    bool first = true;
                    for (const auto& st : stores) {
                        if (!first)
                            pyc_output << ", ";
                        print_src(st->src(), mod, pyc_output);
                        first = false;

                        if (st->src().cast<ASTName>()->name()->value() != st->dest().cast<ASTName>()->name()->value()) {
                            pyc_output << " as ";
                            print_src(st->dest(), mod, pyc_output);
                        }
                    }
                }
            } else {
                pyc_output << "import ";
                print_src(import->name(), mod, pyc_output);
            }
        }
        break;
    case ASTNode::NODE_FUNCTION:
        {
            /* Actual named functions are NODE_STORE with a name */
            pyc_output << "(lambda ";
            PycRef<ASTNode> code = node.cast<ASTFunction>()->code();
            PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
            ASTFunction::defarg_t defargs = node.cast<ASTFunction>()->defargs();
            ASTFunction::defarg_t kwdefargs = node.cast<ASTFunction>()->kwdefargs();
            auto da = defargs.cbegin();
            int narg = 0;
            for (int i=0; i<code_src->argCount(); i++) {
                if (narg)
                    pyc_output << ", ";
                pyc_output << code_src->getLocal(narg++)->value();
                if ((code_src->argCount() - i) <= (int)defargs.size()) {
                    pyc_output << " = ";
                    print_src(*da++, mod, pyc_output);
                }
            }
            da = kwdefargs.cbegin();
            if (code_src->kwOnlyArgCount() != 0) {
                pyc_output << (narg == 0 ? "*" : ", *");
                for (int i = 0; i < code_src->argCount(); i++) {
                    pyc_output << ", ";
                    pyc_output << code_src->getLocal(narg++)->value();
                    if ((code_src->kwOnlyArgCount() - i) <= (int)kwdefargs.size()) {
                        pyc_output << " = ";
                        print_src(*da++, mod, pyc_output);
                    }
                }
            }
            pyc_output << ": ";

            inLambda = true;
            print_src(code, mod, pyc_output);
            inLambda = false;

            pyc_output << ")";
        }
        break;
    case ASTNode::NODE_STORE:
        {
            PycRef<ASTNode> src = node.cast<ASTStore>()->src();
            PycRef<ASTNode> dest = node.cast<ASTStore>()->dest();
            if (src.type() == ASTNode::NODE_FUNCTION) {
                PycRef<ASTNode> code = src.cast<ASTFunction>()->code();
                PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
                bool isLambda = false;

                if (strcmp(code_src->name()->value(), "<lambda>") == 0) {
                    pyc_output << "\n";
                    start_line(cur_indent, pyc_output);
                    print_src(dest, mod, pyc_output);
                    pyc_output << " = lambda ";
                    isLambda = true;
                } else {
                    pyc_output << "\n";
                    start_line(cur_indent, pyc_output);
                    if (code_src->flags() & PycCode::CO_COROUTINE)
                        pyc_output << "async ";
                    pyc_output << "def ";
                    print_src(dest, mod, pyc_output);
                    pyc_output << "(";
                }

                ASTFunction::defarg_t defargs = src.cast<ASTFunction>()->defargs();
                ASTFunction::defarg_t kwdefargs = src.cast<ASTFunction>()->kwdefargs();
                auto da = defargs.cbegin();
                int narg = 0;
                for (int i = 0; i < code_src->argCount(); ++i) {
                    if (narg)
                        pyc_output << ", ";
                    pyc_output << code_src->getLocal(narg++)->value();
                    if ((code_src->argCount() - i) <= (int)defargs.size()) {
                        pyc_output << " = ";
                        print_src(*da++, mod, pyc_output);
                    }
                }
                da = kwdefargs.cbegin();
                if (code_src->kwOnlyArgCount() != 0) {
                    pyc_output << (narg == 0 ? "*" : ", *");
                    for (int i = 0; i < code_src->kwOnlyArgCount(); ++i) {
                        pyc_output << ", ";
                        pyc_output << code_src->getLocal(narg++)->value();
                        if ((code_src->kwOnlyArgCount() - i) <= (int)kwdefargs.size()) {
                            pyc_output << " = ";
                            print_src(*da++, mod, pyc_output);
                        }
                    }
                }
                if (code_src->flags() & PycCode::CO_VARARGS) {
                    if (narg)
                        pyc_output << ", ";
                    pyc_output << "*" << code_src->getLocal(narg++)->value();
                }
                if (code_src->flags() & PycCode::CO_VARKEYWORDS) {
                    if (narg)
                        pyc_output << ", ";
                    pyc_output << "**" << code_src->getLocal(narg++)->value();
                }

                if (isLambda) {
                    pyc_output << ": ";
                } else {
                    pyc_output << "):\n";
                    printDocstringAndGlobals = true;
                }

                bool preLambda = inLambda;
                inLambda |= isLambda;

                print_src(code, mod, pyc_output);

                inLambda = preLambda;
            } else if (src.type() == ASTNode::NODE_CLASS) {
                pyc_output << "\n";
                start_line(cur_indent, pyc_output);
                pyc_output << "class ";
                print_src(dest, mod, pyc_output);
                PycRef<ASTTuple> bases = src.cast<ASTClass>()->bases().cast<ASTTuple>();
                if (bases->values().size() > 0) {
                    pyc_output << "(";
                    bool first = true;
                    for (const auto& val : bases->values()) {
                        if (!first)
                            pyc_output << ", ";
                        print_src(val, mod, pyc_output);
                        first = false;
                    }
                    pyc_output << "):\n";
                } else {
                    // Don't put parens if there are no base classes
                    pyc_output << ":\n";
                }
                printClassDocstring = true;
                PycRef<ASTNode> code = src.cast<ASTClass>()->code().cast<ASTCall>()
                                       ->func().cast<ASTFunction>()->code();
                print_src(code, mod, pyc_output);
            } else if (src.type() == ASTNode::NODE_IMPORT) {
                PycRef<ASTImport> import = src.cast<ASTImport>();
                if (import->fromlist() != NULL) {
                    PycRef<PycObject> fromlist = import->fromlist().cast<ASTObject>()->object();
                    if (fromlist != Pyc_None) {
                        pyc_output << "from ";
                        if (import->name().type() == ASTNode::NODE_IMPORT)
                            print_src(import->name().cast<ASTImport>()->name(), mod, pyc_output);
                        else
                            print_src(import->name(), mod, pyc_output);
                        pyc_output << " import ";
                        if (fromlist.type() == PycObject::TYPE_TUPLE ||
                                fromlist.type() == PycObject::TYPE_SMALL_TUPLE) {
                            bool first = true;
                            for (const auto& val : fromlist.cast<PycTuple>()->values()) {
                                if (!first)
                                    pyc_output << ", ";
                                pyc_output << val.cast<PycString>()->value();
                                first = false;
                            }
                        } else {
                            pyc_output << fromlist.cast<PycString>()->value();
                        }
                    } else {
                        pyc_output << "import ";
                        print_src(import->name(), mod, pyc_output);
                    }
                } else {
                    pyc_output << "import ";
                    PycRef<ASTNode> import_name = import->name();
                    print_src(import_name, mod, pyc_output);
                    if (!dest.cast<ASTName>()->name()->isEqual(import_name.cast<ASTName>()->name().cast<PycObject>())) {
                        pyc_output << " as ";
                        print_src(dest, mod, pyc_output);
                    }
                }
            } else if (src.type() == ASTNode::NODE_BINARY
                    && src.cast<ASTBinary>()->is_inplace()) {
                print_src(src, mod, pyc_output);
            } else if (node.cast<ASTStore>()->isWalrus()) {
                pyc_output << "(";
                print_src(dest, mod, pyc_output);
                pyc_output << " := ";
                print_src(src, mod, pyc_output);
                pyc_output << ")";
            } else {
                print_src(dest, mod, pyc_output);
                pyc_output << " = ";
                print_src(src, mod, pyc_output);
            }
        }
        break;
    case ASTNode::NODE_CHAINSTORE:
        {
            for (auto& dest : node.cast<ASTChainStore>()->nodes()) {
                print_src(dest, mod, pyc_output);
                pyc_output << " = ";
            }
            print_src(node.cast<ASTChainStore>()->src(), mod, pyc_output);
        }
        break;
    case ASTNode::NODE_SUBSCR:
        {
            print_src(node.cast<ASTSubscr>()->name(), mod, pyc_output);
            pyc_output << "[";
            print_src(node.cast<ASTSubscr>()->key(), mod, pyc_output);
            pyc_output << "]";
        }
        break;
    case ASTNode::NODE_CONVERT:
        {
            pyc_output << "`";
            print_src(node.cast<ASTConvert>()->name(), mod, pyc_output);
            pyc_output << "`";
        }
        break;
    case ASTNode::NODE_TUPLE:
        {
            PycRef<ASTTuple> tuple = node.cast<ASTTuple>();
            ASTTuple::value_t values = tuple->values();
            if (tuple->requireParens())
                pyc_output << "(";
            bool first = true;
            for (const auto& val : values) {
                if (!first)
                    pyc_output << ", ";
                print_src(val, mod, pyc_output);
                first = false;
            }
            if (values.size() == 1)
                pyc_output << ',';
            if (tuple->requireParens())
                pyc_output << ')';
        }
        break;
    case ASTNode::NODE_ANNOTATED_VAR:
        {
            PycRef<ASTAnnotatedVar> annotated_var = node.cast<ASTAnnotatedVar>();
            PycRef<ASTObject> name = annotated_var->name().cast<ASTObject>();
            PycRef<ASTNode> annotation = annotated_var->annotation();

            pyc_output << name->object().cast<PycString>()->value();
            pyc_output << ": ";
            print_src(annotation, mod, pyc_output);
        }
        break;
    case ASTNode::NODE_TERNARY:
        {
            /* parenthesis might be needed
             * 
             * when if-expr is part of numerical expression, ternary has the LOWEST precedence
             *     print(a + b if False else c)
             * output is c, not a+c (a+b is calculated first)
             * 
             * but, let's not add parenthesis - to keep the source as close to original as possible in most cases
             */
            PycRef<ASTTernary> ternary = node.cast<ASTTernary>();
            //pyc_output << "(";
            print_src(ternary->if_expr(), mod, pyc_output);
            const auto if_block = ternary->if_block().cast<ASTCondBlock>();
            pyc_output << " if ";
            if (if_block->negative())
                pyc_output << "not ";
            print_src(if_block->cond(), mod, pyc_output);
            pyc_output << " else ";
            print_src(ternary->else_expr(), mod, pyc_output);
            //pyc_output << ")";
        }
        break;
    case ASTNode::NODE_LOCALS:
        pyc_output << "locals()";
        break;
    case ASTNode::NODE_AWAITABLE:
        {
            /* Python 3.11+ await expressions: the SEND/END_SEND machinery
               leaves the awaitable on the stack; print it as an expression
               so stores and returns both render as `await <expr>`. */
            PycRef<ASTAwaitable> awaitable = node.cast<ASTAwaitable>();
            pyc_output << "await ";
            print_src(awaitable->expression(), mod, pyc_output);
        }
        break;
    default:
        pyc_output << "<NODE:" << node->type() << ">";
        fprintf(stderr, "Unsupported Node type: %d\n", node->type());
        cleanBuild = false;
        node_seen.erase((ASTNode *)node);
        return;
    }

    cleanBuild = true;
    node_seen.erase((ASTNode *)node);
}

bool print_docstring(PycRef<PycObject> obj, int indent, PycModule* mod,
                     std::ostream& pyc_output)
{
    // docstrings are translated from the bytecode __doc__ = 'string' to simply '''string'''
    auto doc = obj.try_cast<PycString>();
    if (doc != nullptr) {
        start_line(indent, pyc_output);
        doc->print(pyc_output, mod, true);
        pyc_output << "\n";
        return true;
    }
    return false;
}

static std::unordered_set<PycCode *> code_seen;

/* A real docstring is only referenced through the co_consts[0] convention
   (3.0+ emits no bytecode for it).  If any instruction loads the first
   constant, it is ordinary code data and must not be printed as a
   docstring. */
static bool isFirstConstDocstring(PycRef<PycCode> code, PycModule* mod)
{
    if (code->consts()->size() == 0)
        return false;
    if (code->getConst(0).try_cast<PycString>() == nullptr)
        return false;

    PycBuffer source(code->code()->value(), code->code()->length());
    int opcode, operand, pos = 0;
    while (!source.atEof()) {
        bc_next(source, mod, opcode, operand, pos);
        switch (opcode) {
        case Pyc::LOAD_CONST_A:
        case Pyc::KW_NAMES_A:
        case Pyc::RETURN_CONST_A:
            if (operand == 0)
                return false;
            break;
        default:
            break;
        }
    }
    return true;
}

void decompyle(PycRef<PycCode> code, PycModule* mod, std::ostream& pyc_output)
{
    if (code_seen.find((PycCode *)code) != code_seen.end()) {
        fputs("WARNING: Circular reference detected\n", stderr);
        return;
    }
    code_seen.insert((PycCode *)code);

    PycRef<ASTNode> source = BuildFromCode(code, mod);

    PycRef<ASTNodeList> clean = source.cast<ASTNodeList>();
    if (cleanBuild) {
        // The Python compiler adds some stuff that we don't really care
        // about, and would add extra code for re-compilation anyway.
        // We strip these lines out here, and then add a "pass" statement
        // if the cleaned up code is empty
        if (clean->nodes().front().type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->src().type() == ASTNode::NODE_NAME
                    && store->dest().type() == ASTNode::NODE_NAME) {
                PycRef<ASTName> src = store->src().cast<ASTName>();
                PycRef<ASTName> dest = store->dest().cast<ASTName>();
                if (src->name()->isEqual("__name__")
                        && dest->name()->isEqual("__module__")) {
                    // __module__ = __name__
                    // Automatically added by Python 2.2.1 and later
                    clean->removeFirst();
                }
            }
        }
        if (clean->nodes().front().type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->src().type() == ASTNode::NODE_OBJECT
                    && store->dest().type() == ASTNode::NODE_NAME) {
                PycRef<ASTObject> src = store->src().cast<ASTObject>();
                PycRef<PycString> srcString = src->object().try_cast<PycString>();
                PycRef<ASTName> dest = store->dest().cast<ASTName>();
                if (dest->name()->isEqual("__qualname__")) {
                    // __qualname__ = '<Class Name>'
                    // Automatically added by Python 3.3 and later
                    clean->removeFirst();
                }
            }
        }

        // Class and module docstrings may only appear at the beginning of their source
        if (printClassDocstring && clean->nodes().front().type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->dest().type() == ASTNode::NODE_NAME &&
                    store->dest().cast<ASTName>()->name()->isEqual("__doc__") &&
                    store->src().type() == ASTNode::NODE_OBJECT) {
                if (print_docstring(store->src().cast<ASTObject>()->object(),
                        cur_indent + (code->name()->isEqual("<module>") ? 0 : 1), mod, pyc_output))
                    clean->removeFirst();
            }
        }

        /* Python 3.13+: the compiler synthesizes class bookkeeping
           attributes (__firstlineno__, __static_attributes__) and, in
           3.14, the __classdict__/__classdictcell__ cell plumbing for
           closures over the class namespace.  None have a source-level
           representation, and re-emitting __classdictcell__ would not
           even compile. */
        clean->removeIf([](PycRef<ASTNode> node) -> bool {
            if (node->type() != ASTNode::NODE_STORE)
                return false;
            PycRef<ASTStore> store = node.cast<ASTStore>();
            if (store->dest().type() != ASTNode::NODE_NAME)
                return false;
            PycRef<PycString> dest = store->dest().cast<ASTName>()->name();
            if (dest->isEqual("__firstlineno__")
                    || dest->isEqual("__static_attributes__"))
                return store->src().type() == ASTNode::NODE_OBJECT;
            if (dest->isEqual("__classdict__"))
                return store->src().type() == ASTNode::NODE_LOCALS;
            if (dest->isEqual("__classdictcell__")) {
                return store->src().type() == ASTNode::NODE_NAME
                        && store->src().cast<ASTName>()->name()->isEqual("__classdict__");
            }
            return false;
        });
        if (clean->nodes().back().type() == ASTNode::NODE_RETURN) {
            PycRef<ASTReturn> ret = clean->nodes().back().cast<ASTReturn>();

            PycRef<ASTObject> retObj = ret->value().try_cast<ASTObject>();
            if (ret->value() == NULL || ret->value().type() == ASTNode::NODE_LOCALS ||
                    (retObj && retObj->object().type() == PycObject::TYPE_NONE)) {
                clean->removeLast();  // Always an extraneous return statement
            }
        }
        if (clean->nodes().back().type() == ASTNode::NODE_BLOCK
                && clean->nodes().back().cast<ASTBlock>()->blktype() == ASTBlock::BLK_IF) {
            /* Python 3.14 module tail: when the last statement is an
               `if __name__ == '__main__':`-style block, the compiler ends
               the then-part with the implicit module return (LOAD_CONST
               None; RETURN_VALUE) instead of a JUMP_FORWARD, so the
               extraneous `return None` lands inside the if block. */
            PycRef<ASTBlock> ifblk = clean->nodes().back().cast<ASTBlock>();
            if (ifblk->size() > 0 && ifblk->nodes().back().type() == ASTNode::NODE_RETURN) {
                PycRef<ASTReturn> ret = ifblk->nodes().back().cast<ASTReturn>();
                PycRef<ASTObject> retObj = ret->value().try_cast<ASTObject>();
                if (ret->value() == NULL
                        || (retObj && retObj->object().type() == PycObject::TYPE_NONE)) {
                    ifblk->removeLast();
                }
            }
        }
    }
    if (printClassDocstring)
        printClassDocstring = false;
    // This is outside the clean check so a source block will always
    // be compilable, even if decompylation failed.
    if (clean->nodes().size() == 0 && !code.isIdent(mod->code()))
        clean->append(new ASTKeyword(ASTKeyword::KW_PASS));

    bool part1clean = cleanBuild;

    if (printDocstringAndGlobals) {
        if (isFirstConstDocstring(code, mod))
            print_docstring(code->getConst(0), cur_indent + 1, mod, pyc_output);

        PycCode::globals_t globs = code->getGlobals();
        if (globs.size()) {
            start_line(cur_indent + 1, pyc_output);
            pyc_output << "global ";
            bool first = true;
            for (const auto& glob : globs) {
                if (!first)
                    pyc_output << ", ";
                pyc_output << glob->value();
                first = false;
            }
            pyc_output << "\n";
        }
        printDocstringAndGlobals = false;
    }

    print_src(source, mod, pyc_output);

    if (!cleanBuild || !part1clean) {
        start_line(cur_indent, pyc_output);
        pyc_output << "# WARNING: Decompyle incomplete\n";
    }

    code_seen.erase((PycCode *)code);
}
