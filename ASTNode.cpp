#include "ASTNode.h"

PycRef<ASTNode> Node_NULL = (ASTNode*)0;

/* ASTCompare */
const char* ASTCompare::op_str() const
{
    static const char* s_cmp_strings[] = {
        "<", "<=", "==", "!=", ">", ">=", "in", "not in", "is", "is not",
        "<EXCEPTION MATCH>", "<BAD>"
    };
    return s_cmp_strings[m_op];
}
