#include "bytecode.h"
#include "numeric.h"

const char* Py1k::OpcodeNames[256] = {
    "STOP_CODE", "POP_TOP", "ROT_TWO", "ROT_THREE", "DUP_TOP",
    "<5>", "<6>", "<7>", "<8>", "<9>",
    "UNARY_POSITIVE", "UNARY_NEGATIVE", "UNARY_NOT", "UNARY_CONVERT",
    "UNARY_CALL", "UNARY_INVERT", "<16>", "<17>", "<18>",
    "BINARY_POWER", "BINARY_MULTIPLY", "BINARY_DIVIDE", "BINARY_MODULO",
    "BINARY_ADD", "BINARY_SUBTRACT", "BINARY_SUBSCR", "BINARY_CALL",
    "<27>", "<28>", "<29>",
    "SLICE+0", "SLICE+1", "SLICE+2", "SLICE+3",
    "<34>", "<35>", "<36>", "<37>", "<38>", "<39>",
    "STORE_SLICE+0", "STORE_SLICE+1", "STORE_SLICE+2", "STORE_SLICE+3",
    "<44>", "<45>", "<46>", "<47>", "<48>", "<49>",
    "DELETE_SLICE+0", "DELETE_SLICE+1", "DELETE_SLICE+2", "DELETE_SLICE+3",
    "<54>", "<55>", "<56>", "<57>", "<58>", "<59>",
    "STORE_SUBSCR", "DELETE_SUBSCR", "BINARY_LSHIFT", "BINARY_RSHIFT",
    "BINARY_AND", "BINARY_XOR", "BINARY_OR", "<67>", "<68>", "<69>",
    "PRINT_EXPR", "PRINT_ITEM", "PRINT_NEWLINE",
    "<73>", "<74>", "<75>", "<76>", "<77>", "<78>", "<79>",
    "BREAK_LOOP", "RAISE_EXCEPTION", "LOAD_LOCALS", "RETURN_VALUE",
    "LOAD_GLOBALS", "EXEC_STMT", "BUILD_FUNCTION", "POP_BLOCK", "END_FINALLY",
    "BUILD_CLASS",
    "STORE_NAME", "DELETE_NAME", "UNPACK_TUPLE", "UNPACK_LIST", "UNPACK_ARG",
    "STORE_ATTR", "DELETE_ATTR", "STORE_GLOBAL", "DELETE_GLOBAL", "UNPACK_VARARG",
    "LOAD_CONST", "LOAD_NAME", "BUILD_TUPLE", "BUILD_LIST", "BUILD_MAP",
    "LOAD_ATTR", "COMPARE_OP", "IMPORT_NAME", "IMPORT_FROM", "<109>",
    "JUMP_FORWARD", "JUMP_IF_FALSE", "JUMP_IF_TRUE", "JUMP_ABSOLUTE",
    "FOR_LOOP", "LOAD_LOCAL", "LOAD_GLOBAL", "SET_FUNC_ARGS", "<118>", "<119>",
    "SETUP_LOOP", "SETUP_EXCEPT", "SETUP_FINALLY", "RESERVE_FAST",
    "LOAD_FAST", "STORE_FAST", "DELETE_FAST", "SET_LINENO", "<128>", "<129>",
    "RAISE_VARARGS", "CALL_FUNCTION", "MAKE_FUNCTION", "BUILD_SLICE",
    "<134>", "<135>", "<136>", "<137>", "<138>", "<139>",
    "CALL_FUNCTION_VAR", "CALL_FUNCTION_KW", "CALL_FUNCTION_VAR_KW", "<143>",
    "<144>", "<145>", "<146>", "<147>", "<148>", "<149>", "<150>", "<151>",
    "<152>", "<153>", "<154>", "<155>", "<156>", "<157>", "<158>", "<159>",
    "<160>", "<161>", "<162>", "<163>", "<164>", "<165>", "<166>", "<167>",
    "<168>", "<169>", "<170>", "<171>", "<172>", "<173>", "<174>", "<175>",
    "<176>", "<177>", "<178>", "<179>", "<180>", "<181>", "<182>", "<183>",
    "<184>", "<185>", "<186>", "<187>", "<188>", "<189>", "<190>", "<191>",
    "<192>", "<193>", "<194>", "<195>", "<196>", "<197>", "<198>", "<199>",
    "<200>", "<201>", "<202>", "<203>", "<204>", "<205>", "<206>", "<207>",
    "<208>", "<209>", "<210>", "<211>", "<212>", "<213>", "<214>", "<215>",
    "<216>", "<217>", "<218>", "<219>", "<220>", "<221>", "<222>", "<223>",
    "<224>", "<225>", "<226>", "<227>", "<228>", "<229>", "<230>", "<231>",
    "<232>", "<233>", "<234>", "<235>", "<236>", "<237>", "<238>", "<239>",
    "<240>", "<241>", "<242>", "<243>", "<244>", "<245>", "<246>", "<247>",
    "<248>", "<249>", "<250>", "<251>", "<252>", "<253>", "<254>", "<255>",
};

const char* Py2k::OpcodeNames[256] = {
    "STOP_CODE", "POP_TOP", "ROT_TWO", "ROT_THREE", "DUP_TOP", "ROT_FOUR",
    "<6>", "<7>", "<8>", "NOP",
    "UNARY_POSITIVE", "UNARY_NEGATIVE", "UNARY_NOT", "UNARY_CONVERT",
    "<14>", "UNARY_INVERT", "<16>", "<17>", "LIST_APPEND",
    "BINARY_POWER", "BINARY_MULTIPLY", "BINARY_DIVIDE", "BINARY_MODULO",
    "BINARY_ADD", "BINARY_SUBTRACT", "BINARY_SUBSCR", "BINARY_FLOOR_DIVIDE",
    "BINARY_TRUE_DIVIDE", "INPLACE_FLOOR_DIVIDE", "INPLACE_TRUE_DIVIDE",
    "SLICE+0", "SLICE+1", "SLICE+2", "SLICE+3",
    "<34>", "<35>", "<36>", "<37>", "<38>", "<39>",
    "STORE_SLICE+0", "STORE_SLICE+1", "STORE_SLICE+2", "STORE_SLICE+3",
    "<44>", "<45>", "<46>", "<47>", "<48>", "<49>",
    "DELETE_SLICE+0", "DELETE_SLICE+1", "DELETE_SLICE+2", "DELETE_SLICE+3",
    "STORE_MAP", "INPLACE_ADD", "INPLACE_SUBTRACT", "INPLACE_MULTIPLY",
    "INPLACE_DIVIDE", "INPLACE_MODULO",
    "STORE_SUBSCR", "DELETE_SUBSCR", "BINARY_LSHIFT", "BINARY_RSHIFT",
    "BINARY_AND", "BINARY_XOR", "BINARY_OR", "INPLACE_POWER", "GET_ITER", "<69>",
    "PRINT_EXPR", "PRINT_ITEM", "PRINT_NEWLINE", "PRINT_ITEM_TO",
    "PRINT_NEWLINE_TO", "INPLACE_LSHIFT", "INPLACE_RSHIFT", "INPLACE_AND",
    "INPLACE_XOR", "INPLACE_OR",
    "BREAK_LOOP", "WITH_CLEANUP", "LOAD_LOCALS", "RETURN_VALUE",
    "IMPORT_STAR", "EXEC_STMT", "YIELD_VALUE", "POP_BLOCK", "END_FINALLY",
    "BUILD_CLASS",
    "STORE_NAME", "DELETE_NAME", "UNPACK_SEQUENCE", "FOR_ITER", "<94>",
    "STORE_ATTR", "DELETE_ATTR", "STORE_GLOBAL", "DELETE_GLOBAL", "DUP_TOPX",
    "LOAD_CONST", "LOAD_NAME", "BUILD_TUPLE", "BUILD_LIST", "BUILD_MAP",
    "LOAD_ATTR", "COMPARE_OP", "IMPORT_NAME", "IMPORT_FROM", "<109>",
    "JUMP_FORWARD", "JUMP_IF_FALSE", "JUMP_IF_TRUE", "JUMP_ABSOLUTE",
    "FOR_LOOP", "<115>", "LOAD_GLOBAL", "<117>", "<118>", "CONTINUE_LOOP",
    "SETUP_LOOP", "SETUP_EXCEPT", "SETUP_FINALLY", "<123>",
    "LOAD_FAST", "STORE_FAST", "DELETE_FAST", "SET_LINENO", "<128>", "<129>",
    "RAISE_VARARGS", "CALL_FUNCTION", "MAKE_FUNCTION", "BUILD_SLICE",
    "MAKE_CLOSURE", "LOAD_CLOSURE", "LOAD_DEREF", "STORE_DEREF", "<138>", "<139>",
    "CALL_FUNCTION_VAR", "CALL_FUNCTION_KW", "CALL_FUNCTION_VAR_KW", "EXTENDED_ARG",
    "<144>", "<145>", "<146>", "<147>", "<148>", "<149>", "<150>", "<151>",
    "<152>", "<153>", "<154>", "<155>", "<156>", "<157>", "<158>", "<159>",
    "<160>", "<161>", "<162>", "<163>", "<164>", "<165>", "<166>", "<167>",
    "<168>", "<169>", "<170>", "<171>", "<172>", "<173>", "<174>", "<175>",
    "<176>", "<177>", "<178>", "<179>", "<180>", "<181>", "<182>", "<183>",
    "<184>", "<185>", "<186>", "<187>", "<188>", "<189>", "<190>", "<191>",
    "<192>", "<193>", "<194>", "<195>", "<196>", "<197>", "<198>", "<199>",
    "<200>", "<201>", "<202>", "<203>", "<204>", "<205>", "<206>", "<207>",
    "<208>", "<209>", "<210>", "<211>", "<212>", "<213>", "<214>", "<215>",
    "<216>", "<217>", "<218>", "<219>", "<220>", "<221>", "<222>", "<223>",
    "<224>", "<225>", "<226>", "<227>", "<228>", "<229>", "<230>", "<231>",
    "<232>", "<233>", "<234>", "<235>", "<236>", "<237>", "<238>", "<239>",
    "<240>", "<241>", "<242>", "<243>", "<244>", "<245>", "<246>", "<247>",
    "<248>", "<249>", "<250>", "<251>", "<252>", "<253>", "<254>", "<255>",
};

const char* Py3k::OpcodeNames[256] = {
    "STOP_CODE", "POP_TOP", "ROT_TWO", "ROT_THREE", "DUP_TOP", "ROT_FOUR",
    "<6>", "<7>", "<8>", "NOP",
    "UNARY_POSITIVE", "UNARY_NEGATIVE", "UNARY_NOT", "<13>", "<14>",
    "UNARY_INVERT", "<16>", "SET_ADD", "LIST_APPEND",
    "BINARY_POWER", "BINARY_MULTIPLY", "<21>", "BINARY_MODULO",
    "BINARY_ADD", "BINARY_SUBTRACT", "BINARY_SUBSCR", "BINARY_FLOOR_DIVIDE",
    "BINARY_TRUE_DIVIDE", "INPLACE_FLOOR_DIVIDE", "INPLACE_TRUE_DIVIDE",
    "<30>", "<31>", "<32>", "<33>", "<34>", "<35>", "<36>", "<37>", "<38>", "<39>",
    "<40>", "<41>", "<42>", "<43>", "<44>", "<45>", "<46>", "<47>", "<48>", "<49>",
    "<50>", "<51>", "<52>", "<53>",
    "STORE_MAP", "INPLACE_ADD", "INPLACE_SUBTRACT", "INPLACE_MULTIPLY",
    "<58>", "INPLACE_MODULO",
    "STORE_SUBSCR", "DELETE_SUBSCR", "BINARY_LSHIFT", "BINARY_RSHIFT",
    "BINARY_AND", "BINARY_XOR", "BINARY_OR", "INPLACE_POWER", "GET_ITER",
    "STORE_LOCALS",
    "PRINT_EXPR", "LOAD_BUILD_CLASS", "<72>", "<73>", "<74>",
    "INPLACE_LSHIFT", "INPLACE_RSHIFT", "INPLACE_AND", "INPLACE_XOR",
    "INPLACE_OR",
    "BREAK_LOOP", "WITH_CLEANUP", "<82>", "RETURN_VALUE",
    "IMPORT_STAR", "<85>", "YIELD_VALUE", "POP_BLOCK", "END_FINALLY",
    "POP_EXCEPT",
    "STORE_NAME", "DELETE_NAME", "UNPACK_SEQUENCE", "FOR_ITER", "UNPACK_EX",
    "STORE_ATTR", "DELETE_ATTR", "STORE_GLOBAL", "DELETE_GLOBAL", "DUP_TOPX",
    "LOAD_CONST", "LOAD_NAME", "BUILD_TUPLE", "BUILD_LIST", "BUILD_SET",
    "BUILD_MAP", "LOAD_ATTR", "COMPARE_OP", "IMPORT_NAME", "IMPORT_FROM",
    "JUMP_FORWARD", "JUMP_IF_FALSE", "JUMP_IF_TRUE", "JUMP_ABSOLUTE",
    "POP_JUMP_IF_FALSE", "POP_JUMP_IF_TRUE", "LOAD_GLOBAL", "<117>", "<118>",
    "CONTINUE_LOOP", "SETUP_LOOP", "SETUP_EXCEPT", "SETUP_FINALLY", "<123>",
    "LOAD_FAST", "STORE_FAST", "DELETE_FAST", "<127>", "<128>", "<129>",
    "RAISE_VARARGS", "CALL_FUNCTION", "MAKE_FUNCTION", "BUILD_SLICE",
    "MAKE_CLOSURE", "LOAD_CLOSURE", "LOAD_DEREF", "STORE_DEREF", "<138>", "<139>",
    "CALL_FUNCTION_VAR", "CALL_FUNCTION_KW", "CALL_FUNCTION_VAR_KW",
    "EXTENDED_ARG", "<144>", "LIST_APPEND", "SET_ADD", "MAP_ADD",
    "<148>", "<149>", "<150>", "<151>",
    "<152>", "<153>", "<154>", "<155>", "<156>", "<157>", "<158>", "<159>",
    "<160>", "<161>", "<162>", "<163>", "<164>", "<165>", "<166>", "<167>",
    "<168>", "<169>", "<170>", "<171>", "<172>", "<173>", "<174>", "<175>",
    "<176>", "<177>", "<178>", "<179>", "<180>", "<181>", "<182>", "<183>",
    "<184>", "<185>", "<186>", "<187>", "<188>", "<189>", "<190>", "<191>",
    "<192>", "<193>", "<194>", "<195>", "<196>", "<197>", "<198>", "<199>",
    "<200>", "<201>", "<202>", "<203>", "<204>", "<205>", "<206>", "<207>",
    "<208>", "<209>", "<210>", "<211>", "<212>", "<213>", "<214>", "<215>",
    "<216>", "<217>", "<218>", "<219>", "<220>", "<221>", "<222>", "<223>",
    "<224>", "<225>", "<226>", "<227>", "<228>", "<229>", "<230>", "<231>",
    "<232>", "<233>", "<234>", "<235>", "<236>", "<237>", "<238>", "<239>",
    "<240>", "<241>", "<242>", "<243>", "<244>", "<245>", "<246>", "<247>",
    "<248>", "<249>", "<250>", "<251>", "<252>", "<253>", "<254>", "<255>",
};

bool Py1k::IsConstArg(int opcode)
{
    return (opcode == Py1k::LOAD_CONST) || (opcode == Py1k::RESERVE_FAST);
}

bool Py1k::IsNameArg(int opcode)
{
    return (opcode == Py1k::DELETE_ATTR) || (opcode == Py1k::DELETE_GLOBAL) ||
           (opcode == Py1k::DELETE_NAME) || (opcode == Py1k::IMPORT_FROM) ||
           (opcode == Py1k::IMPORT_NAME) || (opcode == Py1k::LOAD_ATTR) ||
           (opcode == Py1k::LOAD_GLOBAL) || (opcode == Py1k::LOAD_LOCAL) ||
           (opcode == Py1k::LOAD_NAME) || (opcode == Py1k::STORE_ATTR) ||
           (opcode == Py1k::STORE_GLOBAL) || (opcode == Py1k::STORE_NAME);
}

bool Py1k::IsVarNameArg(int opcode)
{
    return (opcode == Py1k::DELETE_FAST) || (opcode == Py1k::LOAD_FAST) ||
           (opcode == Py1k::STORE_FAST);
}

bool Py2k::IsConstArg(int opcode)
{
    return (opcode == Py2k::LOAD_CONST);
}

bool Py2k::IsNameArg(int opcode)
{
    return (opcode == Py2k::DELETE_ATTR) || (opcode == Py2k::DELETE_GLOBAL) ||
           (opcode == Py2k::DELETE_NAME) || (opcode == Py2k::IMPORT_FROM) ||
           (opcode == Py2k::IMPORT_NAME) || (opcode == Py2k::LOAD_ATTR) ||
           (opcode == Py2k::LOAD_GLOBAL) || (opcode == Py2k::LOAD_NAME) ||
           (opcode == Py2k::STORE_ATTR) || (opcode == Py2k::STORE_GLOBAL) ||
           (opcode == Py2k::STORE_NAME);
}

bool Py2k::IsVarNameArg(int opcode)
{
    return (opcode == Py2k::DELETE_FAST) || (opcode == Py2k::LOAD_FAST) ||
           (opcode == Py2k::STORE_FAST);
}

bool Py2k::IsCellArg(int opcode)
{
    return (opcode == Py2k::LOAD_CLOSURE) || (opcode == Py2k::LOAD_DEREF) ||
           (opcode == Py2k::STORE_DEREF);
}

bool Py3k::IsConstArg(int opcode)
{
    return (opcode == Py3k::LOAD_CONST);
}

bool Py3k::IsNameArg(int opcode)
{
    return (opcode == Py3k::DELETE_ATTR) || (opcode == Py3k::DELETE_GLOBAL) ||
           (opcode == Py3k::DELETE_NAME) || (opcode == Py3k::IMPORT_FROM) ||
           (opcode == Py3k::IMPORT_NAME) || (opcode == Py3k::LOAD_ATTR) ||
           (opcode == Py3k::LOAD_GLOBAL) || (opcode == Py3k::LOAD_NAME) ||
           (opcode == Py3k::STORE_ATTR) || (opcode == Py3k::STORE_GLOBAL) ||
           (opcode == Py3k::STORE_NAME);
}

bool Py3k::IsVarNameArg(int opcode)
{
    return (opcode == Py3k::DELETE_FAST) || (opcode == Py3k::LOAD_FAST) ||
           (opcode == Py3k::STORE_FAST);
}

bool Py3k::IsCellArg(int opcode)
{
    return (opcode == Py3k::LOAD_CLOSURE) || (opcode == Py3k::LOAD_DEREF) ||
           (opcode == Py3k::STORE_DEREF);
}


void print_const(PycRef<PycObject> obj, PycModule* mod)
{
    switch (obj->type()) {
    case PycObject::TYPE_STRING:
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0);
        break;
    case PycObject::TYPE_UNICODE:
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u');
        break;
    case PycObject::TYPE_TUPLE:
        {
            printf("(");
            PycTuple::value_t values = obj.cast<PycTuple>()->values();
            PycTuple::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    printf(", ");
                    print_const(*it, mod);
                }
            }
            if (values.size() == 1)
                printf(",)");
            else
                printf(")");
        }
        break;
    case PycObject::TYPE_LIST:
        {
            printf("[");
            PycList::value_t values = obj.cast<PycList>()->values();
            PycList::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    printf(", ");
                    print_const(*it, mod);
                }
            }
            printf("]");
        }
        break;
    case PycObject::TYPE_DICT:
        {
            printf("{");
            PycDict::key_t keys = obj.cast<PycDict>()->keys();
            PycDict::value_t values = obj.cast<PycDict>()->values();
            PycDict::key_t::const_iterator ki = keys.begin();
            PycDict::value_t::const_iterator vi = values.begin();
            if (ki != keys.end()) {
                print_const(*ki, mod);
                printf(": ");
                print_const(*vi, mod);
                while (++ki != keys.end()) {
                    ++vi;
                    printf(", ");
                    print_const(*ki, mod);
                    printf(": ");
                    print_const(*vi, mod);
                }
            }
            printf("}");
        }
        break;
    case PycObject::TYPE_SET:
        {
            printf("{");
            PycSet::value_t values = obj.cast<PycSet>()->values();
            PycSet::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    printf(", ");
                    print_const(*it, mod);
                }
            }
            printf("}");
        }
        break;
    case PycObject::TYPE_NONE:
        printf("None");
        break;
    case PycObject::TYPE_TRUE:
        printf("True");
        break;
    case PycObject::TYPE_FALSE:
        printf("False");
        break;
    case PycObject::TYPE_INT:
        printf("%d", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_FLOAT:
        printf("%s", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        printf("(%s+%sj)", obj.cast<PycComplex>()->value(),
                           obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        printf("%g", obj.cast<PycCFloat>()->value());
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        printf("(%g+%gj)", obj.cast<PycCComplex>()->value(),
                           obj.cast<PycCComplex>()->imag());
        break;
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        printf("<CODE> %s", obj.cast<PycCode>()->name()->value());
        break;
    }
}

void bc_next(PycBuffer& source, PycModule* mod, int& opcode, int& operand, int& pos)
{
    opcode = source.getByte();
    operand = 0;
    bool haveExtArg = false;
    pos += 1;

    if ((mod->majorVer() == 2 && opcode == Py2k::EXTENDED_ARG) ||
        (mod->majorVer() == 3 && opcode == Py3k::EXTENDED_ARG)) {
        operand = source.get16() << 16;
        opcode = source.getByte();
        haveExtArg = true;
        pos += 3;
    }
    if (opcode >= HAVE_ARG) {
        // If we have an extended arg, we want to OR the lower part,
        // else we want the whole thing (in case it's negative).  We use
        // the bool so that values between 0x8000 and 0xFFFF can be stored
        // without becoming negative
        if (haveExtArg)
            operand |= (source.get16() & 0xFFFF);
        else
            operand = source.get16();
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
            printf("    ");
        printf("%-7d ", pos);   // Current bytecode position

        bc_next(source, mod, opcode, operand, pos);

        if (mod->majorVer() == 1) {
            printf("%-24s", Py1k::OpcodeNames[opcode]);
        } else if (mod->majorVer() == 2) {
            printf("%-24s", Py2k::OpcodeNames[opcode]);
        } else if (mod->majorVer() == 3) {
            printf("%-24s", Py3k::OpcodeNames[opcode]);
        }
        if (opcode >= HAVE_ARG) {
            if ((mod->majorVer() == 1 && Py1k::IsConstArg(opcode)) ||
                (mod->majorVer() == 2 && Py2k::IsConstArg(opcode)) ||
                (mod->majorVer() == 3 && Py3k::IsConstArg(opcode))) {
                printf("%d: ", operand);
                print_const(code->getConst(operand), mod);
            } else if ((mod->majorVer() == 1 && Py1k::IsNameArg(opcode)) ||
                       (mod->majorVer() == 1 && mod->minorVer() < 3 && Py1k::IsVarNameArg(opcode)) ||
                       (mod->majorVer() == 2 && Py2k::IsNameArg(opcode)) ||
                       (mod->majorVer() == 3 && Py3k::IsNameArg(opcode))) {
                printf("%d: %s", operand, code->getName(operand)->value());
            } else if ((mod->majorVer() == 1 && Py1k::IsVarNameArg(opcode)) ||
                       (mod->majorVer() == 2 && Py2k::IsVarNameArg(opcode)) ||
                       (mod->majorVer() == 3 && Py3k::IsVarNameArg(opcode))) {
                printf("%d: %s", operand, code->getVarName(operand)->value());
            } else if ((mod->majorVer() == 2 && Py2k::IsCellArg(opcode)) ||
                       (mod->majorVer() == 3 && Py3k::IsCellArg(opcode))) {
                printf("%d: ", operand);
                print_const(code->getConst(operand), mod);
            } else {
                printf("%d", operand);
            }
        }
        printf("\n");
    }
}
