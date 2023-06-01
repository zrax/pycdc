#include "ASTNode.h"
#include "bytecode.h"

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
        " >>= ", " &= ", " ^= ", " |= ", " //= ", " @= ", " <INVALID> "

    };
    return s_op_strings[op()];
}

ASTBinary::BinOp ASTBinary::from_opcode(int opcode)
{
    switch (opcode) {
    case Pyc::BINARY_ADD:
        return BIN_ADD;
    case Pyc::BINARY_AND:
        return BIN_AND;
    case Pyc::BINARY_DIVIDE:
        return BIN_DIVIDE;
    case Pyc::BINARY_FLOOR_DIVIDE:
        return BIN_FLOOR_DIVIDE;
    case Pyc::BINARY_LSHIFT:
        return BIN_LSHIFT;
    case Pyc::BINARY_MODULO:
        return BIN_MODULO;
    case Pyc::BINARY_MULTIPLY:
        return BIN_MULTIPLY;
    case Pyc::BINARY_OR:
        return BIN_OR;
    case Pyc::BINARY_POWER:
        return BIN_POWER;
    case Pyc::BINARY_RSHIFT:
        return BIN_RSHIFT;
    case Pyc::BINARY_SUBTRACT:
        return BIN_SUBTRACT;
    case Pyc::BINARY_TRUE_DIVIDE:
        return BIN_DIVIDE;
    case Pyc::BINARY_XOR:
        return BIN_XOR;
    case Pyc::BINARY_MATRIX_MULTIPLY:
        return BIN_MAT_MULTIPLY;
    case Pyc::INPLACE_ADD:
        return BIN_IP_ADD;
    case Pyc::INPLACE_AND:
        return BIN_IP_AND;
    case Pyc::INPLACE_DIVIDE:
        return BIN_IP_DIVIDE;
    case Pyc::INPLACE_FLOOR_DIVIDE:
        return BIN_IP_FLOOR_DIVIDE;
    case Pyc::INPLACE_LSHIFT:
        return BIN_IP_LSHIFT;
    case Pyc::INPLACE_MODULO:
        return BIN_IP_MODULO;
    case Pyc::INPLACE_MULTIPLY:
        return BIN_IP_MULTIPLY;
    case Pyc::INPLACE_OR:
        return BIN_IP_OR;
    case Pyc::INPLACE_POWER:
        return BIN_IP_POWER;
    case Pyc::INPLACE_RSHIFT:
        return BIN_IP_RSHIFT;
    case Pyc::INPLACE_SUBTRACT:
        return BIN_IP_SUBTRACT;
    case Pyc::INPLACE_TRUE_DIVIDE:
        return BIN_IP_DIVIDE;
    case Pyc::INPLACE_XOR:
        return BIN_IP_XOR;
    case Pyc::INPLACE_MATRIX_MULTIPLY:
        return BIN_IP_MAT_MULTIPLY;
    default:
        return BIN_INVALID;
    }
}

ASTBinary::BinOp ASTBinary::from_binary_op(int operand)
{
    switch (operand) {
    case 0:
        return BIN_ADD;
    case 1:
        return BIN_AND;
    case 2:
        return BIN_FLOOR_DIVIDE;
    case 3:
        return BIN_LSHIFT;
    case 4:
        return BIN_MAT_MULTIPLY;
    case 5:
        return BIN_MULTIPLY;
    case 6:
        return BIN_MODULO;
    case 7:
        return BIN_OR;
    case 8:
        return BIN_POWER;
    case 9:
        return BIN_RSHIFT;
    case 10:
        return BIN_SUBTRACT;
    case 11:
        return BIN_DIVIDE;
    case 12:
        return BIN_XOR;
    case 13:
        return BIN_IP_ADD;
    case 14:
        return BIN_IP_AND;
    case 15:
        return BIN_IP_FLOOR_DIVIDE;
    case 16:
        return BIN_IP_LSHIFT;
    case 17:
        return BIN_MAT_MULTIPLY;
    case 18:
        return BIN_IP_MULTIPLY;
    case 19:
        return BIN_IP_MODULO;
    case 20:
        return BIN_IP_OR;
    case 21:
        return BIN_IP_POWER;
    case 22:
        return BIN_IP_RSHIFT;
    case 23:
        return BIN_IP_SUBTRACT;
    case 24:
        return BIN_IP_DIVIDE;
    case 25:
        return BIN_IP_XOR;
    default:
        return BIN_INVALID; // Return BIN_INVALID for out-of-range operand
    }
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
