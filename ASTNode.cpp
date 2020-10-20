#include "ASTNode.h"
#include <bytecode.h>

/* ASTNodeList */
void ASTNodeList::removeLast()
{
    list_t::iterator it = m_nodes.end();
    --it;
    m_nodes.erase(it);
}

void ASTNodeList::removeFirst()
{
    m_nodes.erase(m_nodes.cbegin());
}


/* ASTUnary */
const char* ASTUnary::op_str() const
{
    static const char* s_op_strings[] = {
        "+", "-", "~", "not "
    };
    return s_op_strings[op()];
}


/* ASTBinary */
const char* ASTBinary::op_str() const
{
    static const char* s_op_strings[] = {
        ".", " ** ", " * ", " / ", " // ", " % ", " + ", " - ",
        " << ", " >> ", " & ", " ^ ", " | ", " and ", " or ", " @ ",
        " += ", " -= ", " *= ", " /= ", " %= ", " **= ", " <<= ",
        " >>= ", " &= ", " ^= ", " |= ", " //= ", " @= ",
    };
    return s_op_strings[op()];
}


/* ASTCompare */
const char* ASTCompare::op_str() const
{
    static const char* s_cmp_strings[] = {
        " < ", " <= ", " == ", " != ", " > ", " >= ", " in ", " not in ", " is ", " is not ",
        "<EXCEPTION MATCH>", "<BAD>"
    };
    return s_cmp_strings[op()];
}


/* ASTKeyword */
const char* ASTKeyword::word_str() const
{
    static const char* s_word_strings[] = {
        "pass", "break", "continue"
    };
    return s_word_strings[key()];
}


/* ASTBlock */
void ASTBlock::removeLast()
{
    list_t::iterator it = m_nodes.end();
    --it;
    m_nodes.erase(it);
}

void ASTBlock::removeFirst()
{
    m_nodes.erase(m_nodes.begin());
}

const char* ASTBlock::type_str() const
{
    static const char* s_type_strings[] = {
        "", "if", "else", "elif", "try", "CONTAINER", "except",
        "finally", "while", "for", "with", "async for"
    };
    return s_type_strings[blktype()];
}


/* ASTIterBlock */
const int ASTIterBlock::ASYNCFOR_BOILER_READLOOPINDEX = 4;
const int ASTIterBlock::ASYNCFOR_BOILER_READLOOPCONTENTS = 11;
const int ASTIterBlock::ASYNCFOR_BOILER_OFFSETFROMEND = 12;
const int ASTIterBlock::ASYNCFOR_BOILER_FIRSTJUMP = 5;
const Pyc::Opcode ASTIterBlock::ASYNCFOR_BOILER_ALTJUMPOP = Pyc::JUMP_ABSOLUTE_A;

const std::vector<Pyc::Opcode> ASTIterBlock::ASYNCFOR_BOILERPLATE = {
    // Ignore all these after GET_AITER.
    Pyc::SETUP_EXCEPT_A,
    Pyc::GET_ANEXT,
    Pyc::LOAD_CONST_A,          // None
    Pyc::YIELD_FROM,

    // Process ops from here to load index variable.

    // Continue ignoring all these
    Pyc::POP_BLOCK,
    Pyc::JUMP_FORWARD_A,        // or JUMP_ABSOLUTE if pass
    Pyc::DUP_TOP,
    Pyc::LOAD_GLOBAL_A,         // StopAsyncIteration
    Pyc::COMPARE_OP_A,          // (<EXCEPTION MATCH>)
    Pyc::POP_JUMP_IF_TRUE_A,
    Pyc::END_FINALLY,

    // Actual code inside the loop begins here. 'Preamble' before this, 'Trailer' after.

    // After reading a JUMP at the address we are expecting, ignore the following:
    Pyc::POP_TOP,
    Pyc::POP_TOP,
    Pyc::POP_TOP,
    Pyc::POP_EXCEPT,
    Pyc::POP_TOP

    // Then a POP_BLOCK as per normal loop
};
