#include "pyc_numeric.h"
#include "bytecode.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define DECLARE_PYTHON(maj, min) \
    extern int python_##maj##min##_map(int); \
    extern int python_##maj##min##_unmap(int);

DECLARE_PYTHON(1, 0)
DECLARE_PYTHON(1, 1)
DECLARE_PYTHON(1, 3)
DECLARE_PYTHON(1, 4)
DECLARE_PYTHON(1, 5)
DECLARE_PYTHON(1, 6)
DECLARE_PYTHON(2, 0)
DECLARE_PYTHON(2, 1)
DECLARE_PYTHON(2, 2)
DECLARE_PYTHON(2, 3)
DECLARE_PYTHON(2, 4)
DECLARE_PYTHON(2, 5)
DECLARE_PYTHON(2, 6)
DECLARE_PYTHON(2, 7)
DECLARE_PYTHON(3, 0)
DECLARE_PYTHON(3, 1)
DECLARE_PYTHON(3, 2)
DECLARE_PYTHON(3, 3)
DECLARE_PYTHON(3, 4)
DECLARE_PYTHON(3, 5)

const char* Pyc::OpcodeName(int opcode)
{
    static const char* opcode_names[] = {
    "STOP_CODE", "POP_TOP", "ROT_TWO", "ROT_THREE", "DUP_TOP", "DUP_TOP_TWO",
    "UNARY_POSITIVE", "UNARY_NEGATIVE", "UNARY_NOT", "UNARY_CONVERT",
    "UNARY_CALL", "UNARY_INVERT", "BINARY_POWER", "BINARY_MULTIPLY",
    "BINARY_DIVIDE", "BINARY_MODULO", "BINARY_ADD", "BINARY_SUBTRACT",
    "BINARY_SUBSCR", "BINARY_CALL", "SLICE_0", "SLICE_1", "SLICE_2", "SLICE_3",
    "STORE_SLICE_0", "STORE_SLICE_1", "STORE_SLICE_2", "STORE_SLICE_3",
    "DELETE_SLICE_0", "DELETE_SLICE_1", "DELETE_SLICE_2", "DELETE_SLICE_3",
    "STORE_SUBSCR", "DELETE_SUBSCR", "BINARY_LSHIFT", "BINARY_RSHIFT",
    "BINARY_AND", "BINARY_XOR", "BINARY_OR", "PRINT_EXPR", "PRINT_ITEM",
    "PRINT_NEWLINE", "BREAK_LOOP", "RAISE_EXCEPTION", "LOAD_LOCALS",
    "RETURN_VALUE", "LOAD_GLOBALS", "EXEC_STMT", "BUILD_FUNCTION", "POP_BLOCK",
    "END_FINALLY", "BUILD_CLASS", "ROT_FOUR", "NOP", "LIST_APPEND",
    "BINARY_FLOOR_DIVIDE", "BINARY_TRUE_DIVIDE", "INPLACE_FLOOR_DIVIDE",
    "INPLACE_TRUE_DIVIDE", "STORE_MAP", "INPLACE_ADD", "INPLACE_SUBTRACT",
    "INPLACE_MULTIPLY", "INPLACE_DIVIDE", "INPLACE_MODULO", "INPLACE_POWER",
    "GET_ITER", "PRINT_ITEM_TO", "PRINT_NEWLINE_TO", "INPLACE_LSHIFT",
    "INPLACE_RSHIFT", "INPLACE_AND", "INPLACE_XOR", "INPLACE_OR",
    "WITH_CLEANUP", "WITH_CLEANUP_START", "WITH_CLEANUP_FINISH", "IMPORT_STAR",
    "YIELD_VALUE", "LOAD_BUILD_CLASS", "STORE_LOCALS", "POP_EXCEPT", "SET_ADD",
    "YIELD_FROM", "BINARY_MATRIX_MULTIPLY", "INPLACE_MATRIX_MULTIPLY",
    "GET_AITER", "GET_ANEXT", "BEFORE_ASYNC_WITH", "GET_YEILD_FROM_ITER",
    "GET_AWAITABLE",

    "STORE_NAME", "DELETE_NAME", "UNPACK_TUPLE", "UNPACK_LIST", "UNPACK_ARG",
    "STORE_ATTR", "DELETE_ATTR", "STORE_GLOBAL", "DELETE_GLOBAL",
    "UNPACK_VARARG", "LOAD_CONST", "LOAD_NAME", "BUILD_TUPLE", "BUILD_LIST",
    "BUILD_MAP", "LOAD_ATTR", "COMPARE_OP", "IMPORT_NAME", "IMPORT_FROM",
    "JUMP_FORWARD", "JUMP_IF_FALSE", "JUMP_IF_TRUE", "JUMP_ABSOLUTE",
    "FOR_LOOP", "LOAD_LOCAL", "LOAD_GLOBAL", "SET_FUNC_ARGS", "SETUP_LOOP",
    "SETUP_EXCEPT", "SETUP_FINALLY", "RESERVE_FAST", "LOAD_FAST",
    "STORE_FAST", "DELETE_FAST", "SET_LINENO", "RAISE_VARARGS",
    "CALL_FUNCTION", "MAKE_FUNCTION", "BUILD_SLICE", "CALL_FUNCTION_VAR",
    "CALL_FUNCTION_KW", "CALL_FUNCTION_VAR_KW", "UNPACK_SEQUENCE", "FOR_ITER",
    "DUP_TOPX", "BUILD_SET", "JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP",
    "POP_JUMP_IF_FALSE", "POP_JUMP_IF_TRUE", "CONTINUE_LOOP", "MAKE_CLOSURE",
    "LOAD_CLOSURE", "LOAD_DEREF", "STORE_DEREF", "DELETE_DEREF",
    "EXTENDED_ARG", "SETUP_WITH", "SET_ADD", "MAP_ADD", "UNPACK_EX",
    "LIST_APPEND", "LOAD_CLASSDEREF", "BUILD_LIST_UNPACK", "BUILD_MAP_UNPACK",
    "BUILD_MAP_UNPACK_WITH_CALL", "BUILD_TUPLE_UNPACK", "BUILD_SET_UNPACK",
    "SETUP_ASYNC_WITH",
    };

#if __cplusplus >= 201103L
    static_assert(sizeof(opcode_names) / sizeof(opcode_names[0]) == PYC_LAST_OPCODE,
                  "Pyc::OpcodeName opcode_names not in sync with opcode enum");
#endif

    if (opcode < 0)
        return "<INVALID>";

    else if (opcode < PYC_LAST_OPCODE)
        return opcode_names[opcode];

	else {
		static char badcode[10];
		_snprintf_s(badcode, 10, "<%d>", opcode);
		return badcode;
	}
};

int Pyc::ByteToOpcode(int maj, int min, int opcode)
{
    switch (maj) {
    case 1:
        switch (min) {
        case 0: return python_10_map(opcode);
        case 1: return python_11_map(opcode);
        case 3: return python_13_map(opcode);
        case 4: return python_14_map(opcode);
        case 5: return python_15_map(opcode);
        case 6: return python_16_map(opcode);
        }
        break;
    case 2:
        switch (min) {
        case 0: return python_20_map(opcode);
        case 1: return python_21_map(opcode);
        case 2: return python_22_map(opcode);
        case 3: return python_23_map(opcode);
        case 4: return python_24_map(opcode);
        case 5: return python_25_map(opcode);
        case 6: return python_26_map(opcode);
        case 7: return python_27_map(opcode);
        }
        break;
    case 3:
        switch (min) {
        case 0: return python_30_map(opcode);
        case 1: return python_31_map(opcode);
        case 2: return python_32_map(opcode);
        case 3: return python_33_map(opcode);
        case 4: return python_34_map(opcode);
        case 5: return python_35_map(opcode);
        }
        break;
    }
    return PYC_INVALID_OPCODE;
}

bool Pyc::IsConstArg(int opcode)
{
    return (opcode == Pyc::LOAD_CONST_A) || (opcode == Pyc::RESERVE_FAST_A);
}

bool Pyc::IsNameArg(int opcode)
{
#define IsInList(Value,List)

	switch(opcode) {
		case Pyc::DELETE_ATTR_A:		case Pyc::DELETE_GLOBAL_A:		case Pyc::DELETE_NAME_A:
		case Pyc::IMPORT_FROM_A:		case Pyc::IMPORT_NAME_A:
		case Pyc::LOAD_ATTR_A:			case Pyc::LOAD_GLOBAL_A:
		case Pyc::LOAD_LOCAL_A:			case Pyc::LOAD_NAME_A:
		case Pyc::STORE_ATTR_A:			case Pyc::STORE_GLOBAL_A:		case Pyc::STORE_NAME_A:
			return true;
		default: 
			return false;
	}
	// as switch case is faster than Or-chained compares 
	// since a switch case is done via table lookup

	/*
    return (opcode == Pyc::DELETE_ATTR_A) || (opcode == Pyc::DELETE_GLOBAL_A) ||
           (opcode == Pyc::DELETE_NAME_A) || (opcode == Pyc::IMPORT_FROM_A) ||
           (opcode == Pyc::IMPORT_NAME_A) || (opcode == Pyc::LOAD_ATTR_A) ||
           (opcode == Pyc::LOAD_GLOBAL_A) || (opcode == Pyc::LOAD_LOCAL_A) ||
           (opcode == Pyc::LOAD_NAME_A	) || (opcode == Pyc::STORE_ATTR_A) ||
           (opcode == Pyc::STORE_GLOBAL_A)|| (opcode == Pyc::STORE_NAME_A);
		   */
}

bool Pyc::IsVarNameArg(int opcode)
{
    return (opcode == Pyc::DELETE_FAST_A) || (opcode == Pyc::LOAD_FAST_A) ||
           (opcode == Pyc::STORE_FAST_A);
}

bool Pyc::IsCellArg(int opcode)
{
    return (opcode == Pyc::LOAD_CLOSURE_A) || (opcode == Pyc::LOAD_DEREF_A) ||
           (opcode == Pyc::STORE_DEREF_A);
}

bool Pyc::IsJumpOffsetArg(int opcode)
{
	switch (opcode) {
		case Pyc::JUMP_FORWARD_A:		case Pyc::JUMP_IF_FALSE_A:
		case Pyc::JUMP_IF_TRUE_A:
		case Pyc::SETUP_LOOP_A:			case Pyc::SETUP_FINALLY_A:
		case Pyc::SETUP_EXCEPT_A:		case Pyc::FOR_LOOP_A:
			return true;
		default:
			return false;
}/*
    return (opcode == Pyc::JUMP_FORWARD_A) || (opcode == Pyc::JUMP_IF_FALSE_A) ||
           (opcode == Pyc::JUMP_IF_TRUE_A) || (opcode == Pyc::SETUP_LOOP_A) ||
           (opcode == Pyc::SETUP_FINALLY_A) || (opcode == Pyc::SETUP_EXCEPT_A) ||
           (opcode == Pyc::FOR_LOOP_A);*/
}


void print_const(PycRef<PycObject> obj, PycModule* mod)
{
    switch (obj->type()) {
    case PycObject::TYPE_STRING:
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0);
        break;
    case PycObject::TYPE_UNICODE:
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u');
        break;
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        OutputString(obj.cast<PycString>(), 0);
        break;
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            log( "(");
            PycTuple::value_t values = obj.cast<PycTuple>()->values();
            PycTuple::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    log( ", ");
                    print_const(*it, mod);
                }
            }
            if (values.size() == 1)
                log( ",)");
            else
                log( ")");
        }
        break;
    case PycObject::TYPE_LIST:
        {
            log( "[");
            PycList::value_t values = obj.cast<PycList>()->values();
            PycList::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    log( ", ");
                    print_const(*it, mod);
                }
            }
            log( "]");
        }
        break;
    case PycObject::TYPE_DICT:
        {
            log( "{");
            PycDict::key_t keys = obj.cast<PycDict>()->keys();
            PycDict::value_t values = obj.cast<PycDict>()->values();
            PycDict::key_t::const_iterator ki = keys.begin();
            PycDict::value_t::const_iterator vi = values.begin();
            if (ki != keys.end()) {
                print_const(*ki, mod);
                log( ": ");
                print_const(*vi, mod);
                while (++ki != keys.end()) {
                    ++vi;
                    log( ", ");
                    print_const(*ki, mod);
                    log( ": ");
                    print_const(*vi, mod);
                }
            }
            log( "}");
        }
        break;
    case PycObject::TYPE_SET:
        {
            log( "{");
            PycSet::value_t values = obj.cast<PycSet>()->values();
            PycSet::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    log( ", ");
                    print_const(*it, mod);
                }
            }
            log( "}");
        }
        break;
    case PycObject::TYPE_NONE:
        log( "None");
        break;
    case PycObject::TYPE_TRUE:
        log( "True");
        break;
    case PycObject::TYPE_FALSE:
        log( "False");
        break;
    case PycObject::TYPE_INT:
        log( "%d", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_LONG:
        log( "%s", obj.cast<PycLong>()->repr().c_str());
        break;
    case PycObject::TYPE_FLOAT:
        log( "%s", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        log( "(%s+%sj)", obj.cast<PycComplex>()->value(),
                                        obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        log( "%g", obj.cast<PycCFloat>()->value());
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        log( "(%g+%gj)", obj.cast<PycCComplex>()->value(),
                                        obj.cast<PycCComplex>()->imag());
        break;
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        log( "<CODE> %s", obj.cast<PycCode>()->name()->value());
        break;
    }
}

void bc_next(PycBuffer& source, PycModule* mod, int& opcode, int& operand, int& pos)
{
    opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
    operand = 0;
    pos += 1;

    if (opcode == Pyc::EXTENDED_ARG_A) {
        operand = source.get16() << 16;
        opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
        pos += 3;
    }
    if (opcode >= Pyc::PYC_HAVE_ARG) {
        operand |= source.get16();
        pos += 2;
    }
}

void bc_disasm(PycRef<PycCode> code, PycModule* mod, int indent)
{
    PycBuffer source(code->code()->value(), code->code()->length());

    int opcode, operand;
    int pos = 0;
    while (!source.atEof()) {
		
        for (int i=0; i<indent; i++)
            log( "    ");
        log( "%-7d ", pos);   // Current bytecode position

        bc_next(source, mod, opcode, operand, pos);
        log( "%-24s", Pyc::OpcodeName(opcode));

        if (opcode >= Pyc::PYC_HAVE_ARG) {
            if (Pyc::IsConstArg(opcode)) {
                log( "%d: ", operand);
                print_const(code->getConst(operand), mod);

            } else if (Pyc::IsNameArg(opcode)) {
                log( "%d: %s", operand, code->getName(operand)->value());

            } else if (Pyc::IsVarNameArg(opcode)) {
                log( "%d: %s", operand, code->getVarName(operand)->value());

            } else if (Pyc::IsCellArg(opcode)) {
                log( "%d: ", operand);
                print_const(code->getCellVar(operand), mod);

            } else if (Pyc::IsJumpOffsetArg(opcode)) {
                log( "%d (to %d)", operand, pos+operand);

            } else {
                log( "%d", operand);

            }
        }
        log( "\n");
    }
}
