/* No parameter word */
OPCODE(STOP_CODE)                       // Python 1.0 - 3.2
OPCODE(POP_TOP)                         // Python 1.0 ->
OPCODE(ROT_TWO)                         // Python 1.0 - 3.10
OPCODE(ROT_THREE)                       // Python 1.0 - 3.10
OPCODE(DUP_TOP)                         // Python 1.0 - 3.10
OPCODE(DUP_TOP_TWO)                     // Python 3.2 - 3.10
OPCODE(UNARY_POSITIVE)                  // Python 1.0 - 3.11
OPCODE(UNARY_NEGATIVE)                  // Python 1.0 ->
OPCODE(UNARY_NOT)                       // Python 1.0 ->
OPCODE(UNARY_CONVERT)                   // Python 1.0 - 2.7
OPCODE(UNARY_CALL)                      // Python 1.0 - 1.2
OPCODE(UNARY_INVERT)                    // Python 1.0 ->
OPCODE(BINARY_POWER)                    // Python 1.4 - 3.10
OPCODE(BINARY_MULTIPLY)                 // Python 1.0 - 3.10
OPCODE(BINARY_DIVIDE)                   // Python 1.0 - 2.7
OPCODE(BINARY_MODULO)                   // Python 1.0 - 3.10
OPCODE(BINARY_ADD)                      // Python 1.0 - 3.10
OPCODE(BINARY_SUBTRACT)                 // Python 1.0 - 3.10
OPCODE(BINARY_SUBSCR)                   // Python 1.0 ->
OPCODE(BINARY_CALL)                     // Python 1.0 - 1.2
OPCODE(SLICE_0)                         // Python 1.0 - 2.7
OPCODE(SLICE_1)                         // Python 1.0 - 2.7
OPCODE(SLICE_2)                         // Python 1.0 - 2.7
OPCODE(SLICE_3)                         // Python 1.0 - 2.7
OPCODE(STORE_SLICE_0)                   // Python 1.0 - 2.7
OPCODE(STORE_SLICE_1)                   // Python 1.0 - 2.7
OPCODE(STORE_SLICE_2)                   // Python 1.0 - 2.7
OPCODE(STORE_SLICE_3)                   // Python 1.0 - 2.7
OPCODE(DELETE_SLICE_0)                  // Python 1.0 - 2.7
OPCODE(DELETE_SLICE_1)                  // Python 1.0 - 2.7
OPCODE(DELETE_SLICE_2)                  // Python 1.0 - 2.7
OPCODE(DELETE_SLICE_3)                  // Python 1.0 - 2.7
OPCODE(STORE_SUBSCR)                    // Python 1.0 ->
OPCODE(DELETE_SUBSCR)                   // Python 1.0 ->
OPCODE(BINARY_LSHIFT)                   // Python 1.0 - 3.10
OPCODE(BINARY_RSHIFT)                   // Python 1.0 - 3.10
OPCODE(BINARY_AND)                      // Python 1.0 - 3.10
OPCODE(BINARY_XOR)                      // Python 1.0 - 3.10
OPCODE(BINARY_OR)                       // Python 1.0 - 3.10
OPCODE(PRINT_EXPR)                      // Python 1.0 - 3.11
OPCODE(PRINT_ITEM)                      // Python 1.0 - 2.7
OPCODE(PRINT_NEWLINE)                   // Python 1.0 - 2.7
OPCODE(BREAK_LOOP)                      // Python 1.0 - 3.7
OPCODE(RAISE_EXCEPTION)                 // Python 1.0 - 1.2
OPCODE(LOAD_LOCALS)                     // Python 1.0 - 2.7, 3.12 ->
OPCODE(RETURN_VALUE)                    // Python 1.0 ->
OPCODE(LOAD_GLOBALS)                    // Python 1.0 - 1.2
OPCODE(EXEC_STMT)                       // Python 1.0 - 2.7
OPCODE(BUILD_FUNCTION)                  // Python 1.0 - 1.2
OPCODE(POP_BLOCK)                       // Python 1.0 - 3.10
OPCODE(END_FINALLY)                     // Python 1.0 - 3.8
OPCODE(BUILD_CLASS)                     // Python 1.0 - 2.7
OPCODE(ROT_FOUR)                        // Python 2.0 - 3.1, 3.8 - 3.10
OPCODE(NOP)                             // Python 2.4 ->
OPCODE(LIST_APPEND)                     // Python 2.4 - 2.6, 3.0
OPCODE(BINARY_FLOOR_DIVIDE)             // Python 2.2 - 3.10
OPCODE(BINARY_TRUE_DIVIDE)              // Python 2.2 - 3.10
OPCODE(INPLACE_FLOOR_DIVIDE)            // Python 2.2 - 3.10
OPCODE(INPLACE_TRUE_DIVIDE)             // Python 2.2 - 3.10
OPCODE(GET_LEN)                         // Python 3.10 ->
OPCODE(MATCH_MAPPING)                   // Python 3.10 ->
OPCODE(MATCH_SEQUENCE)                  // Python 3.10 ->
OPCODE(MATCH_KEYS)                      // Python 3.10 ->
OPCODE(COPY_DICT_WITHOUT_KEYS)          // Python 3.10
OPCODE(STORE_MAP)                       // Python 2.6 - 3.4
OPCODE(INPLACE_ADD)                     // Python 2.0 - 3.10
OPCODE(INPLACE_SUBTRACT)                // Python 2.0 - 3.10
OPCODE(INPLACE_MULTIPLY)                // Python 2.0 - 3.10
OPCODE(INPLACE_DIVIDE)                  // Python 2.0 - 2.7
OPCODE(INPLACE_MODULO)                  // Python 2.0 - 3.10
OPCODE(INPLACE_POWER)                   // Python 2.0 - 3.10
OPCODE(GET_ITER)                        // Python 2.2 ->
OPCODE(PRINT_ITEM_TO)                   // Python 2.0 - 2.7
OPCODE(PRINT_NEWLINE_TO)                // Python 2.0 - 2.7
OPCODE(INPLACE_LSHIFT)                  // Python 2.0 - 3.10
OPCODE(INPLACE_RSHIFT)                  // Python 2.0 - 3.10
OPCODE(INPLACE_AND)                     // Python 2.0 - 3.10
OPCODE(INPLACE_XOR)                     // Python 2.0 - 3.10
OPCODE(INPLACE_OR)                      // Python 2.0 - 3.10
OPCODE(WITH_CLEANUP)                    // Python 2.5 - 3.4
OPCODE(WITH_CLEANUP_START)              // Python 3.5 - 3.8
OPCODE(WITH_CLEANUP_FINISH)             // Python 3.5 - 3.8
OPCODE(IMPORT_STAR)                     // Python 2.0 - 3.11
OPCODE(SETUP_ANNOTATIONS)               // Python 3.6 ->
OPCODE(YIELD_VALUE)                     // Python 2.2 - 3.11
OPCODE(LOAD_BUILD_CLASS)                // Python 3.0 ->
OPCODE(STORE_LOCALS)                    // Python 3.0 - 3.3
OPCODE(POP_EXCEPT)                      // Python 3.0 ->
OPCODE(SET_ADD)                         // Python 3.0
OPCODE(YIELD_FROM)                      // Python 3.3 - 3.10
OPCODE(BINARY_MATRIX_MULTIPLY)          // Python 3.5 - 3.10
OPCODE(INPLACE_MATRIX_MULTIPLY)         // Python 3.5 - 3.10
OPCODE(GET_AITER)                       // Python 3.5 ->
OPCODE(GET_ANEXT)                       // Python 3.5 ->
OPCODE(BEFORE_ASYNC_WITH)               // Python 3.5 ->
OPCODE(GET_YIELD_FROM_ITER)             // Python 3.5 ->
OPCODE(GET_AWAITABLE)                   // Python 3.5 - 3.10
OPCODE(BEGIN_FINALLY)                   // Python 3.8
OPCODE(END_ASYNC_FOR)                   // Python 3.8 ->
OPCODE(RERAISE)                         // Python 3.9
OPCODE(WITH_EXCEPT_START)               // Python 3.9 ->
OPCODE(LOAD_ASSERTION_ERROR)            // Python 3.9 ->
OPCODE(LIST_TO_TUPLE)                   // Python 3.9 - 3.11
OPCODE(CACHE)                           // Python 3.11 ->
OPCODE(PUSH_NULL)                       // Python 3.11 ->
OPCODE(PUSH_EXC_INFO)                   // Python 3.11 ->
OPCODE(CHECK_EXC_MATCH)                 // Python 3.11 ->
OPCODE(CHECK_EG_MATCH)                  // Python 3.11 ->
OPCODE(BEFORE_WITH)                     // Python 3.11 ->
OPCODE(RETURN_GENERATOR)                // Python 3.11 ->
OPCODE(ASYNC_GEN_WRAP)                  // Python 3.11
OPCODE(PREP_RERAISE_STAR)               // Python 3.11
OPCODE(INTERPRETER_EXIT)                // Python 3.12 ->
OPCODE(END_FOR)                         // Python 3.12 ->
OPCODE(END_SEND)                        // Python 3.12 ->
OPCODE(RESERVED)                        // Python 3.12 ->
OPCODE(BINARY_SLICE)                    // Python 3.12 ->
OPCODE(STORE_SLICE)                     // Python 3.12 ->
OPCODE(CLEANUP_THROW)                   // Python 3.12 ->

/* Has parameter word */
OPCODE_A_FIRST(STORE_NAME)              // Python 1.0 ->                names[A]
OPCODE_A(DELETE_NAME)                   // Python 1.0 ->                names[A]
OPCODE_A(UNPACK_TUPLE)                  // Python 1.0 - 1.6             A=count
OPCODE_A(UNPACK_LIST)                   // Python 1.0 - 1.6             A=count
OPCODE_A(UNPACK_ARG)                    // Python 1.0 - 1.4             A=count
OPCODE_A(STORE_ATTR)                    // Python 1.0 ->                names[A]
OPCODE_A(DELETE_ATTR)                   // Python 1.0 ->                names[A]
OPCODE_A(STORE_GLOBAL)                  // Python 1.0 ->                names[A]
OPCODE_A(DELETE_GLOBAL)                 // Python 1.0 ->                names[A]
OPCODE_A(ROT_N)                         // Python 3.10                  A=count
OPCODE_A(UNPACK_VARARG)                 // Python 1.0 - 1.4             A=count
OPCODE_A(LOAD_CONST)                    // Python 1.0 ->                consts[A]
OPCODE_A(LOAD_NAME)                     // Python 1.0 ->                names[A]
OPCODE_A(BUILD_TUPLE)                   // Python 1.0 ->                A=size
OPCODE_A(BUILD_LIST)                    // Python 1.0 ->                A=size
OPCODE_A(BUILD_MAP)                     // Python 1.0 ->                A=size
OPCODE_A(LOAD_ATTR)                     // Python 1.0 ->                names[A]
OPCODE_A(COMPARE_OP)                    // Python 1.0 ->                cmp_ops[A]
OPCODE_A(IMPORT_NAME)                   // Python 1.0 ->                names[A]
OPCODE_A(IMPORT_FROM)                   // Python 1.0 ->                names[A]
OPCODE_A(ACCESS_MODE)                   // Python 1.0 - 1.4             names[A]
OPCODE_A(JUMP_FORWARD)                  // Python 1.0 ->                rel jmp +A
OPCODE_A(JUMP_IF_FALSE)                 // Python 1.0 - 2.6, 3.0        rel jmp +A
OPCODE_A(JUMP_IF_TRUE)                  // Python 1.0 - 2.6, 3.0        rel jmp +A
OPCODE_A(JUMP_ABSOLUTE)                 // Python 1.0 - 3.10            abs jmp A
OPCODE_A(FOR_LOOP)                      // Python 1.0 - 2.2             rel jmp +A
OPCODE_A(LOAD_LOCAL)                    // Python 1.0 - 1.4             names[A]
OPCODE_A(LOAD_GLOBAL)                   // Python 1.0 ->                names[A]
OPCODE_A(SET_FUNC_ARGS)                 // Python 1.1 - 1.4             A=count
OPCODE_A(SETUP_LOOP)                    // Python 1.0 - 3.7             rel jmp +A
OPCODE_A(SETUP_EXCEPT)                  // Python 1.0 - 3.7             rel jmp +A
OPCODE_A(SETUP_FINALLY)                 // Python 1.0 - 3.10            rel jmp +A
OPCODE_A(RESERVE_FAST)                  // Python 1.0 - 1.2             A=count
OPCODE_A(LOAD_FAST)                     // Python 1.0 ->                locals[A]
OPCODE_A(STORE_FAST)                    // Python 1.0 ->                locals[A]
OPCODE_A(DELETE_FAST)                   // Python 1.0 ->                locals[A]
OPCODE_A(GEN_START)                     // Python 3.10                  ???
OPCODE_A(SET_LINENO)                    // Python 1.0 - 2.2             A=line
OPCODE_A(STORE_ANNOTATION)              // Python 3.6                   names[A]
OPCODE_A(RAISE_VARARGS)                 // Python 1.3 ->                A=count
OPCODE_A(CALL_FUNCTION)                 // Python 1.3 - 3.5             A=(#args)+(#kwargs<<8)
                                        // Python 3.6 - 3.10            A=#args
OPCODE_A(MAKE_FUNCTION)                 // Python 1.3 - 2.7             A=#defaults
                                        // Python 3.0 - 3.5             A=(#defaults)+(#kwdefaults<<8)+(#annotations<<16)
                                        // Python 3.6 ->                A=flags
OPCODE_A(BUILD_SLICE)                   // Python 1.4 ->                A=count
OPCODE_A(CALL_FUNCTION_VAR)             // Python 1.6 - 3.5             A=(#args)+(#kwargs<<8)
OPCODE_A(CALL_FUNCTION_KW)              // Python 1.6 - 3.5             A=(#args)+(#kwargs<<8)
                                        // Python 3.6 - 3.10            A=#args
OPCODE_A(CALL_FUNCTION_VAR_KW)          // Python 1.6 - 3.5             A=(#args)+(#kwargs<<8)
OPCODE_A(CALL_FUNCTION_EX)              // Python 3.6 ->                A=flags
OPCODE_A(UNPACK_SEQUENCE)               // Python 2.0 ->                A=count
OPCODE_A(FOR_ITER)                      // Python 2.0 ->                rel jmp +A
OPCODE_A(DUP_TOPX)                      // Python 2.0 - 3.1             A=count
OPCODE_A(BUILD_SET)                     // Python 2.7 ->                A=size
OPCODE_A(JUMP_IF_FALSE_OR_POP)          // Python 2.7, 3.1 - 3.11       abs jmp A
OPCODE_A(JUMP_IF_TRUE_OR_POP)           // Python 2.7, 3.1 - 3.11       abs jmp A
OPCODE_A(POP_JUMP_IF_FALSE)             // Python 2.7, 3.1 - 3.10       abs jmp A
                                        // Python 3.12 ->               rel jmp +A
OPCODE_A(POP_JUMP_IF_TRUE)              // Python 2.7, 3.1 - 3.10       abs jmp A
                                        // Python 3.12 ->               rel jmp +A
OPCODE_A(CONTINUE_LOOP)                 // Python 2.1 - 3.7             abs jmp A
OPCODE_A(MAKE_CLOSURE)                  // Python 2.1 - 2.7             A=#defaults
                                        // Python 3.0 - 3.5             A=(#defaults)+(#kwdefaults<<8)+(#annotations<<16)
OPCODE_A(LOAD_CLOSURE)                  // Python 2.1 ->                freevars[A]
OPCODE_A(LOAD_DEREF)                    // Python 2.1 ->                freevars[A]
OPCODE_A(STORE_DEREF)                   // Python 2.1 ->                freevars[A]
OPCODE_A(DELETE_DEREF)                  // Python 3.2 ->                freevars[A]
OPCODE_A(EXTENDED_ARG)                  // Python 2.0 ->                A=extended_arg
OPCODE_A(SETUP_WITH)                    // Python 2.7, 3.2 - 3.10       rel jmp +A
OPCODE_A(SET_ADD)                       // Python 2.7, 3.1 ->           stack[A]
OPCODE_A(MAP_ADD)                       // Python 2.7, 3.1 ->           stack[A]
OPCODE_A(UNPACK_EX)                     // Python 3.0 ->                A=(before)+(after<<8)
OPCODE_A(LIST_APPEND)                   // Python 2.7, 3.1 ->           stack[A]
OPCODE_A(LOAD_CLASSDEREF)               // Python 3.4 - 3.10            (cellvars+freevars)[A]
                                        // Python 3.11                  localsplusnames[A]
OPCODE_A(MATCH_CLASS)                   // Python 3.10 ->               A=#args
OPCODE_A(BUILD_LIST_UNPACK)             // Python 3.5 - 3.8             A=count
OPCODE_A(BUILD_MAP_UNPACK)              // Python 3.5 - 3.8             A=count
OPCODE_A(BUILD_MAP_UNPACK_WITH_CALL)    // Python 3.5                   A=(count)+(fnloc<<8)
                                        // Python 3.6 - 3.8             A=count
OPCODE_A(BUILD_TUPLE_UNPACK)            // Python 3.5 - 3.8             A=count
OPCODE_A(BUILD_SET_UNPACK)              // Python 3.5 - 3.8             A=count
OPCODE_A(SETUP_ASYNC_WITH)              // Python 3.5 - 3.10            rel jmp +A
OPCODE_A(FORMAT_VALUE)                  // Python 3.6 ->                A=conversion_type
OPCODE_A(BUILD_CONST_KEY_MAP)           // Python 3.6 ->                A=count
OPCODE_A(BUILD_STRING)                  // Python 3.6 ->                A=count
OPCODE_A(BUILD_TUPLE_UNPACK_WITH_CALL)  // Python 3.6 - 3.8             A=count
OPCODE_A(LOAD_METHOD)                   // Python 3.7 - 3.11            names[A]
OPCODE_A(CALL_METHOD)                   // Python 3.7 - 3.10            A=#args
OPCODE_A(CALL_FINALLY)                  // Python 3.8                   rel jmp +A
OPCODE_A(POP_FINALLY)                   // Python 3.8                   A=flags
OPCODE_A(IS_OP)                         // Python 3.9 ->                A=inverted
OPCODE_A(CONTAINS_OP)                   // Python 3.9 ->                A=inverted
OPCODE_A(RERAISE)                       // Python 3.10 ->               A=flag
OPCODE_A(JUMP_IF_NOT_EXC_MATCH)         // Python 3.9 - 3.10            abs jmp A
OPCODE_A(LIST_EXTEND)                   // Python 3.9 ->                stack[A]
OPCODE_A(SET_UPDATE)                    // Python 3.9 ->                stack[A]
OPCODE_A(DICT_MERGE)                    // Python 3.9 ->                stack[A]
OPCODE_A(DICT_UPDATE)                   // Python 3.9 ->                stack[A]
OPCODE_A(SWAP)                          // Python 3.11 ->               stack[A]
OPCODE_A(POP_JUMP_FORWARD_IF_FALSE)     // Python 3.11                  rel jmp +A
OPCODE_A(POP_JUMP_FORWARD_IF_TRUE)      // Python 3.11                  rel jmp +A
OPCODE_A(COPY)                          // Python 3.11 ->               stack[A]
OPCODE_A(BINARY_OP)                     // Python 3.11 ->               bin_ops[A]
OPCODE_A(SEND)                          // Python 3.11 ->               rel jmp +A
OPCODE_A(POP_JUMP_FORWARD_IF_NOT_NONE)  // Python 3.11                  rel jmp +A
OPCODE_A(POP_JUMP_FORWARD_IF_NONE)      // Python 3.11                  rel jmp +A
OPCODE_A(GET_AWAITABLE)                 // Python 3.11 ->               A=awaitable_type
OPCODE_A(JUMP_BACKWARD_NO_INTERRUPT)    // Python 3.11 ->               rel jmp -A
OPCODE_A(MAKE_CELL)                     // Python 3.11 ->               locals[A]
OPCODE_A(JUMP_BACKWARD)                 // Python 3.11 ->               rel jmp -A
OPCODE_A(COPY_FREE_VARS)                // Python 3.11 ->               A=count
OPCODE_A(RESUME)                        // Python 3.11 ->               ???
OPCODE_A(PRECALL)                       // Python 3.11                  A=#args
OPCODE_A(CALL)                          // Python 3.11 ->               A=#args
OPCODE_A(KW_NAMES)                      // Python 3.11 ->               consts[A]
OPCODE_A(POP_JUMP_BACKWARD_IF_NOT_NONE) // Python 3.11                  jmp rel -A
OPCODE_A(POP_JUMP_BACKWARD_IF_NONE)     // Python 3.11                  jmp rel -A
OPCODE_A(POP_JUMP_BACKWARD_IF_FALSE)    // Python 3.11                  jmp rel -A
OPCODE_A(POP_JUMP_BACKWARD_IF_TRUE)     // Python 3.11                  jmp rel -A
OPCODE_A(RETURN_CONST)                  // Python 3.12 ->               consts[A]
OPCODE_A(LOAD_FAST_CHECK)               // Python 3.12 ->               locals[A]
OPCODE_A(POP_JUMP_IF_NOT_NONE)          // Python 3.12 ->               rel jmp +A
OPCODE_A(POP_JUMP_IF_NONE)              // Python 3.12 ->               rel jmp +A
OPCODE_A(LOAD_SUPER_ATTR)               // Python 3.12 ->               A=(flags&0x3)+names[A<<2]
OPCODE_A(LOAD_FAST_AND_CLEAR)           // Python 3.12 ->               locals[A]
OPCODE_A(YIELD_VALUE)                   // Python 3.12 ->               ???
OPCODE_A(CALL_INTRINSIC_1)              // Python 3.12 ->               intrinsics_1[A]
OPCODE_A(CALL_INTRINSIC_2)              // Python 3.12 ->               intrinsics_2[A]
OPCODE_A(LOAD_FROM_DICT_OR_GLOBALS)     // Python 3.12 ->               names[A]
OPCODE_A(LOAD_FROM_DICT_OR_DEREF)       // Python 3.12 ->               localsplusnames[A]

/* Instrumented opcodes */
OPCODE_A(INSTRUMENTED_LOAD_SUPER_ATTR)      // Python 3.12 ->           (see LOAD_SUPER_ATTR)
OPCODE_A(INSTRUMENTED_POP_JUMP_IF_NONE)     // Python 3.12 ->           (see POP_JUMP_IF_NONE)
OPCODE_A(INSTRUMENTED_POP_JUMP_IF_NOT_NONE) // Python 3.12 ->           (see POP_JUMP_IF_NOT_NONE)
OPCODE_A(INSTRUMENTED_RESUME)               // Python 3.12 ->           (see RESUME)
OPCODE_A(INSTRUMENTED_CALL)                 // Python 3.12 ->           (see CALL)
OPCODE_A(INSTRUMENTED_RETURN_VALUE)         // Python 3.12 ->           (see RETURN_VALUE)
OPCODE_A(INSTRUMENTED_YIELD_VALUE)          // Python 3.12 ->           (see YIELD_VALUE)
OPCODE_A(INSTRUMENTED_CALL_FUNCTION_EX)     // Python 3.12 ->           (see CALL_FUNCTION_EX)
OPCODE_A(INSTRUMENTED_JUMP_FORWARD)         // Python 3.12 ->           (see JUMP_FORWARD)
OPCODE_A(INSTRUMENTED_JUMP_BACKWARD)        // Python 3.12 ->           (see JUMP_BACKWARD)
OPCODE_A(INSTRUMENTED_RETURN_CONST)         // Python 3.12 ->           (see RETURN_CONST)
OPCODE_A(INSTRUMENTED_FOR_ITER)             // Python 3.12 ->           (see FOR_ITER)
OPCODE_A(INSTRUMENTED_POP_JUMP_IF_FALSE)    // Python 3.12 ->           (see POP_JUMP_IF_FALSE)
OPCODE_A(INSTRUMENTED_POP_JUMP_IF_TRUE)     // Python 3.12 ->           (see POP_JUMP_IF_TRUE)
OPCODE_A(INSTRUMENTED_END_FOR)              // Python 3.12 ->           (see END_FOR)
OPCODE_A(INSTRUMENTED_END_SEND)             // Python 3.12 ->           (see END_SEND)
OPCODE_A(INSTRUMENTED_INSTRUCTION)          // Python 3.12 ->           ???
OPCODE_A(INSTRUMENTED_LINE)                 // Python 3.12 ->           ???
