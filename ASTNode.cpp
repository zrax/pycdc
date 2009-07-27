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


/* ASTBinary */
const char* ASTBinary::op_str() const
{
    static const char* s_op_strings[] = {
        "**", "*", "/", "%", "+", "-", "<<", ">>", "&", "^", "|", "//",
        "<ATTR>"
    };
    return s_op_strings[op()];
}


/* ASTCompare */
const char* ASTCompare::op_str() const
{
    static const char* s_cmp_strings[] = {
        "<", "<=", "==", "!=", ">", ">=", "in", "not in", "is", "is not",
        "<EXCEPTION MATCH>", "<BAD>"
    };
    return s_cmp_strings[op()];
}
