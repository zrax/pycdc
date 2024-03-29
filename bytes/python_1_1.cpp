#include "bytecode_map.h"

BEGIN_MAP(1, 1)
    MAP_OP(0, STOP_CODE)
    MAP_OP(1, POP_TOP)
    MAP_OP(2, ROT_TWO)
    MAP_OP(3, ROT_THREE)
    MAP_OP(4, DUP_TOP)
    MAP_OP(10, UNARY_POSITIVE)
    MAP_OP(11, UNARY_NEGATIVE)
    MAP_OP(12, UNARY_NOT)
    MAP_OP(13, UNARY_CONVERT)
    MAP_OP(14, UNARY_CALL)
    MAP_OP(15, UNARY_INVERT)
    MAP_OP(20, BINARY_MULTIPLY)
    MAP_OP(21, BINARY_DIVIDE)
    MAP_OP(22, BINARY_MODULO)
    MAP_OP(23, BINARY_ADD)
    MAP_OP(24, BINARY_SUBTRACT)
    MAP_OP(25, BINARY_SUBSCR)
    MAP_OP(26, BINARY_CALL)
    MAP_OP(30, SLICE_0)
    MAP_OP(31, SLICE_1)
    MAP_OP(32, SLICE_2)
    MAP_OP(33, SLICE_3)
    MAP_OP(40, STORE_SLICE_0)
    MAP_OP(41, STORE_SLICE_1)
    MAP_OP(42, STORE_SLICE_2)
    MAP_OP(43, STORE_SLICE_3)
    MAP_OP(50, DELETE_SLICE_0)
    MAP_OP(51, DELETE_SLICE_1)
    MAP_OP(52, DELETE_SLICE_2)
    MAP_OP(53, DELETE_SLICE_3)
    MAP_OP(60, STORE_SUBSCR)
    MAP_OP(61, DELETE_SUBSCR)
    MAP_OP(62, BINARY_LSHIFT)
    MAP_OP(63, BINARY_RSHIFT)
    MAP_OP(64, BINARY_AND)
    MAP_OP(65, BINARY_XOR)
    MAP_OP(66, BINARY_OR)
    MAP_OP(70, PRINT_EXPR)
    MAP_OP(71, PRINT_ITEM)
    MAP_OP(72, PRINT_NEWLINE)
    MAP_OP(80, BREAK_LOOP)
    MAP_OP(81, RAISE_EXCEPTION)
    MAP_OP(82, LOAD_LOCALS)
    MAP_OP(83, RETURN_VALUE)
    MAP_OP(84, LOAD_GLOBALS)
    MAP_OP(85, EXEC_STMT)
    MAP_OP(86, BUILD_FUNCTION)
    MAP_OP(87, POP_BLOCK)
    MAP_OP(88, END_FINALLY)
    MAP_OP(89, BUILD_CLASS)
    MAP_OP(90, STORE_NAME_A)
    MAP_OP(91, DELETE_NAME_A)
    MAP_OP(92, UNPACK_TUPLE_A)
    MAP_OP(93, UNPACK_LIST_A)
    MAP_OP(94, UNPACK_ARG_A)
    MAP_OP(95, STORE_ATTR_A)
    MAP_OP(96, DELETE_ATTR_A)
    MAP_OP(97, STORE_GLOBAL_A)
    MAP_OP(98, DELETE_GLOBAL_A)
    MAP_OP(99, UNPACK_VARARG_A)
    MAP_OP(100, LOAD_CONST_A)
    MAP_OP(101, LOAD_NAME_A)
    MAP_OP(102, BUILD_TUPLE_A)
    MAP_OP(103, BUILD_LIST_A)
    MAP_OP(104, BUILD_MAP_A)
    MAP_OP(105, LOAD_ATTR_A)
    MAP_OP(106, COMPARE_OP_A)
    MAP_OP(107, IMPORT_NAME_A)
    MAP_OP(108, IMPORT_FROM_A)
    MAP_OP(109, ACCESS_MODE_A)
    MAP_OP(110, JUMP_FORWARD_A)
    MAP_OP(111, JUMP_IF_FALSE_A)
    MAP_OP(112, JUMP_IF_TRUE_A)
    MAP_OP(113, JUMP_ABSOLUTE_A)
    MAP_OP(114, FOR_LOOP_A)
    MAP_OP(115, LOAD_LOCAL_A)
    MAP_OP(116, LOAD_GLOBAL_A)
    MAP_OP(117, SET_FUNC_ARGS_A)
    MAP_OP(120, SETUP_LOOP_A)
    MAP_OP(121, SETUP_EXCEPT_A)
    MAP_OP(122, SETUP_FINALLY_A)
    MAP_OP(123, RESERVE_FAST_A)
    MAP_OP(124, LOAD_FAST_A)
    MAP_OP(125, STORE_FAST_A)
    MAP_OP(126, DELETE_FAST_A)
    MAP_OP(127, SET_LINENO_A)
END_MAP()
