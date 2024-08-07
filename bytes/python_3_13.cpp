#include "bytecode_map.h"

BEGIN_MAP(3, 13)
    MAP_OP(0, CACHE)
    MAP_OP(1, BEFORE_ASYNC_WITH)
    MAP_OP(2, BEFORE_WITH)
    MAP_OP(3, BINARY_OP_INPLACE_ADD_UNICODE)
    MAP_OP(4, BINARY_SLICE)
    MAP_OP(5, BINARY_SUBSCR)
    MAP_OP(6, CHECK_EG_MATCH)
    MAP_OP(7, CHECK_EXC_MATCH)
    MAP_OP(8, CLEANUP_THROW)
    MAP_OP(9, DELETE_SUBSCR)
    MAP_OP(10, END_ASYNC_FOR)
    MAP_OP(11, END_FOR)
    MAP_OP(12, END_SEND)
    MAP_OP(13, EXIT_INIT_CHECK)
    MAP_OP(14, FORMAT_SIMPLE)
    MAP_OP(15, FORMAT_WITH_SPEC)
    MAP_OP(16, GET_AITER)
    MAP_OP(17, RESERVED)
    MAP_OP(18, GET_ANEXT)
    MAP_OP(19, GET_ITER)
    MAP_OP(20, GET_LEN)
    MAP_OP(21, GET_YIELD_FROM_ITER)
    MAP_OP(22, INTERPRETER_EXIT)
    MAP_OP(23, LOAD_ASSERTION_ERROR)
    MAP_OP(24, LOAD_BUILD_CLASS)
    MAP_OP(25, LOAD_LOCALS)
    MAP_OP(26, MAKE_FUNCTION)
    MAP_OP(27, MATCH_KEYS)
    MAP_OP(28, MATCH_MAPPING)
    MAP_OP(29, MATCH_SEQUENCE)
    MAP_OP(30, NOP)
    MAP_OP(31, POP_EXCEPT)
    MAP_OP(32, POP_TOP)
    MAP_OP(33, PUSH_EXC_INFO)
    MAP_OP(34, PUSH_NULL)
    MAP_OP(35, RETURN_GENERATOR)
    MAP_OP(36, RETURN_VALUE)
    MAP_OP(37, SETUP_ANNOTATIONS)
    MAP_OP(38, STORE_SLICE)
    MAP_OP(39, STORE_SUBSCR)
    MAP_OP(40, TO_BOOL)
    MAP_OP(41, UNARY_INVERT)
    MAP_OP(42, UNARY_NEGATIVE)
    MAP_OP(43, UNARY_NOT)
    MAP_OP(44, WITH_EXCEPT_START)
    MAP_OP(45, BINARY_OP_A)
    MAP_OP(46, BUILD_CONST_KEY_MAP_A)
    MAP_OP(47, BUILD_LIST_A)
    MAP_OP(48, BUILD_MAP_A)
    MAP_OP(49, BUILD_SET_A)
    MAP_OP(50, BUILD_SLICE_A)
    MAP_OP(51, BUILD_STRING_A)
    MAP_OP(52, BUILD_TUPLE_A)
    MAP_OP(53, CALL_A)
    MAP_OP(54, CALL_FUNCTION_EX_A)
    MAP_OP(55, CALL_INTRINSIC_1_A)
    MAP_OP(56, CALL_INTRINSIC_2_A)
    MAP_OP(57, CALL_KW_A)
    MAP_OP(58, COMPARE_OP_A)
    MAP_OP(59, CONTAINS_OP_A)
    MAP_OP(60, CONVERT_VALUE_A)
    MAP_OP(61, COPY_A)
    MAP_OP(62, COPY_FREE_VARS_A)
    MAP_OP(63, DELETE_ATTR_A)
    MAP_OP(64, DELETE_DEREF_A)
    MAP_OP(65, DELETE_FAST_A)
    MAP_OP(66, DELETE_GLOBAL_A)
    MAP_OP(67, DELETE_NAME_A)
    MAP_OP(68, DICT_MERGE_A)
    MAP_OP(69, DICT_UPDATE_A)
    MAP_OP(70, ENTER_EXECUTOR_A)
    MAP_OP(71, EXTENDED_ARG_A)
    MAP_OP(72, FOR_ITER_A)
    MAP_OP(73, GET_AWAITABLE_A)
    MAP_OP(74, IMPORT_FROM_A)
    MAP_OP(75, IMPORT_NAME_A)
    MAP_OP(76, IS_OP_A)
    MAP_OP(77, JUMP_BACKWARD_A)
    MAP_OP(78, JUMP_BACKWARD_NO_INTERRUPT_A)
    MAP_OP(79, JUMP_FORWARD_A)
    MAP_OP(80, LIST_APPEND_A)
    MAP_OP(81, LIST_EXTEND_A)
    MAP_OP(82, LOAD_ATTR_A)
    MAP_OP(83, LOAD_CONST_A)
    MAP_OP(84, LOAD_DEREF_A)
    MAP_OP(85, LOAD_FAST_A)
    MAP_OP(86, LOAD_FAST_AND_CLEAR_A)
    MAP_OP(87, LOAD_FAST_CHECK_A)
    MAP_OP(88, LOAD_FAST_LOAD_FAST_A)
    MAP_OP(89, LOAD_FROM_DICT_OR_DEREF_A)
    MAP_OP(90, LOAD_FROM_DICT_OR_GLOBALS_A)
    MAP_OP(91, LOAD_GLOBAL_A)
    MAP_OP(92, LOAD_NAME_A)
    MAP_OP(93, LOAD_SUPER_ATTR_A)
    MAP_OP(94, MAKE_CELL_A)
    MAP_OP(95, MAP_ADD_A)
    MAP_OP(96, MATCH_CLASS_A)
    MAP_OP(97, POP_JUMP_IF_FALSE_A)
    MAP_OP(98, POP_JUMP_IF_NONE_A)
    MAP_OP(99, POP_JUMP_IF_NOT_NONE_A)
    MAP_OP(100, POP_JUMP_IF_TRUE_A)
    MAP_OP(101, RAISE_VARARGS_A)
    MAP_OP(102, RERAISE_A)
    MAP_OP(103, RETURN_CONST_A)
    MAP_OP(104, SEND_A)
    MAP_OP(105, SET_ADD_A)
    MAP_OP(106, SET_FUNCTION_ATTRIBUTE_A)
    MAP_OP(107, SET_UPDATE_A)
    MAP_OP(108, STORE_ATTR_A)
    MAP_OP(109, STORE_DEREF_A)
    MAP_OP(110, STORE_FAST_A)
    MAP_OP(111, STORE_FAST_LOAD_FAST_A)
    MAP_OP(112, STORE_FAST_STORE_FAST_A)
    MAP_OP(113, STORE_GLOBAL_A)
    MAP_OP(114, STORE_NAME_A)
    MAP_OP(115, SWAP_A)
    MAP_OP(116, UNPACK_EX_A)
    MAP_OP(117, UNPACK_SEQUENCE_A)
    MAP_OP(118, YIELD_VALUE_A)
    MAP_OP(149, RESUME_A)
    MAP_OP(150, BINARY_OP_ADD_FLOAT)
    MAP_OP(151, BINARY_OP_ADD_INT)
    MAP_OP(152, BINARY_OP_ADD_UNICODE)
    MAP_OP(153, BINARY_OP_MULTIPLY_FLOAT)
    MAP_OP(154, BINARY_OP_MULTIPLY_INT)
    MAP_OP(155, BINARY_OP_SUBTRACT_FLOAT)
    MAP_OP(156, BINARY_OP_SUBTRACT_INT)
    MAP_OP(157, BINARY_SUBSCR_DICT)
    MAP_OP(158, BINARY_SUBSCR_GETITEM)
    MAP_OP(159, BINARY_SUBSCR_LIST_INT)
    MAP_OP(160, BINARY_SUBSCR_STR_INT)
    MAP_OP(161, BINARY_SUBSCR_TUPLE_INT)
    MAP_OP(162, CALL_ALLOC_AND_ENTER_INIT_A)
    MAP_OP(163, CALL_BOUND_METHOD_EXACT_ARGS_A)
    MAP_OP(164, CALL_BOUND_METHOD_GENERAL_A)
    MAP_OP(165, CALL_BUILTIN_CLASS_A)
    MAP_OP(166, CALL_BUILTIN_FAST_A)
    MAP_OP(167, CALL_BUILTIN_FAST_WITH_KEYWORDS_A)
    MAP_OP(168, CALL_BUILTIN_O_A)
    MAP_OP(169, CALL_ISINSTANCE_A)
    MAP_OP(170, CALL_LEN_A)
    MAP_OP(171, CALL_LIST_APPEND_A)
    MAP_OP(172, CALL_METHOD_DESCRIPTOR_FAST_A)
    MAP_OP(173, CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS_A)
    MAP_OP(174, CALL_METHOD_DESCRIPTOR_NOARGS_A)
    MAP_OP(175, CALL_METHOD_DESCRIPTOR_O_A)
    MAP_OP(176, CALL_NON_PY_GENERAL_A)
    MAP_OP(177, CALL_PY_EXACT_ARGS_A)
    MAP_OP(178, CALL_PY_GENERAL_A)
    MAP_OP(179, CALL_STR_1_A)
    MAP_OP(180, CALL_TUPLE_1_A)
    MAP_OP(181, CALL_TYPE_1_A)
    MAP_OP(182, COMPARE_OP_FLOAT_A)
    MAP_OP(183, COMPARE_OP_INT_A)
    MAP_OP(184, COMPARE_OP_STR_A)
    MAP_OP(185, CONTAINS_OP_DICT_A)
    MAP_OP(186, CONTAINS_OP_SET_A)
    MAP_OP(187, FOR_ITER_GEN_A)
    MAP_OP(188, FOR_ITER_LIST_A)
    MAP_OP(189, FOR_ITER_RANGE_A)
    MAP_OP(190, FOR_ITER_TUPLE_A)
    MAP_OP(191, LOAD_ATTR_CLASS_A)
    MAP_OP(192, LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN_A)
    MAP_OP(193, LOAD_ATTR_INSTANCE_VALUE_A)
    MAP_OP(194, LOAD_ATTR_METHOD_LAZY_DICT_A)
    MAP_OP(195, LOAD_ATTR_METHOD_NO_DICT_A)
    MAP_OP(196, LOAD_ATTR_METHOD_WITH_VALUES_A)
    MAP_OP(197, LOAD_ATTR_MODULE_A)
    MAP_OP(198, LOAD_ATTR_NONDESCRIPTOR_NO_DICT_A)
    MAP_OP(199, LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES_A)
    MAP_OP(200, LOAD_ATTR_PROPERTY_A)
    MAP_OP(201, LOAD_ATTR_SLOT_A)
    MAP_OP(202, LOAD_ATTR_WITH_HINT_A)
    MAP_OP(203, LOAD_GLOBAL_BUILTIN_A)
    MAP_OP(204, LOAD_GLOBAL_MODULE_A)
    MAP_OP(205, LOAD_SUPER_ATTR_ATTR_A)
    MAP_OP(206, LOAD_SUPER_ATTR_METHOD_A)
    MAP_OP(207, RESUME_CHECK_A)
    MAP_OP(208, SEND_GEN_A)
    MAP_OP(209, STORE_ATTR_INSTANCE_VALUE_A)
    MAP_OP(210, STORE_ATTR_SLOT_A)
    MAP_OP(211, STORE_ATTR_WITH_HINT_A)
    MAP_OP(212, STORE_SUBSCR_DICT)
    MAP_OP(213, STORE_SUBSCR_LIST_INT)
    MAP_OP(214, TO_BOOL_ALWAYS_TRUE)
    MAP_OP(215, TO_BOOL_BOOL)
    MAP_OP(216, TO_BOOL_INT)
    MAP_OP(217, TO_BOOL_LIST)
    MAP_OP(218, TO_BOOL_NONE)
    MAP_OP(219, TO_BOOL_STR)
    MAP_OP(220, UNPACK_SEQUENCE_LIST_A)
    MAP_OP(221, UNPACK_SEQUENCE_TUPLE_A)
    MAP_OP(222, UNPACK_SEQUENCE_TWO_TUPLE_A)
    MAP_OP(236, INSTRUMENTED_RESUME_A)
    MAP_OP(237, INSTRUMENTED_END_FOR_A)
    MAP_OP(238, INSTRUMENTED_END_SEND_A)
    MAP_OP(239, INSTRUMENTED_RETURN_VALUE_A)
    MAP_OP(240, INSTRUMENTED_RETURN_CONST_A)
    MAP_OP(241, INSTRUMENTED_YIELD_VALUE_A)
    MAP_OP(242, INSTRUMENTED_LOAD_SUPER_ATTR_A)
    MAP_OP(243, INSTRUMENTED_FOR_ITER_A)
    MAP_OP(244, INSTRUMENTED_CALL_A)
    MAP_OP(245, INSTRUMENTED_CALL_KW_A)
    MAP_OP(246, INSTRUMENTED_CALL_FUNCTION_EX_A)
    MAP_OP(247, INSTRUMENTED_INSTRUCTION_A)
    MAP_OP(248, INSTRUMENTED_JUMP_FORWARD_A)
    MAP_OP(249, INSTRUMENTED_JUMP_BACKWARD_A)
    MAP_OP(250, INSTRUMENTED_POP_JUMP_IF_TRUE_A)
    MAP_OP(251, INSTRUMENTED_POP_JUMP_IF_FALSE_A)
    MAP_OP(252, INSTRUMENTED_POP_JUMP_IF_NONE_A)
    MAP_OP(253, INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A)
    MAP_OP(254, INSTRUMENTED_LINE_A)
END_MAP()
