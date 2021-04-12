#include "pyc_numeric.h"
#include "bytecode.h"
#include <cmath>

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
DECLARE_PYTHON(3, 6)
DECLARE_PYTHON(3, 7)
DECLARE_PYTHON(3, 8)
DECLARE_PYTHON(3, 9)

const char* Pyc::OpcodeName(int opcode)
{
    static const char* opcode_names[] = {
        #define OPCODE(x) #x,
        #define OPCODE_A_FIRST(x) #x,
        #define OPCODE_A(x) #x,
        #include "bytecode_ops.inl"
        #undef OPCODE_A
        #undef OPCODE_A_FIRST
        #undef OPCODE
    };

#if __cplusplus >= 201103L
    static_assert(sizeof(opcode_names) / sizeof(opcode_names[0]) == PYC_LAST_OPCODE,
                  "Pyc::OpcodeName opcode_names not in sync with opcode enum");
#endif

    if (opcode < 0)
        return "<INVALID>";

    if (opcode < PYC_LAST_OPCODE)
        return opcode_names[opcode];

    static char badcode[16];
    snprintf(badcode, sizeof(badcode), "<%d>", opcode);
    return badcode;
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
        case 6: return python_36_map(opcode);
        case 7: return python_37_map(opcode);
        case 8: return python_38_map(opcode);
        case 9: return python_39_map(opcode);
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
    return (opcode == Pyc::DELETE_ATTR_A) || (opcode == Pyc::DELETE_GLOBAL_A) ||
           (opcode == Pyc::DELETE_NAME_A) || (opcode == Pyc::IMPORT_FROM_A) ||
           (opcode == Pyc::IMPORT_NAME_A) || (opcode == Pyc::LOAD_ATTR_A) ||
           (opcode == Pyc::LOAD_GLOBAL_A) || (opcode == Pyc::LOAD_LOCAL_A) ||
           (opcode == Pyc::LOAD_NAME_A) || (opcode == Pyc::STORE_ATTR_A) ||
           (opcode == Pyc::STORE_GLOBAL_A) || (opcode == Pyc::STORE_NAME_A) ||
           (opcode == Pyc::LOAD_METHOD_A);
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
    return (opcode == Pyc::JUMP_FORWARD_A) || (opcode == Pyc::JUMP_IF_FALSE_A) ||
           (opcode == Pyc::JUMP_IF_TRUE_A) || (opcode == Pyc::SETUP_LOOP_A) ||
           (opcode == Pyc::SETUP_FINALLY_A) || (opcode == Pyc::SETUP_EXCEPT_A) ||
           (opcode == Pyc::FOR_LOOP_A) || (opcode == Pyc::FOR_ITER_A);
}

bool Pyc::IsCompareArg(int opcode)
{
    return (opcode == Pyc::COMPARE_OP_A);
}

void print_const(PycRef<PycObject> obj, PycModule* mod, const char* parent_f_string_quote)
{
    if (obj == NULL) {
        fputs("<NULL>", pyc_output);
        return;
    }

    switch (obj->type()) {
    case PycObject::TYPE_STRING:
        OutputString(obj.cast<PycString>(), mod->strIsUnicode() ? 'b' : 0,
                     false, pyc_output, parent_f_string_quote);
        break;
    case PycObject::TYPE_UNICODE:
        OutputString(obj.cast<PycString>(), mod->strIsUnicode() ? 0 : 'u',
                     false, pyc_output, parent_f_string_quote);
        break;
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        if (mod->majorVer() >= 3)
            OutputString(obj.cast<PycString>(), 0, false, pyc_output, parent_f_string_quote);
        else
            OutputString(obj.cast<PycString>(), mod->strIsUnicode() ? 'b' : 0,
                         false, pyc_output, parent_f_string_quote);
        break;
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            fputs("(", pyc_output);
            PycTuple::value_t values = obj.cast<PycTuple>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(*it, mod);
                while (++it != values.cend()) {
                    fputs(", ", pyc_output);
                    print_const(*it, mod);
                }
            }
            if (values.size() == 1)
                fputs(",)", pyc_output);
            else
                fputs(")", pyc_output);
        }
        break;
    case PycObject::TYPE_LIST:
        {
            fputs("[", pyc_output);
            PycList::value_t values = obj.cast<PycList>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(*it, mod);
                while (++it != values.cend()) {
                    fputs(", ", pyc_output);
                    print_const(*it, mod);
                }
            }
            fputs("]", pyc_output);
        }
        break;
    case PycObject::TYPE_DICT:
        {
            fputs("{", pyc_output);
            PycDict::key_t keys = obj.cast<PycDict>()->keys();
            PycDict::value_t values = obj.cast<PycDict>()->values();
            auto ki = keys.cbegin();
            auto vi = values.cbegin();
            if (ki != keys.cend()) {
                print_const(*ki, mod);
                fputs(": ", pyc_output);
                print_const(*vi, mod);
                while (++ki != keys.cend()) {
                    ++vi;
                    fputs(", ", pyc_output);
                    print_const(*ki, mod);
                    fputs(": ", pyc_output);
                    print_const(*vi, mod);
                }
            }
            fputs("}", pyc_output);
        }
        break;
    case PycObject::TYPE_SET:
        {
            fputs("{", pyc_output);
            PycSet::value_t values = obj.cast<PycSet>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(*it, mod);
                while (++it != values.cend()) {
                    fputs(", ", pyc_output);
                    print_const(*it, mod);
                }
            }
            fputs("}", pyc_output);
        }
        break;
    case PycObject::TYPE_NONE:
        fputs("None", pyc_output);
        break;
    case PycObject::TYPE_TRUE:
        fputs("True", pyc_output);
        break;
    case PycObject::TYPE_FALSE:
        fputs("False", pyc_output);
        break;
    case PycObject::TYPE_ELLIPSIS:
        fputs("...", pyc_output);
        break;
    case PycObject::TYPE_INT:
        fprintf(pyc_output, "%d", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_LONG:
        fprintf(pyc_output, "%s", obj.cast<PycLong>()->repr().c_str());
        break;
    case PycObject::TYPE_FLOAT:
        fprintf(pyc_output, "%s", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        fprintf(pyc_output, "(%s+%sj)", obj.cast<PycComplex>()->value(),
                                        obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        {
            // Wrap any nan/inf values in float('').
            double value = obj.cast<PycCFloat>()->value();
            bool is_negative = std::signbit(value);
            if (std::isnan(value)) {
                if (is_negative) {
                    fprintf(pyc_output, "float('-nan')");
                } else {
                    fprintf(pyc_output, "float('nan')");
                }
            } else if (std::isinf(value)) {
                if (is_negative) {
                    fprintf(pyc_output, "float('-inf')");
                } else {
                    fprintf(pyc_output, "float('inf')");
                }
            } else {
                fprintf(pyc_output, "%g", value);
            }
        }
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        fprintf(pyc_output, "(%g+%gj)", obj.cast<PycCComplex>()->value(),
                                        obj.cast<PycCComplex>()->imag());
        break;
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        fprintf(pyc_output, "<CODE> %s", obj.cast<PycCode>()->name()->value());
        break;
    }
}

void bc_next(PycBuffer& source, PycModule* mod, int& opcode, int& operand, int& pos)
{
    opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
    bool py36_opcode = (mod->majorVer() > 3 || (mod->majorVer() == 3 && mod->minorVer() >= 6));
    if (py36_opcode) {
        operand = source.getByte();
        pos += 2;
    } else {
        operand = 0;
        pos += 1;
    }

    if (opcode == Pyc::EXTENDED_ARG_A) {
        if (py36_opcode) {
            opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
            operand <<= 8;
            operand |= source.getByte();
            pos += 2;
        } else {
            operand = source.get16() << 16;
            opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
            pos += 3;
        }
    }
    if (!py36_opcode && (opcode >= Pyc::PYC_HAVE_ARG)) {
        operand |= source.get16();
        pos += 2;
    }
}

void bc_disasm(PycRef<PycCode> code, PycModule* mod, int indent)
{
    static const char *cmp_strings[] = {
        "<", "<=", "==", "!=", ">", ">=", "in", "not in", "is", "is not",
        "<EXCEPTION MATCH>", "<BAD>"
    };
    static const size_t cmp_strings_len = sizeof(cmp_strings) / sizeof(cmp_strings[0]);

    PycBuffer source(code->code()->value(), code->code()->length());

    int opcode, operand;
    int pos = 0;
    while (!source.atEof()) {
        for (int i=0; i<indent; i++)
            fputs("    ", pyc_output);
        fprintf(pyc_output, "%-7d ", pos);   // Current bytecode position

        bc_next(source, mod, opcode, operand, pos);
        fprintf(pyc_output, "%-24s", Pyc::OpcodeName(opcode));

        if (opcode >= Pyc::PYC_HAVE_ARG) {
            if (Pyc::IsConstArg(opcode)) {
                fprintf(pyc_output, "%d: ", operand);
                print_const(code->getConst(operand), mod);
            } else if (Pyc::IsNameArg(opcode)) {
                fprintf(pyc_output, "%d: %s", operand, code->getName(operand)->value());
            } else if (Pyc::IsVarNameArg(opcode)) {
                fprintf(pyc_output, "%d: %s", operand, code->getVarName(operand)->value());
            } else if (Pyc::IsCellArg(opcode)) {
                fprintf(pyc_output, "%d: %s", operand, code->getCellVar(operand)->value());
            } else if (Pyc::IsJumpOffsetArg(opcode)) {
                fprintf(pyc_output, "%d (to %d)", operand, pos+operand);
            } else if (Pyc::IsCompareArg(opcode)) {
                if (static_cast<size_t>(operand) < cmp_strings_len)
                    fprintf(pyc_output, "%d (%s)", operand, cmp_strings[operand]);
                else
                    fprintf(pyc_output, "%d (UNKNOWN)", operand);
            } else if (opcode == Pyc::IS_OP_A) {
                fprintf(pyc_output, "%d (%s)", operand, (operand == 0) ? "is"
                                                      : (operand == 1) ? "is not"
                                                      : "UNKNOWN");
            } else if (opcode == Pyc::CONTAINS_OP_A) {
                fprintf(pyc_output, "%d (%s)", operand, (operand == 0) ? "in"
                                                      : (operand == 1) ? "not in"
                                                      : "UNKNOWN");
            } else {
                fprintf(pyc_output, "%d", operand);
            }
        }
        fputs("\n", pyc_output);
    }
}
