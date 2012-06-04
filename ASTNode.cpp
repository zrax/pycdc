#include "ASTNode.h"

PycRef<ASTNode> Node_NULL = (ASTNode*)0;

/* ASTNodeList */
void ASTNodeList::removeLast()
{
    list_t::iterator it = m_nodes.end();
    --it;
    m_nodes.erase(it);
}

void ASTNodeList::removeFirst()
{
    list_t::iterator it = m_nodes.begin();
    m_nodes.erase(it);
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
        " << ", " >> ", " & ", " ^ ", " | ", " and ", " or ",
        " += ", " -= ", " *= ", " /= ", " %= ", " **= ", " <<= ",
        " >>= ", " &= ", " ^= ", " |= ", " //= "
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
        "break", "continue"
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
    list_t::iterator it = m_nodes.begin();
    m_nodes.erase(it);
}

const char* ASTBlock::type_str() const
{
    static const char* s_type_strings[] = {
        "", "if", "else", "elif", "try", "CONTAINER", "except",
        "finally", "while", "for", "with",
    };
    return s_type_strings[blktype()];
}
