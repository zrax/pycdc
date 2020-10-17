#include "ASTNode.h"

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
        "finally", "while", "for", "with",
    };
    return s_type_strings[blktype()];
}
/* ASTFormattedValue */
// This must be a triple quote (''' or """), to handle interpolated string literals containing the opposite quote style.
// E.g. f'''{"interpolated "123' literal"}'''    -> valid.
// E.g. f"""{"interpolated "123' literal"}"""    -> valid.
// E.g. f'{"interpolated "123' literal"}'        -> invalid, unescaped quotes in literal.
// E.g. f'{"interpolated \"123\' literal"}'      -> invalid, f-string expression does not allow backslash.
// NOTE: Nested f-strings not supported.
const char* ASTFormattedValue::F_STRING_QUOTE = "'''";
