#include <cstring>
#include "ASTree.h"
#include "FastStack.h"
#include "pyc_numeric.h"
#include "bytecode.h"

/* Use this to determine if an error occurred (and therefore, if we should
 * avoid cleaning the output tree) */
static bool cleanBuild;

/* Keep track of whether we're in a print statement, so we can make
 * chained prints (print x, y, z) prettier */
static bool inPrint;

/* Use this to prevent printing return keywords and newlines in lambdas. */
static bool inLambda = false;

/* Use this to keep track of whether we need to print out the list of global
 * variables that we are using (such as inside a function). */
static bool printGlobals = false;

PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod)
{
    PycBuffer source(code->code()->value(), code->code()->length());

    FastStack stack((mod->majorVer() == 1) ? 20 : code->stackSize());
    stackhist_t stack_hist;

    std::stack<PycRef<ASTBlock> > blocks;
    PycRef<ASTBlock> defblock = new ASTBlock(ASTBlock::BLK_MAIN);
    defblock->init();
    PycRef<ASTBlock>& curblock = defblock;
    blocks.push(defblock);

    int opcode, operand;
    int curpos = 0;
    int pos = 0;
    int unpack = 0;
    bool else_pop = false;
    bool need_try = false;

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

        curpos = pos;
        bc_next(source, mod, opcode, operand, pos);

        if (need_try && opcode != Pyc::SETUP_EXCEPT_A) {
            need_try = false;

            /* Store the current stack for the except/finally statement(s) */
            stack_hist.push(stack);
            PycRef<ASTBlock> tryblock = new ASTBlock(ASTBlock::BLK_TRY, curblock->end(), true);
            blocks.push(tryblock.cast<ASTBlock>());
            curblock = blocks.top();
        } else if (else_pop
                && opcode != Pyc::JUMP_FORWARD_A
                && opcode != Pyc::JUMP_IF_FALSE_A
                && opcode != Pyc::JUMP_IF_FALSE_OR_POP_A
                && opcode != Pyc::POP_JUMP_IF_FALSE_A
                && opcode != Pyc::JUMP_IF_TRUE_A
                && opcode != Pyc::JUMP_IF_TRUE_OR_POP_A
                && opcode != Pyc::POP_JUMP_IF_TRUE_A
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
            }
        }

        switch (opcode) {
        case Pyc::BINARY_ADD:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_ADD));
            }
            break;
        case Pyc::BINARY_AND:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_AND));
            }
            break;
        case Pyc::BINARY_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case Pyc::BINARY_FLOOR_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_FLOOR));
            }
            break;
        case Pyc::BINARY_LSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_LSHIFT));
            }
            break;
        case Pyc::BINARY_MODULO:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_MODULO));
            }
            break;
        case Pyc::BINARY_MULTIPLY:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_MULTIPLY));
            }
            break;
        case Pyc::BINARY_OR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_OR));
            }
            break;
        case Pyc::BINARY_POWER:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_POWER));
            }
            break;
        case Pyc::BINARY_RSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_RSHIFT));
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
        case Pyc::BINARY_SUBTRACT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case Pyc::BINARY_TRUE_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case Pyc::BINARY_XOR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_XOR));
            }
            break;
        case Pyc::BREAK_LOOP:
            curblock->append(new ASTKeyword(ASTKeyword::KW_BREAK));
            break;
        case Pyc::BUILD_CLASS:
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                PycRef<ASTNode> bases = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTClass(code, bases, name));
            }
            break;
        case Pyc::BUILD_FUNCTION:
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                stack.push(new ASTFunction(code, ASTFunction::defarg_t()));
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
        case Pyc::BUILD_MAP_A:
            stack.push(new ASTMap());
            break;
        case Pyc::STORE_MAP:
            {
                ASTList::value_t values;
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

                    if (start->type() == ASTNode::NODE_OBJECT
                            && start.cast<ASTObject>()->object() == Pyc_None) {
                        start = Node_NULL;
                    }

                    if (end->type() == ASTNode::NODE_OBJECT
                            && end.cast<ASTObject>()->object() == Pyc_None) {
                        end = Node_NULL;
                    }

                    if (start == Node_NULL && end == Node_NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE0));
                    } else if (start == Node_NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
                    } else if (end == Node_NULL) {
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

                    if (start->type() == ASTNode::NODE_OBJECT
                            && start.cast<ASTObject>()->object() == Pyc_None) {
                        start = Node_NULL;
                    }

                    if (end->type() == ASTNode::NODE_OBJECT
                            && end.cast<ASTObject>()->object() == Pyc_None) {
                        end = Node_NULL;
                    }

                    if (step->type() == ASTNode::NODE_OBJECT
                            && step.cast<ASTObject>()->object() == Pyc_None) {
                        step = Node_NULL;
                    }

                    /* We have to do this as a slice where one side is another slice */
                    /* [[a:b]:c] */

                    if (start == Node_NULL && end == Node_NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE0));
                    } else if (start == Node_NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE2, start, end));
                    } else if (end == Node_NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, start, end));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, start, end));
                    }

                    PycRef<ASTNode> lhs = stack.top();
                    stack.pop();

                    if (step == Node_NULL) {
                        stack.push(new ASTSlice(ASTSlice::SLICE1, lhs, step));
                    } else {
                        stack.push(new ASTSlice(ASTSlice::SLICE3, lhs, step));
                    }
                }
            }
            break;
        case Pyc::BUILD_TUPLE_A:
            {
                ASTTuple::value_t values;
                values.resize(operand);
                for (int i=0; i<operand; i++) {
                    values[operand-i-1] = stack.top();
                    stack.pop();
                }
                stack.push(new ASTTuple(values));
            }
            break;
        case Pyc::CALL_FUNCTION_A:
            {
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
                    PycRef<ASTNode> param = stack.top();
                    stack.pop();
                    if (param->type() == ASTNode::NODE_FUNCTION) {
                        PycRef<ASTNode> code = param.cast<ASTFunction>()->code();
                        PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
                        PycRef<PycString> function_name = code_src->name();
                        if (function_name->isEqual("<lambda>")) {
                            pparamList.push_front(param);
                        } else {
                            // Decorator used
                            PycRef<ASTNode> name = new ASTName(function_name);
                            curblock->append(new ASTStore(param, name));

                            pparamList.push_front(name);
                        }
                    } else {
                        pparamList.push_front(param);
                    }
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();
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
        case Pyc::CONTINUE_LOOP_A:
            curblock->append(new ASTKeyword(ASTKeyword::KW_CONTINUE));
            break;
        case Pyc::COMPARE_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTCompare(left, right, operand));
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

                if (varname->value()[0] == '_' && varname->value()[1] == '[') {
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
                    name = new ASTName(code->getVarName(operand));

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

                curblock->append(new ASTDelete(new ASTSubscr(name, new ASTSlice(ASTSlice::SLICE2, Node_NULL, lower))));
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
            stack.push(stack.top());
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
                    /* Turn it into an else statement. */
                    blocks.pop();
                    PycRef<ASTBlock> prev = curblock;
                    if (curblock->size() != 0) {
                        blocks.top()->append(curblock.cast<ASTNode>());
                    }
                    curblock = blocks.top();

                    if (curblock->end() != pos || curblock.cast<ASTContainerBlock>()->hasFinally()) {
                        PycRef<ASTBlock> elseblk = new ASTBlock(ASTBlock::BLK_ELSE, prev->end());
                        elseblk->init();
                        blocks.push(elseblk);
                        curblock = blocks.top();
                    } else {
                        stack = stack_hist.top();
                        stack_hist.pop();
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
            {
                PycRef<ASTNode> iter = stack.top(); // Iterable
                stack.pop();
                /* Pop it? Don't pop it? */

                bool comprehension = false;
                PycRef<ASTBlock> top = blocks.top();
                if (top->blktype() == ASTBlock::BLK_WHILE) {
                    blocks.pop();
                } else {
                    comprehension = true;
                }
                PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, top->end(), iter);
                forblk->setComprehension(comprehension);
                blocks.push(forblk.cast<ASTBlock>());
                curblock = blocks.top();

                stack.push(Node_NULL);
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
                PycRef<ASTIterBlock> forblk = new ASTIterBlock(ASTBlock::BLK_FOR, top->end(), iter);
                forblk->setComprehension(comprehension);
                blocks.push(forblk.cast<ASTBlock>());
                curblock = blocks.top();

                /* Python Docs say:
                      "push the sequence, the incremented counter,
                       and the current item onto the stack." */
                stack.push(iter);
                stack.push(curidx);
                stack.push(Node_NULL); // We can totally hack this >_>
            }
            break;
        case Pyc::GET_ITER:
            /* We just entirely ignore this */
            break;
        case Pyc::IMPORT_NAME_A:
            if (mod->majorVer() == 1) {
                stack.push(new ASTImport(new ASTName(code->getName(operand)), Node_NULL));
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
                curblock->append(new ASTStore(import, Node_NULL));
            }
            break;
        case Pyc::INPLACE_ADD:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_ADD));
            }
            break;
        case Pyc::INPLACE_AND:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_AND));
            }
            break;
        case Pyc::INPLACE_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_DIVIDE));
            }
            break;
        case Pyc::INPLACE_FLOOR_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_FLOOR));
            }
            break;
        case Pyc::INPLACE_LSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_LSHIFT));
            }
            break;
        case Pyc::INPLACE_MODULO:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_MODULO));
            }
            break;
        case Pyc::INPLACE_MULTIPLY:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_MULTIPLY));
            }
            break;
        case Pyc::INPLACE_OR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_OR));
            }
            break;
        case Pyc::INPLACE_POWER:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_POWER));
            }
            break;
        case Pyc::INPLACE_RSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_RSHIFT));
            }
            break;
        case Pyc::INPLACE_SUBTRACT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_IP_SUBTRACT));
            }
            break;
        case Pyc::INPLACE_TRUE_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_DIVIDE));
            }
            break;
        case Pyc::INPLACE_XOR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_IP_XOR));
            }
            break;
        case Pyc::JUMP_IF_FALSE_A:
        case Pyc::JUMP_IF_TRUE_A:
        case Pyc::JUMP_IF_FALSE_OR_POP_A:
        case Pyc::JUMP_IF_TRUE_OR_POP_A:
        case Pyc::POP_JUMP_IF_FALSE_A:
        case Pyc::POP_JUMP_IF_TRUE_A:
            {
                PycRef<ASTNode> cond = stack.top();
                PycRef<ASTCondBlock> ifblk;
                int popped = ASTCondBlock::UNINITED;

                if (opcode == Pyc::POP_JUMP_IF_FALSE_A
                        || opcode == Pyc::POP_JUMP_IF_TRUE_A) {
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
                        || opcode == Pyc::POP_JUMP_IF_TRUE_A;

                int offs = operand;
                if (opcode == Pyc::JUMP_IF_FALSE_A
                        || opcode == Pyc::JUMP_IF_TRUE_A) {
                    /* Offset is relative in these cases */
                    offs = pos + operand;
                }

                if (cond->type() == ASTNode::NODE_COMPARE
                        && cond.cast<ASTCompare>()->op() == ASTCompare::CMP_EXCEPTION) {
                    if (curblock->blktype() == ASTBlock::BLK_EXCEPT
                            && curblock.cast<ASTCondBlock>()->cond() == Node_NULL) {
                        blocks.pop();
                        curblock = blocks.top();

                        stack_hist.pop();
                    }
                    ifblk = new ASTCondBlock(ASTBlock::BLK_EXCEPT, offs, cond.cast<ASTCompare>()->right(), false);
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

                    if (curblock->end() == offs
                            || (curblock->end() == curpos && !top->negative())) {
                        /* if blah and blah */
                        newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_AND);
                    } else {
                        /* if blah or blah */
                        newcond = new ASTBinary(cond1, cond, ASTBinary::BIN_LOG_OR);
                    }
                    ifblk = new ASTCondBlock(top->blktype(), offs, newcond, neg);
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
        case Pyc::JUMP_ABSOLUTE_A:
            {
                if (operand < pos) {
                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && curblock.cast<ASTIterBlock>()->isComprehension()) {
                        PycRef<ASTNode> top = stack.top();

                        if (top->type() == ASTNode::NODE_COMPREHENSION) {
                            PycRef<ASTComprehension> comp = top.cast<ASTComprehension>();

                            comp->addGenerator(curblock.cast<ASTIterBlock>());
                        }

                        blocks.pop();
                        curblock = blocks.top();
                    } if (curblock->blktype() == ASTBlock::BLK_ELSE) {
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
                        PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, 0, Node_NULL, false);
                        except->init();
                        blocks.push(except);
                        curblock = blocks.top();
                    }
                    break;
                }

                stack = stack_hist.top();
                stack_hist.pop();

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
                        PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_EXCEPT, blocks.top()->end(), Node_NULL, false);
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
            {
                if (curblock->blktype() == ASTBlock::BLK_CONTAINER) {
                    PycRef<ASTContainerBlock> cont = curblock.cast<ASTContainerBlock>();
                    if (cont->hasExcept()) {
                        stack_hist.push(stack);

                        curblock->setEnd(pos+operand);
                        PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, pos+operand, Node_NULL, false);
                        except->init();
                        blocks.push(except);
                        curblock = blocks.top();
                    }
                    break;
                }

                if ((curblock->blktype() == ASTBlock::BLK_WHILE
                            && !curblock->inited())
                        || (curblock->blktype() == ASTBlock::BLK_IF
                            && curblock->size() == 0)) {
                    PycRef<PycObject> fakeint = new PycInt(1);
                    PycRef<ASTNode> truthy = new ASTObject(fakeint);

                    stack.push(truthy);
                    break;
                }

                if (!stack_hist.empty()) {
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
                        if (operand == 0) {
                            prev = nil;
                            continue;
                        }

                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTBlock(ASTBlock::BLK_ELSE, pos+operand);
                        if (prev->inited() == ASTCondBlock::PRE_POPPED) {
                            next->init(ASTCondBlock::PRE_POPPED);
                        }

                        blocks.push(next.cast<ASTBlock>());
                        prev = nil;
                    } else if (prev->blktype() == ASTBlock::BLK_EXCEPT) {
                        if (operand == 0) {
                            prev = nil;
                            continue;
                        }

                        if (push) {
                            stack_hist.push(stack);
                        }
                        PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_EXCEPT, pos+operand, Node_NULL, false);
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
                            && prev->end() < pos+operand) {
                        /* Need to add an except/finally block */
                        stack = stack_hist.top();
                        stack.pop();

                        if (blocks.top()->blktype() == ASTBlock::BLK_CONTAINER) {
                            PycRef<ASTContainerBlock> cont = blocks.top().cast<ASTContainerBlock>();
                            if (cont->hasExcept()) {
                                if (push) {
                                    stack_hist.push(stack);
                                }

                                PycRef<ASTBlock> except = new ASTCondBlock(ASTBlock::BLK_EXCEPT, pos+operand, Node_NULL, false);
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

                curblock = blocks.top();

                if (curblock->blktype() == ASTBlock::BLK_EXCEPT) {
                    curblock->setEnd(pos+operand);
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
                    stack.push(new ASTComprehension(value));
                } else {
                    stack.push(new ASTSubscr(list, value)); /* Total hack */
                }
            }
            break;
        case Pyc::LOAD_ATTR_A:
            {
                PycRef<ASTNode> name = stack.top();
                if (name->type() != ASTNode::NODE_IMPORT) {
                    stack.pop();
                    stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
                }
            }
            break;
        case Pyc::LOAD_CLOSURE_A:
            /* Ignore this */
            break;
        case Pyc::LOAD_CONST_A:
            {
                PycRef<ASTObject> t_ob = new ASTObject(code->getConst(operand));

                if ((t_ob->object()->type() == PycObject::TYPE_TUPLE ||
                        t_ob->object()->type() == PycObject::TYPE_SMALL_TUPLE) &&
                        !t_ob->object().cast<PycTuple>()->values().size()) {
                    ASTTuple::value_t values;
                    stack.push(new ASTTuple(values));
                } else if (t_ob->object()->type() == PycObject::TYPE_NONE) {
                    stack.push(Node_NULL);
                } else {
                    stack.push(t_ob.cast<ASTNode>());
                }
            }
            break;
        case Pyc::LOAD_DEREF_A:
            stack.push(new ASTName(code->getCellVar(operand).cast<PycString>()));
            break;
        case Pyc::LOAD_FAST_A:
            if (mod->verCompare(1, 3) < 0)
                stack.push(new ASTName(code->getName(operand)));
            else
                stack.push(new ASTName(code->getVarName(operand)));
            break;
        case Pyc::LOAD_GLOBAL_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::LOAD_LOCALS:
            stack.push(new ASTNode(ASTNode::NODE_LOCALS));
            break;
        case Pyc::LOAD_NAME_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::MAKE_CLOSURE_A:
        case Pyc::MAKE_FUNCTION_A:
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                ASTFunction::defarg_t defArgs;
                for (int i=0; i<operand; i++) {
                    defArgs.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTFunction(code, defArgs));
            }
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

                PycRef<ASTBlock> tmp;

                if (curblock->nodes().size() &&
                        curblock->nodes().back()->type() == ASTNode::NODE_KEYWORD) {
                    curblock->removeLast();
                }

                if (curblock->blktype() == ASTBlock::BLK_IF
                        || curblock->blktype() == ASTBlock::BLK_ELIF
                        || curblock->blktype() == ASTBlock::BLK_ELSE
                        || curblock->blktype() == ASTBlock::BLK_TRY
                        || curblock->blktype() == ASTBlock::BLK_EXCEPT
                        || curblock->blktype() == ASTBlock::BLK_FINALLY) {
                    stack = stack_hist.top();
                    stack_hist.pop();
                }

                tmp = curblock;
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

                if (curblock->blktype() == ASTBlock::BLK_FOR
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
        case Pyc::POP_TOP:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                if (!curblock->inited()) {
                    if (curblock->blktype() == ASTBlock::BLK_WITH) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                    } else {
                        curblock.cast<ASTCondBlock>()->init();
                    }
                    break;
                } else if (value->type() == ASTNode::NODE_INVALID
                        || value->type() == ASTNode::NODE_BINARY
                        || value->type() == ASTNode::NODE_NAME) {
                    break;
                } else if (value->type() == ASTNode::NODE_COMPARE
                        && value.cast<ASTCompare>()->op() == ASTCompare::CMP_EXCEPTION) {
                    break;
                }

                curblock->append(value);

                if (curblock->blktype() == ASTBlock::BLK_FOR
                        && curblock.cast<ASTIterBlock>()->isComprehension()) {
                    /* This relies on some really uncertain logic...
                     * If it's a comprehension, the only POP_TOP should be
                     * a call to append the iter to the list.
                     */
                    if (value->type() == ASTNode::NODE_CALL) {
                        PycRef<ASTNode> res = value.cast<ASTCall>()->pparams().front();

                        stack.push(new ASTComprehension(res));
                    }
                }
            }
            break;
        case Pyc::PRINT_ITEM:
            curblock->append(new ASTPrint(stack.top()));
            stack.pop();
            break;
        case Pyc::PRINT_ITEM_TO:
            {
                PycRef<ASTNode> stream = stack.top();
                stack.pop();

                curblock->append(new ASTPrint(stack.top(), stream));
                stack.pop();
                break;
            }
        case Pyc::PRINT_NEWLINE:
            curblock->append(new ASTPrint(Node_NULL));
            break;
        case Pyc::PRINT_NEWLINE_TO:
            curblock->append(new ASTPrint(Node_NULL, stack.top()));
            stack.pop();
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

                    bc_next(source, mod, opcode, operand, pos);
                }
            }
            break;
        case Pyc::RETURN_VALUE:
            {
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

                    bc_next(source, mod, opcode, operand, pos);
                }
            }
            break;
        case Pyc::ROT_TWO:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
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
            {
                PycRef<ASTBlock> withblock = new ASTWithBlock(pos+operand);
                blocks.push(withblock);
                curblock = blocks.top();
            }
            break;
        case Pyc::WITH_CLEANUP:
            {
                // Stack top should be a None. Ignore it.
                PycRef<ASTNode> none = stack.top();
                stack.pop();

                if (none != Node_NULL) {
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
                PycRef<ASTBlock> next = new ASTCondBlock(ASTBlock::BLK_WHILE, pos+operand, Node_NULL, false);
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

                PycRef<ASTNode> slice = new ASTSlice(ASTSlice::SLICE2, Node_NULL, upper);
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
                    if (tup->type() == ASTNode::NODE_TUPLE) {
                        stack.pop();

                        PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                        tuple->add(attr);

                        stack.push(tuple.cast<ASTNode>());
                    } else {
                        fprintf(stderr, "Something TERRIBLE happened!\n");
                    }

                    if (--unpack <= 0) {
                        PycRef<ASTNode> tup = stack.top();
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        curblock->append(new ASTStore(seq, tup));
                    }
                } else {
                    PycRef<ASTNode> name = stack.top();
                    stack.pop();
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);

                    curblock->append(new ASTStore(value, attr));
                }
            }
            break;
        case Pyc::STORE_DEREF_A:
            {
                if (unpack) {
                    PycRef<ASTNode> name = new ASTName(code->getCellVar(operand).cast<PycString>());

                    PycRef<ASTNode> tup = stack.top();
                    if (tup->type() == ASTNode::NODE_TUPLE) {
                        stack.pop();

                        PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                        tuple->add(name);

                        stack.push(tuple.cast<ASTNode>());
                    } else {
                        fprintf(stderr, "Something TERRIBLE happened!\n");
                    }

                    if (--unpack <= 0) {
                        PycRef<ASTNode> tup = stack.top();
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        curblock->append(new ASTStore(seq, tup));
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    PycRef<ASTNode> name = new ASTName(code->getCellVar(operand).cast<PycString>());
                    curblock->append(new ASTStore(value, name));
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
                        name = new ASTName(code->getVarName(operand));

                    PycRef<ASTNode> tup = stack.top();
                    if (tup->type() == ASTNode::NODE_TUPLE) {
                        stack.pop();

                        PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                        tuple->add(name);

                        stack.push(tuple.cast<ASTNode>());
                    } else {
                        fprintf(stderr, "Something TERRIBLE happened!\n");
                    }

                    if (--unpack <= 0) {
                        PycRef<ASTNode> tup = stack.top();
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
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
                        name = new ASTName(code->getVarName(operand));

                    if (name.cast<ASTName>()->name()->value()[0] == '_'
                            && name.cast<ASTName>()->name()->value()[1] == '[') {
                        /* Don't show stores of list comp append objects. */
                        break;
                    }

                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && !curblock->inited()) {
                        curblock.cast<ASTIterBlock>()->setIndex(name);
                    } else if (curblock->blktype() == ASTBlock::BLK_WITH
                                   && !curblock->inited()) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                        curblock.cast<ASTWithBlock>()->setVar(name);
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
                    if (tup->type() == ASTNode::NODE_TUPLE) {
                        stack.pop();

                        PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                        tuple->add(name);

                        stack.push(tuple.cast<ASTNode>());
                    } else {
                        fprintf(stderr, "Something TERRIBLE happened!\n");
                    }

                    if (--unpack <= 0) {
                        PycRef<ASTNode> tup = stack.top();
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();
                    curblock->append(new ASTStore(value, name));
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
                    if (tup->type() == ASTNode::NODE_TUPLE) {
                        stack.pop();

                        PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                        tuple->add(name);

                        stack.push(tuple.cast<ASTNode>());
                    } else {
                        fprintf(stderr, "Something TERRIBLE happened!\n");
                    }

                    if (--unpack <= 0) {
                        PycRef<ASTNode> tup = stack.top();
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        if (curblock->blktype() == ASTBlock::BLK_FOR
                                && !curblock->inited()) {
                            curblock.cast<ASTIterBlock>()->setIndex(tup);
                        } else {
                            curblock->append(new ASTStore(seq, tup));
                        }
                    }
                } else {
                    PycRef<ASTNode> value = stack.top();
                    stack.pop();

                    PycRef<PycString> varname = code->getName(operand);
                    if (varname->value()[0] == '_' && varname->value()[1] == '[') {
                        /* Don't show stores of list comp append objects. */
                        break;
                    }

                    PycRef<ASTNode> name = new ASTName(varname);

                    if (curblock->blktype() == ASTBlock::BLK_FOR
                            && !curblock->inited()) {
                        curblock.cast<ASTIterBlock>()->setIndex(name);
                    } else if (stack.top()->type() == ASTNode::NODE_IMPORT) {
                        PycRef<ASTImport> import = stack.top().cast<ASTImport>();

                        import->add_store(new ASTStore(value, name));
                    } else if (curblock->blktype() == ASTBlock::BLK_WITH
                               && !curblock->inited()) {
                        curblock.cast<ASTWithBlock>()->setExpr(value);
                        curblock.cast<ASTWithBlock>()->setVar(name);
                    } else {
                        curblock->append(new ASTStore(value, name));

                        if (value->type() == ASTNode::NODE_INVALID)
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

                curblock->append(new ASTStore(value, new ASTSubscr(dest, new ASTSlice(ASTSlice::SLICE2, Node_NULL, lower))));
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
                    if (tup->type() == ASTNode::NODE_TUPLE) {
                        stack.pop();

                        PycRef<ASTTuple> tuple = tup.cast<ASTTuple>();
                        tuple->add(save);

                        stack.push(tuple.cast<ASTNode>());
                    } else {
                        fprintf(stderr, "Something TERRIBLE happened!\n");
                    }

                    if (--unpack <= 0) {
                        PycRef<ASTNode> tup = stack.top();
                        stack.pop();
                        PycRef<ASTNode> seq = stack.top();
                        stack.pop();

                        curblock->append(new ASTStore(seq, tup));
                    }
                } else {
                    PycRef<ASTNode> subscr = stack.top();
                    stack.pop();
                    PycRef<ASTNode> dest = stack.top();
                    stack.pop();
                    PycRef<ASTNode> src = stack.top();
                    stack.pop();

                    if (dest->type() == ASTNode::NODE_MAP) {
                        dest.cast<ASTMap>()->add(subscr, src);
                    } else {
                        curblock->append(new ASTStore(src, new ASTSubscr(dest, subscr)));
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

                ASTTuple::value_t vals;

                stack.push(new ASTTuple(vals));
            }
            break;
        case Pyc::YIELD_VALUE:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                curblock->append(new ASTReturn(value, ASTReturn::YIELD));
            }
            break;
        default:
            fprintf(stderr, "Unsupported opcode: %s\n", Pyc::OpcodeName(opcode & 0xFF));
            cleanBuild = false;
            return new ASTNodeList(defblock->nodes());
        }

        else_pop =  ( (curblock->blktype() == ASTBlock::BLK_ELSE)
                      || (curblock->blktype() == ASTBlock::BLK_IF)
                      || (curblock->blktype() == ASTBlock::BLK_ELIF) )
                 && (curblock->end() == pos);
    }

    if (stack_hist.size()) {
        fprintf(stderr, "Warning: Stack history is not empty!\n");

        while (stack_hist.size()) {
            stack_hist.pop();
        }
    }

    if (blocks.size() > 1) {
        fprintf(stderr, "Warning: block stack is not empty!\n");

        while (blocks.size() > 1) {
            PycRef<ASTBlock> tmp = blocks.top();
            blocks.pop();

            blocks.top()->append(tmp.cast<ASTNode>());
        }
    }

    cleanBuild = true;
    return new ASTNodeList(defblock->nodes());
}

static int cmp_prec(PycRef<ASTNode> parent, PycRef<ASTNode> child)
{
    /* Determine whether the parent has higher precedence than therefore
       child, so we don't flood the source code with extraneous parens.
       Else we'd have expressions like (((a + b) + c) + d) when therefore
       equivalent, a + b + c + d would suffice. */

    if (parent->type() == ASTNode::NODE_UNARY && parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT)
        return 1;   // Always parenthesize not(x)
    if (child->type() == ASTNode::NODE_BINARY) {
        PycRef<ASTBinary> binChild = child.cast<ASTBinary>();
        if (parent->type() == ASTNode::NODE_BINARY)
            return binChild->op() - parent.cast<ASTBinary>()->op();
        else if (parent->type() == ASTNode::NODE_COMPARE)
            return (binChild->op() == ASTBinary::BIN_LOG_AND ||
                    binChild->op() == ASTBinary::BIN_LOG_OR) ? 1 : -1;
        else if (parent->type() == ASTNode::NODE_UNARY)
            return (binChild->op() == ASTBinary::BIN_POWER) ? -1 : 1;
    } else if (child->type() == ASTNode::NODE_UNARY) {
        PycRef<ASTUnary> unChild = child.cast<ASTUnary>();
        if (parent->type() == ASTNode::NODE_BINARY) {
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
        } else if (parent->type() == ASTNode::NODE_COMPARE) {
            return (unChild->op() == ASTUnary::UN_NOT) ? 1 : -1;
        } else if (parent->type() == ASTNode::NODE_UNARY) {
            return unChild->op() - parent.cast<ASTUnary>()->op();
        }
    } else if (child->type() == ASTNode::NODE_COMPARE) {
        PycRef<ASTCompare> cmpChild = child.cast<ASTCompare>();
        if (parent->type() == ASTNode::NODE_BINARY)
            return (parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_AND ||
                    parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_OR) ? -1 : 1;
        else if (parent->type() == ASTNode::NODE_COMPARE)
            return cmpChild->op() - parent.cast<ASTCompare>()->op();
        else if (parent->type() == ASTNode::NODE_UNARY)
            return (parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT) ? -1 : 1;
    }

    /* For normal nodes, don't parenthesize anything */
    return -1;
}

static void print_ordered(PycRef<ASTNode> parent, PycRef<ASTNode> child,
                          PycModule* mod)
{
    if (child->type() == ASTNode::NODE_BINARY ||
        child->type() == ASTNode::NODE_COMPARE) {
        if (cmp_prec(parent, child) > 0) {
            fprintf(pyc_output, "(");
            print_src(child, mod);
            fprintf(pyc_output, ")");
        } else {
            print_src(child, mod);
        }
    } else if (child->type() == ASTNode::NODE_UNARY) {
        if (cmp_prec(parent, child) > 0) {
            fprintf(pyc_output, "(");
            print_src(child, mod);
            fprintf(pyc_output, ")");
        } else {
            print_src(child, mod);
        }
    } else {
        print_src(child, mod);
    }
}

static void start_line(int indent)
{
    if (inPrint || inLambda)
        return;
    for (int i=0; i<indent; i++)
        fprintf(pyc_output, "    ");
}

static void end_line()
{
    if (inPrint || inLambda)
        return;
    fprintf(pyc_output, "\n");
}

int cur_indent = -1;
static void print_block(PycRef<ASTBlock> blk, PycModule* mod) {
    ASTBlock::list_t lines = blk->nodes();

    if (lines.size() == 0) {
        PycRef<ASTNode> pass = new ASTNode(ASTNode::NODE_PASS);
        start_line(cur_indent);
        print_src(pass, mod);
    }

    for (ASTBlock::list_t::const_iterator ln = lines.begin(); ln != lines.end();) {
        if ((*ln).cast<ASTNode>()->type() != ASTNode::NODE_NODELIST) {
            start_line(cur_indent);
        }
        print_src(*ln, mod);
        if (++ln != lines.end()) {
            end_line();
        }
    }
}

void print_src(PycRef<ASTNode> node, PycModule* mod)
{
    if (node == Node_NULL) {
        fprintf(pyc_output, "None");
        cleanBuild = true;
        return;
    }

    switch (node->type()) {
    case ASTNode::NODE_BINARY:
    case ASTNode::NODE_COMPARE:
        {
            PycRef<ASTBinary> bin = node.cast<ASTBinary>();
            print_ordered(node, bin->left(), mod);
            fprintf(pyc_output, "%s", bin->op_str());
            print_ordered(node, bin->right(), mod);
        }
        break;
    case ASTNode::NODE_UNARY:
        {
            PycRef<ASTUnary> un = node.cast<ASTUnary>();
            fprintf(pyc_output, "%s", un->op_str());
            print_ordered(node, un->operand(), mod);
        }
        break;
    case ASTNode::NODE_CALL:
        {
            PycRef<ASTCall> call = node.cast<ASTCall>();
            print_src(call->func(), mod);
            fprintf(pyc_output, "(");
            bool first = true;
            for (ASTCall::pparam_t::const_iterator p = call->pparams().begin(); p != call->pparams().end(); ++p) {
                if (!first)
                    fprintf(pyc_output, ", ");
                print_src(*p, mod);
                first = false;
            }
            for (ASTCall::kwparam_t::const_iterator p = call->kwparams().begin(); p != call->kwparams().end(); ++p) {
                if (!first)
                    fprintf(pyc_output, ", ");
                fprintf(pyc_output, "%s = ", p->first.cast<ASTName>()->name()->value());
                print_src(p->second, mod);
                first = false;
            }
            if (call->hasVar()) {
                if (!first)
                    fprintf(pyc_output, ", ");
                fprintf(pyc_output, "*");
                print_src(call->var(), mod);
                first = false;
            }
            if (call->hasKW()) {
                if (!first)
                    fprintf(pyc_output, ", ");
                fprintf(pyc_output, "**");
                print_src(call->var(), mod);
                first = false;
            }
            fprintf(pyc_output, ")");
        }
        break;
    case ASTNode::NODE_DELETE:
        {
            fprintf(pyc_output, "del ");
            print_src(node.cast<ASTDelete>()->value(), mod);
        }
        break;
    case ASTNode::NODE_EXEC:
        {
            PycRef<ASTExec> exec = node.cast<ASTExec>();
            fprintf(pyc_output, "exec ");
            print_src(exec->statement(), mod);

            if (exec->globals() != Node_NULL) {
                fprintf(pyc_output, " in ");
                print_src(exec->globals(), mod);

                if (exec->locals() != Node_NULL
                        && exec->globals() != exec->locals()) {
                    fprintf(pyc_output, ", ");
                    print_src(exec->locals(), mod);
                }
            }
        }
        break;
    case ASTNode::NODE_KEYWORD:
        fprintf(pyc_output, "%s", node.cast<ASTKeyword>()->word_str());
        break;
    case ASTNode::NODE_LIST:
        {
            ASTList::value_t values = node.cast<ASTList>()->values();
            fprintf(pyc_output, "[");
            bool first = true;
            cur_indent++;
            for (ASTList::value_t::const_iterator b = values.begin(); b != values.end(); ++b) {
                if (first)
                    fprintf(pyc_output, "\n");
                else
                    fprintf(pyc_output, ",\n");
                start_line(cur_indent);
                print_src(*b, mod);
                first = false;
            }
            cur_indent--;
            fprintf(pyc_output, "]");
        }
        break;
    case ASTNode::NODE_COMPREHENSION:
        {
            PycRef<ASTComprehension> comp = node.cast<ASTComprehension>();
            ASTComprehension::generator_t values = comp->generators();

            fprintf(pyc_output, "[ ");
            print_src(comp->result(), mod);

            for (ASTComprehension::generator_t::const_iterator it = values.begin(); it != values.end(); ++it) {
                fprintf(pyc_output, " for ");
                print_src((*it)->index(), mod);
                fprintf(pyc_output, " in ");
                print_src((*it)->iter(), mod);
            }
            fprintf(pyc_output, " ]");
        }
        break;
    case ASTNode::NODE_MAP:
        {
            ASTMap::map_t values = node.cast<ASTMap>()->values();
            fprintf(pyc_output, "{");
            bool first = true;
            cur_indent++;
            for (ASTMap::map_t::const_iterator b = values.begin(); b != values.end(); ++b) {
                if (first)
                    fprintf(pyc_output, "\n");
                else
                    fprintf(pyc_output, ",\n");
                start_line(cur_indent);
                print_src(b->first, mod);
                fprintf(pyc_output, ": ");
                print_src(b->second, mod);
                first = false;
            }
            cur_indent--;
            fprintf(pyc_output, " }");
        }
        break;
    case ASTNode::NODE_NAME:
        fprintf(pyc_output, "%s", node.cast<ASTName>()->name()->value());
        break;
    case ASTNode::NODE_NODELIST:
        {
            cur_indent++;
            ASTNodeList::list_t lines = node.cast<ASTNodeList>()->nodes();
            for (ASTNodeList::list_t::const_iterator ln = lines.begin(); ln != lines.end(); ++ln) {
                if ((*ln).cast<ASTNode>()->type() != ASTNode::NODE_NODELIST) {
                    start_line(cur_indent);
                }
                print_src(*ln, mod);
                end_line();
            }
            cur_indent--;
        }
        break;
    case ASTNode::NODE_BLOCK:
        {
            if (node.cast<ASTBlock>()->blktype() == ASTBlock::BLK_ELSE
                    && node.cast<ASTBlock>()->size() == 0)
                break;

            if (node.cast<ASTBlock>()->blktype() == ASTBlock::BLK_CONTAINER) {
                end_line();
                PycRef<ASTBlock> blk = node.cast<ASTBlock>();
                print_block(blk, mod);
                end_line();
                break;
            }
            inPrint = false;

            fprintf(pyc_output, "%s", node.cast<ASTBlock>()->type_str());
            PycRef<ASTBlock> blk = node.cast<ASTBlock>();
            if (blk->blktype() == ASTBlock::BLK_IF
                    || blk->blktype() == ASTBlock::BLK_ELIF
                    || blk->blktype() == ASTBlock::BLK_WHILE) {
                if (blk.cast<ASTCondBlock>()->negative())
                    fprintf(pyc_output, " not ");
                else
                    fprintf(pyc_output, " ");

                print_src(blk.cast<ASTCondBlock>()->cond(), mod);
            } else if (blk->blktype() == ASTBlock::BLK_FOR) {
                fprintf(pyc_output, " ");
                print_src(blk.cast<ASTIterBlock>()->index(), mod);
                fprintf(pyc_output, " in ");
                print_src(blk.cast<ASTIterBlock>()->iter(), mod);
            } else if (blk->blktype() == ASTBlock::BLK_EXCEPT &&
                    blk.cast<ASTCondBlock>()->cond() != Node_NULL) {
                fprintf(pyc_output, " ");
                print_src(blk.cast<ASTCondBlock>()->cond(), mod);
            } else if (blk->blktype() == ASTBlock::BLK_WITH) {
                fprintf(pyc_output, " ");
                print_src(blk.cast<ASTWithBlock>()->expr(), mod);
                PycRef<ASTNode> var = blk.cast<ASTWithBlock>()->var();
                if (var != Node_NULL) {
                    fprintf(pyc_output, " as ");
                    print_src(var, mod);
                }
            }
            fprintf(pyc_output, ":\n");

            cur_indent++;
            print_block(blk, mod);
            if (inPrint) {
                fprintf(pyc_output, ",");
            }
            cur_indent--;
            inPrint = false;
        }
        break;
    case ASTNode::NODE_OBJECT:
        {
            PycRef<PycObject> obj = node.cast<ASTObject>()->object();
            if (obj->type() == PycObject::TYPE_CODE) {
                PycRef<PycCode> code = obj.cast<PycCode>();
                decompyle(code, mod);
            } else {
                print_const(obj, mod);
            }
        }
        break;
    case ASTNode::NODE_PASS:
        fprintf(pyc_output, "pass");
        break;
    case ASTNode::NODE_PRINT:
        if (node.cast<ASTPrint>()->value() == Node_NULL) {
            if (!inPrint) {
                fprintf(pyc_output, "print ");
                if (node.cast<ASTPrint>()->stream() != Node_NULL) {
                    fprintf(pyc_output, ">>");
                    print_src(node.cast<ASTPrint>()->stream(), mod);
                }
            }
            inPrint = false;
        } else if (!inPrint) {
            fprintf(pyc_output, "print ");
            if (node.cast<ASTPrint>()->stream() != Node_NULL) {
                fprintf(pyc_output, ">>");
                print_src(node.cast<ASTPrint>()->stream(), mod);
                fprintf(pyc_output, ", ");
            }
            print_src(node.cast<ASTPrint>()->value(), mod);
            inPrint = true;
        } else {
            fprintf(pyc_output, ", ");
            print_src(node.cast<ASTPrint>()->value(), mod);
        }
        break;
    case ASTNode::NODE_RAISE:
        {
            PycRef<ASTRaise> raise = node.cast<ASTRaise>();
            fprintf(pyc_output, "raise ");
            bool first = true;
            for (ASTRaise::param_t::const_iterator p = raise->params().begin(); p != raise->params().end(); ++p) {
                if (!first)
                    fprintf(pyc_output, ", ");
                print_src(*p, mod);
                first = false;
            }
        }
        break;
    case ASTNode::NODE_RETURN:
        {
            PycRef<ASTReturn> ret = node.cast<ASTReturn>();
            if (!inLambda) {
                switch (ret->rettype()) {
                    case ASTReturn::RETURN:
                        fprintf(pyc_output, "return ");
                        break;
                    case ASTReturn::YIELD:
                        fprintf(pyc_output, "yield ");
                        break;
                }
            }
            print_src(ret->value(), mod);
        }
        break;
    case ASTNode::NODE_SLICE:
        {
            PycRef<ASTSlice> slice = node.cast<ASTSlice>();

            if (slice->op() & ASTSlice::SLICE1) {
                print_src(slice->left(), mod);
            }
            fprintf(pyc_output, ":");
            if (slice->op() & ASTSlice::SLICE2) {
                print_src(slice->right(), mod);
            }
        }
        break;
    case ASTNode::NODE_IMPORT:
        {
            PycRef<ASTImport> import = node.cast<ASTImport>();
            if (import->stores().size()) {
                ASTImport::list_t stores = import->stores();

                fprintf(pyc_output, "from ");
                if (import->name()->type() == ASTNode::NODE_IMPORT)
                    print_src(import->name().cast<ASTImport>()->name(), mod);
                else
                    print_src(import->name(), mod);
                fprintf(pyc_output, " import ");

                ASTImport::list_t::const_iterator ii = stores.begin();
                if (stores.size() == 1) {
                    print_src((*ii)->src(), mod);

                    if ((*ii)->src().cast<ASTName>()->name()->value() != (*ii)->dest().cast<ASTName>()->name()->value()) {
                        fprintf(pyc_output, " as ");
                        print_src((*ii)->dest(), mod);
                    }
                } else {
                    bool first = true;
                    for (; ii != stores.end(); ++ii) {
                        if (!first)
                            fprintf(pyc_output, ", ");
                        print_src((*ii)->src(), mod);
                        first = false;

                        if ((*ii)->src().cast<ASTName>()->name()->value() != (*ii)->dest().cast<ASTName>()->name()->value()) {
                            fprintf(pyc_output, " as ");
                            print_src((*ii)->dest(), mod);
                        }
                    }
                }
            } else {
                fprintf(pyc_output, "import ");
                print_src(import->name(), mod);
            }
        }
        break;
    case ASTNode::NODE_FUNCTION:
        {
            /* Actual named functions are NODE_STORE with a name */
            fprintf(pyc_output, "lambda ");
            PycRef<ASTNode> code = node.cast<ASTFunction>()->code();
            PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
            ASTFunction::defarg_t defargs = node.cast<ASTFunction>()->defargs();
            ASTFunction::defarg_t::iterator da = defargs.begin();
            for (int i=0; i<code_src->argCount(); i++) {
                if (i > 0)
                    fprintf(pyc_output, ", ");
                fprintf(pyc_output, "%s", code_src->getVarName(i)->value());
                if ((code_src->argCount() - i) <= (int)defargs.size()) {
                    fprintf(pyc_output, " = ");
                    print_src(*da++, mod);
                }
            }
            fprintf(pyc_output, ": ");

            inLambda = true;
            print_src(code, mod);
            inLambda = false;
        }
        break;
    case ASTNode::NODE_STORE:
        {
            PycRef<ASTNode> src = node.cast<ASTStore>()->src();
            PycRef<ASTNode> dest = node.cast<ASTStore>()->dest();
            if (src->type() == ASTNode::NODE_FUNCTION) {
                PycRef<ASTNode> code = src.cast<ASTFunction>()->code();
                PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
                bool isLambda = false;

                if (strcmp(code_src->name()->value(), "<lambda>") == 0) {
                    fprintf(pyc_output, "\n");
                    start_line(cur_indent);
                    print_src(dest, mod);
                    fprintf(pyc_output, " = lambda ");
                    isLambda = true;
                } else {
                    fprintf(pyc_output, "\n");
                    start_line(cur_indent);
                    fprintf(pyc_output, "def ");
                    print_src(dest, mod);
                    fprintf(pyc_output, "(");
                }

                ASTFunction::defarg_t defargs = src.cast<ASTFunction>()->defargs();
                ASTFunction::defarg_t::iterator da = defargs.begin();
                bool first = true;
                for (int i=0; i<code_src->argCount(); i++) {
                    if (!first)
                        fprintf(pyc_output, ", ");
                    fprintf(pyc_output, "%s", code_src->getVarName(i)->value());
                    if ((code_src->argCount() - i) <= (int)defargs.size()) {
                        fprintf(pyc_output, " = ");
                        print_src(*da++, mod);
                    }
                    first = false;
                }
                if (code_src->flags() & PycCode::CO_VARARGS) {
                    if (!first)
                        fprintf(pyc_output, ", ");
                    fprintf(pyc_output, "*%s", code_src->getVarName(code_src->argCount())->value());
                    first = false;
                }
                if (code_src->flags() & PycCode::CO_VARKEYWORDS) {
                    if (!first)
                        fprintf(pyc_output, ", ");

                    int idx = code_src->argCount();
                    if (code_src->flags() & PycCode::CO_VARARGS) {
                        idx++;
                    }
                    fprintf(pyc_output, "**%s", code_src->getVarName(idx)->value());
                    first = false;
                }

                if (isLambda) {
                    fprintf(pyc_output, ": ");
                } else {
                    fprintf(pyc_output, "):\n");
                    printGlobals = true;
                }

                bool preLambda = inLambda;
                inLambda |= isLambda;

                print_src(code, mod);

                inLambda = preLambda;
            } else if (src->type() == ASTNode::NODE_CLASS) {
                fprintf(pyc_output, "\n");
                start_line(cur_indent);
                fprintf(pyc_output, "class ");
                print_src(dest, mod);
                PycRef<ASTTuple> bases = src.cast<ASTClass>()->bases().cast<ASTTuple>();
                if (bases->values().size() > 0) {
                    fprintf(pyc_output, "(");
                    bool first = true;
                    for (ASTTuple::value_t::const_iterator b = bases->values().begin(); b != bases->values().end(); ++b) {
                        if (!first)
                            fprintf(pyc_output, ", ");
                        print_src(*b, mod);
                        first = false;
                    }
                    fprintf(pyc_output, "):\n");
                } else {
                    // Don't put parens if there are no base classes
                    fprintf(pyc_output, ":\n");
                }
                PycRef<ASTNode> code = src.cast<ASTClass>()->code().cast<ASTCall>()
                                       ->func().cast<ASTFunction>()->code();
                print_src(code, mod);
            } else if (src->type() == ASTNode::NODE_IMPORT) {
                PycRef<ASTImport> import = src.cast<ASTImport>();
                if (import->fromlist() != Node_NULL) {
                    PycRef<PycObject> fromlist = import->fromlist().cast<ASTObject>()->object();
                    if (fromlist != Pyc_None) {
                        fprintf(pyc_output, "from ");
                        if (import->name()->type() == ASTNode::NODE_IMPORT)
                            print_src(import->name().cast<ASTImport>()->name(), mod);
                        else
                            print_src(import->name(), mod);
                        fprintf(pyc_output, " import ");
                        if (fromlist->type() == PycObject::TYPE_TUPLE ||
                                fromlist->type() == PycObject::TYPE_SMALL_TUPLE) {
                            bool first = true;
                            PycTuple::value_t::const_iterator ii = fromlist.cast<PycTuple>()->values().begin();
                            for (; ii != fromlist.cast<PycTuple>()->values().end(); ++ii) {
                                if (!first)
                                    fprintf(pyc_output, ", ");
                                fprintf(pyc_output, "%s", ii->cast<PycString>()->value());
                                first = false;
                            }
                        } else {
                            fprintf(pyc_output, "%s", fromlist.cast<PycString>()->value());
                        }
                    } else {
                        fprintf(pyc_output, "import ");
                        print_src(import->name(), mod);
                    }
                } else {
                    fprintf(pyc_output, "import ");
                    PycRef<ASTNode> import_name = import->name();
                    print_src(import_name, mod);
                    if (!dest.cast<ASTName>()->name()->isEqual(import_name.cast<ASTName>()->name().cast<PycObject>())) {
                        fprintf(pyc_output, " as ");
                        print_src(dest, mod);
                    }
                }
            } else {
                if (src->type() == ASTNode::NODE_BINARY &&
                        src.cast<ASTBinary>()->is_inplace() == true) {
                    print_src(src, mod);
                    break;
                }

                if (dest->type() == ASTNode::NODE_NAME &&
                    dest.cast<ASTName>()->name()->isEqual("__doc__")) {
                    if (src->type() == ASTNode::NODE_OBJECT) {
                        PycRef<PycObject> obj = src.cast<ASTObject>()->object();
                        if (obj->type() == PycObject::TYPE_STRING)
                            OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0, true);
                        else if (obj->type() == PycObject::TYPE_UNICODE)
                            OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u', true);
                        else if (obj->type() == PycObject::TYPE_INTERNED ||
                                 obj->type() == PycObject::TYPE_STRINGREF ||
                                 obj->type() == PycObject::TYPE_ASCII ||
                                 obj->type() == PycObject::TYPE_ASCII_INTERNED ||
                                 obj->type() == PycObject::TYPE_SHORT_ASCII ||
                                 obj->type() == PycObject::TYPE_SHORT_ASCII_INTERNED)
                            OutputString(obj.cast<PycString>(), 0, true);
                    } else {
                        print_src(dest, mod);
                        fprintf(pyc_output, " = ");
                        print_src(src, mod);
                    }
                } else {
                    print_src(dest, mod);
                    fprintf(pyc_output, " = ");
                    print_src(src, mod);
                }
            }
        }
        break;
    case ASTNode::NODE_SUBSCR:
        {
            print_src(node.cast<ASTSubscr>()->name(), mod);
            fprintf(pyc_output, "[");
            print_src(node.cast<ASTSubscr>()->key(), mod);
            fprintf(pyc_output, "]");
        }
        break;
    case ASTNode::NODE_CONVERT:
        {
            fprintf(pyc_output, "`");
            print_src(node.cast<ASTConvert>()->name(), mod);
            fprintf(pyc_output, "`");
        }
        break;
    case ASTNode::NODE_TUPLE:
        {
            ASTTuple::value_t values = node.cast<ASTTuple>()->values();
            fprintf(pyc_output, "(");
            bool first = true;
            for (ASTTuple::value_t::const_iterator b = values.begin(); b != values.end(); ++b) {
                if (!first)
                    fprintf(pyc_output, ", ");
                print_src(*b, mod);
                first = false;
            }
            if (values.size() == 1)
                fprintf(pyc_output, ",)");
            else
                fprintf(pyc_output, ")");
        }
        break;
    default:
        fprintf(pyc_output, "<NODE:%d>", node->type());
        fprintf(stderr, "Unsupported Node type: %d\n", node->type());
        cleanBuild = false;
        return;
    }

    cleanBuild = true;
}

void decompyle(PycRef<PycCode> code, PycModule* mod)
{
    PycRef<ASTNode> source = BuildFromCode(code, mod);

    PycRef<ASTNodeList> clean = source.cast<ASTNodeList>();
    if (cleanBuild) {
        // The Python compiler adds some stuff that we don't really care
        // about, and would add extra code for re-compilation anyway.
        // We strip these lines out here, and then add a "pass" statement
        // if the cleaned up code is empty
        if (clean->nodes().front()->type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->src()->type() == ASTNode::NODE_NAME &&
                store->dest()->type() == ASTNode::NODE_NAME) {
                PycRef<ASTName> src = store->src().cast<ASTName>();
                PycRef<ASTName> dest = store->dest().cast<ASTName>();
                if (src->name()->isEqual("__name__") &&
                    dest->name()->isEqual("__module__")) {
                    // __module__ = __name__
                    clean->removeFirst();
                }
            }
        }
        if (clean->nodes().back()->type() == ASTNode::NODE_RETURN) {
            PycRef<ASTReturn> ret = clean->nodes().back().cast<ASTReturn>();

            if (ret->value() == Node_NULL || ret->value()->type() == ASTNode::NODE_LOCALS) {
                clean->removeLast();  // Always an extraneous return statement
            }
        }
    }
    // This is outside the clean check so a source block will always
    // be compilable, even if decompylation failed.
    if (clean->nodes().size() == 0)
        clean->append(new ASTNode(ASTNode::NODE_PASS));

    inPrint = false;
    bool part1clean = cleanBuild;

    PycCode::globals_t globs = code->getGlobals();
    if (printGlobals && globs.size()) {
        start_line(cur_indent+1);
        fprintf(pyc_output, "global ");
        bool first = true;
        PycCode::globals_t::iterator it;
        for (it = globs.begin(); it != globs.end(); ++it) {
            if (!first)
                fprintf(pyc_output, ", ");
            fprintf(pyc_output, "%s", (*it)->value());
            first = false;
        }
        fprintf(pyc_output, "\n");
        printGlobals = false;
    }

    print_src(source, mod);

    if (!cleanBuild || !part1clean) {
        start_line(cur_indent);
        fprintf(pyc_output, "# WARNING: Decompyle incomplete\n");
    }
}
