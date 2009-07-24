#include "bytecode.h"
#include "data.h"

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

void bc_disasm(PycRef<PycCode> code, PycModule* mod, int indent)
{
    PycBuffer source(code->code()->value(), code->code()->length());

    int operand = 0;
    while (!source.atEof()) {
        int opcode = source.getByte();
        bool extArg = false;
        if ((mod->majorVer() == 2 && opcode == Py2k::EXTENDED_ARG) ||
            (mod->majorVer() == 3 && opcode == Py3k::EXTENDED_ARG)) {
            extArg = true;
            opcode = source.getByte();
        }
        if (opcode >= HAVE_ARG)
            operand = extArg ? source.get32() : source.get16();

        for (int i=0; i<indent; i++)
            printf("    ");
        if (mod->majorVer() == 1) {
            printf("%-24s", Py1k::OpcodeNames[opcode]);
        } else if (mod->majorVer() == 2) {
            printf("%-24s", Py2k::OpcodeNames[opcode]);
        } else if (mod->majorVer() == 3) {
            printf("%-24s", Py3k::OpcodeNames[opcode]);
        }
        if (opcode >= HAVE_ARG)
            printf("%d\n", operand);
        else
            printf("\n");
    }
}
