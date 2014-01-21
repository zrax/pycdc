#include "pyc_code.h"
#include "pyc_module.h"
#include "data.h"

namespace Pyc {

enum Opcode {
    /* No parameter word */
    STOP_CODE, POP_TOP, ROT_TWO, ROT_THREE, DUP_TOP, DUP_TOP_TWO,
    UNARY_POSITIVE, UNARY_NEGATIVE, UNARY_NOT, UNARY_CONVERT, UNARY_CALL,
    UNARY_INVERT, BINARY_POWER, BINARY_MULTIPLY, BINARY_DIVIDE, BINARY_MODULO,
    BINARY_ADD, BINARY_SUBTRACT, BINARY_SUBSCR, BINARY_CALL,
    SLICE_0, SLICE_1, SLICE_2, SLICE_3, STORE_SLICE_0, STORE_SLICE_1,
    STORE_SLICE_2, STORE_SLICE_3, DELETE_SLICE_0, DELETE_SLICE_1,
    DELETE_SLICE_2, DELETE_SLICE_3, STORE_SUBSCR, DELETE_SUBSCR,
    BINARY_LSHIFT, BINARY_RSHIFT, BINARY_AND, BINARY_XOR, BINARY_OR,
    PRINT_EXPR, PRINT_ITEM, PRINT_NEWLINE, BREAK_LOOP, RAISE_EXCEPTION,
    LOAD_LOCALS, RETURN_VALUE, LOAD_GLOBALS, EXEC_STMT, BUILD_FUNCTION,
    POP_BLOCK, END_FINALLY, BUILD_CLASS, ROT_FOUR, NOP, LIST_APPEND,
    BINARY_FLOOR_DIVIDE, BINARY_TRUE_DIVIDE, INPLACE_FLOOR_DIVIDE,
    INPLACE_TRUE_DIVIDE, STORE_MAP, INPLACE_ADD, INPLACE_SUBTRACT,
    INPLACE_MULTIPLY, INPLACE_DIVIDE, INPLACE_MODULO, INPLACE_POWER,
    GET_ITER, PRINT_ITEM_TO, PRINT_NEWLINE_TO, INPLACE_LSHIFT,
    INPLACE_RSHIFT, INPLACE_AND, INPLACE_XOR, INPLACE_OR, WITH_CLEANUP,
    IMPORT_STAR, YIELD_VALUE, LOAD_BUILD_CLASS, STORE_LOCALS,
    POP_EXCEPT, SET_ADD, YIELD_FROM,

    /* Has parameter word */
    PYC_HAVE_ARG,
    STORE_NAME_A = PYC_HAVE_ARG, DELETE_NAME_A, UNPACK_TUPLE_A,
    UNPACK_LIST_A, UNPACK_ARG_A, STORE_ATTR_A, DELETE_ATTR_A,
    STORE_GLOBAL_A, DELETE_GLOBAL_A, UNPACK_VARARG_A, LOAD_CONST_A,
    LOAD_NAME_A, BUILD_TUPLE_A, BUILD_LIST_A, BUILD_MAP_A, LOAD_ATTR_A,
    COMPARE_OP_A, IMPORT_NAME_A, IMPORT_FROM_A, JUMP_FORWARD_A,
    JUMP_IF_FALSE_A, JUMP_IF_TRUE_A, JUMP_ABSOLUTE_A, FOR_LOOP_A,
    LOAD_LOCAL_A, LOAD_GLOBAL_A, SET_FUNC_ARGS_A, SETUP_LOOP_A,
    SETUP_EXCEPT_A, SETUP_FINALLY_A, RESERVE_FAST_A, LOAD_FAST_A,
    STORE_FAST_A, DELETE_FAST_A, SET_LINENO_A, RAISE_VARARGS_A,
    CALL_FUNCTION_A, MAKE_FUNCTION_A, BUILD_SLICE_A, CALL_FUNCTION_VAR_A,
    CALL_FUNCTION_KW_A, CALL_FUNCTION_VAR_KW_A, UNPACK_SEQUENCE_A, FOR_ITER_A,
    DUP_TOPX_A, BUILD_SET_A, JUMP_IF_FALSE_OR_POP_A, JUMP_IF_TRUE_OR_POP_A,
    POP_JUMP_IF_FALSE_A, POP_JUMP_IF_TRUE_A, CONTINUE_LOOP_A, MAKE_CLOSURE_A,
    LOAD_CLOSURE_A, LOAD_DEREF_A, STORE_DEREF_A, DELETE_DEREF_A,
    EXTENDED_ARG_A, SETUP_WITH_A, SET_ADD_A, MAP_ADD_A, UNPACK_EX_A,
    LIST_APPEND_A, LOAD_CLASSDEREF_A,

    PYC_LAST_OPCODE,
    PYC_INVALID_OPCODE = -1,
};

const char* OpcodeName(int opcode);
int ByteToOpcode(int maj, int min, int opcode);

bool IsConstArg(int opcode);
bool IsNameArg(int opcode);
bool IsVarNameArg(int opcode);
bool IsCellArg(int opcode);
bool IsJumpOffsetArg(int opcode);

}

void print_const(PycRef<PycObject> obj, PycModule* mod);
void bc_next(PycBuffer& source, PycRef<PycCode> code, PycModule* mod, int& opcode, int& operand, int& pos, bool is_disasm);
void bc_disasm(PycRef<PycCode> code, PycModule* mod, int indent);
