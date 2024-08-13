#include "pyc_numeric.h"
#include "bytecode.h"
#include <stdexcept>
#include <cstdint>
#include <cmath>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define DECLARE_PYTHON(maj, min) \
    extern int python_##maj##_##min##_map(int);

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
DECLARE_PYTHON(3, 10)
DECLARE_PYTHON(3, 11)
DECLARE_PYTHON(3, 12)
DECLARE_PYTHON(3, 13)

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
        case 0: return python_1_0_map(opcode);
        case 1: return python_1_1_map(opcode);
        case 3: return python_1_3_map(opcode);
        case 4: return python_1_4_map(opcode);
        case 5: return python_1_5_map(opcode);
        case 6: return python_1_6_map(opcode);
        }
        break;
    case 2:
        switch (min) {
        case 0: return python_2_0_map(opcode);
        case 1: return python_2_1_map(opcode);
        case 2: return python_2_2_map(opcode);
        case 3: return python_2_3_map(opcode);
        case 4: return python_2_4_map(opcode);
        case 5: return python_2_5_map(opcode);
        case 6: return python_2_6_map(opcode);
        case 7: return python_2_7_map(opcode);
        }
        break;
    case 3:
        switch (min) {
        case 0: return python_3_0_map(opcode);
        case 1: return python_3_1_map(opcode);
        case 2: return python_3_2_map(opcode);
        case 3: return python_3_3_map(opcode);
        case 4: return python_3_4_map(opcode);
        case 5: return python_3_5_map(opcode);
        case 6: return python_3_6_map(opcode);
        case 7: return python_3_7_map(opcode);
        case 8: return python_3_8_map(opcode);
        case 9: return python_3_9_map(opcode);
        case 10: return python_3_10_map(opcode);
        case 11: return python_3_11_map(opcode);
        case 12: return python_3_12_map(opcode);
        case 13: return python_3_13_map(opcode);
        }
        break;
    }
    return PYC_INVALID_OPCODE;
}

void print_const(std::ostream& pyc_output, PycRef<PycObject> obj, PycModule* mod,
                 const char* parent_f_string_quote)
{
    if (obj == NULL) {
        pyc_output << "<NULL>";
        return;
    }

    switch (obj->type()) {
    case PycObject::TYPE_STRING:
    case PycObject::TYPE_UNICODE:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        obj.cast<PycString>()->print(pyc_output, mod, false, parent_f_string_quote);
        break;
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            pyc_output << "(";
            PycTuple::value_t values = obj.cast<PycTuple>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(pyc_output, *it, mod);
                while (++it != values.cend()) {
                    pyc_output << ", ";
                    print_const(pyc_output, *it, mod);
                }
            }
            if (values.size() == 1)
                pyc_output << ",)";
            else
                pyc_output << ")";
        }
        break;
    case PycObject::TYPE_LIST:
        {
            pyc_output << "[";
            PycList::value_t values = obj.cast<PycList>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(pyc_output, *it, mod);
                while (++it != values.cend()) {
                    pyc_output << ", ";
                    print_const(pyc_output, *it, mod);
                }
            }
            pyc_output << "]";
        }
        break;
    case PycObject::TYPE_DICT:
        {
            pyc_output << "{";
            PycDict::value_t values = obj.cast<PycDict>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(pyc_output, std::get<0>(*it), mod);
                pyc_output << ": ";
                print_const(pyc_output, std::get<1>(*it), mod);
                while (++it != values.cend()) {
                    pyc_output << ", ";
                    print_const(pyc_output, std::get<0>(*it), mod);
                    pyc_output << ": ";
                    print_const(pyc_output, std::get<1>(*it), mod);
                }
            }
            pyc_output << "}";
        }
        break;
    case PycObject::TYPE_SET:
        {
            pyc_output << "{";
            PycSet::value_t values = obj.cast<PycSet>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(pyc_output, *it, mod);
                while (++it != values.cend()) {
                    pyc_output << ", ";
                    print_const(pyc_output, *it, mod);
                }
            }
            pyc_output << "}";
        }
        break;
    case PycObject::TYPE_FROZENSET:
        {
            pyc_output << "frozenset({";
            PycSet::value_t values = obj.cast<PycSet>()->values();
            auto it = values.cbegin();
            if (it != values.cend()) {
                print_const(pyc_output, *it, mod);
                while (++it != values.cend()) {
                    pyc_output << ", ";
                    print_const(pyc_output, *it, mod);
                }
            }
            pyc_output << "})";
        }
        break;
    case PycObject::TYPE_NONE:
        pyc_output << "None";
        break;
    case PycObject::TYPE_TRUE:
        pyc_output << "True";
        break;
    case PycObject::TYPE_FALSE:
        pyc_output << "False";
        break;
    case PycObject::TYPE_ELLIPSIS:
        pyc_output << "...";
        break;
    case PycObject::TYPE_INT:
        formatted_print(pyc_output, "%d", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_LONG:
        formatted_print(pyc_output, "%s", obj.cast<PycLong>()->repr(mod).c_str());
        break;
    case PycObject::TYPE_FLOAT:
        formatted_print(pyc_output, "%s", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        formatted_print(pyc_output, "(%s+%sj)", obj.cast<PycComplex>()->value(),
                                        obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        {
            // Wrap any nan/inf values in float('').
            double value = obj.cast<PycCFloat>()->value();
            bool is_negative = std::signbit(value);
            if (std::isnan(value)) {
                if (is_negative) {
                    pyc_output << "float('-nan')";
                } else {
                    pyc_output << "float('nan')";
                }
            } else if (std::isinf(value)) {
                if (is_negative) {
                    pyc_output << "float('-inf')";
                } else {
                    pyc_output << "float('inf')";
                }
            } else {
                formatted_print(pyc_output, "%g", value);
            }
        }
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        formatted_print(pyc_output, "(%g+%gj)", obj.cast<PycCComplex>()->value(),
                                        obj.cast<PycCComplex>()->imag());
        break;
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        pyc_output << "<CODE> " << obj.cast<PycCode>()->name()->value();
        break;
    default:
        formatted_print(pyc_output, "<TYPE: %d>\n", obj->type());
    }
}

void bc_next(PycBuffer& source, PycModule* mod, int& opcode, int& operand, int& pos)
{
    opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
    if (mod->verCompare(3, 6) >= 0) {
        operand = source.getByte();
        pos += 2;
        if (opcode == Pyc::EXTENDED_ARG_A) {
            opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
            operand = (operand << 8) | source.getByte();
            pos += 2;
        }
    } else {
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
}

int bc_get_at(PycBuffer& source, PycModule *mod, int& opcode, int &operand, int pos)
{
    /* save current pos */
    int old_pos = source.setPos(pos);
    if (old_pos == EOF)
    {
        opcode = 0;
        operand = 0;
        return EOF;
    }

    /* read opcode at requested pos */
    bc_next(source, mod, opcode, operand, pos);

    /* restore saved pos */
    source.setPos(old_pos);
    return pos;
}

void bc_disasm(std::ostream& pyc_output, PycRef<PycCode> code, PycModule* mod,
               int indent, unsigned flags)
{
    static const char *cmp_strings[] = {
        "<", "<=", "==", "!=", ">", ">=", "in", "not in", "is", "is not",
        "<EXCEPTION MATCH>", "<BAD>"
    };
    static const size_t cmp_strings_len = sizeof(cmp_strings) / sizeof(cmp_strings[0]);

    static const char *binop_strings[] = {
        "+", "&", "//", "<<", "@", "*", "%", "|", "**", ">>", "-", "/", "^",
        "+=", "&=", "//=", "<<=", "@=", "*=", "%=", "|=", "**=", ">>=", "-=", "/=", "^=",
    };
    static const size_t binop_strings_len = sizeof(binop_strings) / sizeof(binop_strings[0]);

    static const char *intrinsic1_names[] = {
        "INTRINSIC_1_INVALID", "INTRINSIC_PRINT", "INTRINSIC_IMPORT_STAR",
        "INTRINSIC_STOPITERATION_ERROR", "INTRINSIC_ASYNC_GEN_WRAP",
        "INTRINSIC_UNARY_POSITIVE", "INTRINSIC_LIST_TO_TUPLE", "INTRINSIC_TYPEVAR",
        "INTRINSIC_PARAMSPEC", "INTRINSIC_TYPEVARTUPLE",
        "INTRINSIC_SUBSCRIPT_GENERIC", "INTRINSIC_TYPEALIAS",
    };
    static const size_t intrinsic1_names_len = sizeof(intrinsic1_names) / sizeof(intrinsic1_names[0]);

    static const char *intrinsic2_names[] = {
        "INTRINSIC_2_INVALID", "INTRINSIC_PREP_RERAISE_STAR",
        "INTRINSIC_TYPEVAR_WITH_BOUND", "INTRINSIC_TYPEVAR_WITH_CONSTRAINTS",
        "INTRINSIC_SET_FUNCTION_TYPE_PARAMS", "INTRINSIC_SET_TYPEPARAM_DEFAULT",
    };
    static const size_t intrinsic2_names_len = sizeof(intrinsic2_names) / sizeof(intrinsic2_names[0]);

    static const char *format_value_names[] = {
        "FVC_NONE", "FVC_STR", "FVC_REPR", "FVC_ASCII",
    };
    static const size_t format_value_names_len = sizeof(format_value_names) / sizeof(format_value_names[0]);

    PycBuffer source(code->code()->value(), code->code()->length());

    int opcode, operand;
    int pos = 0;
    while (!source.atEof()) {
        int start_pos = pos;
        bc_next(source, mod, opcode, operand, pos);
        if (opcode == Pyc::CACHE && (flags & Pyc::DISASM_SHOW_CACHES) == 0)
            continue;

        for (int i=0; i<indent; i++)
            pyc_output << "    ";
        formatted_print(pyc_output, "%-7d %-30s  ", start_pos, Pyc::OpcodeName(opcode));

        if (opcode >= Pyc::PYC_HAVE_ARG) {
            switch (opcode) {
            case Pyc::LOAD_CONST_A:
            case Pyc::RESERVE_FAST_A:
            case Pyc::KW_NAMES_A:
            case Pyc::RETURN_CONST_A:
            case Pyc::INSTRUMENTED_RETURN_CONST_A:
                try {
                    auto constParam = code->getConst(operand);
                    formatted_print(pyc_output, "%d: ", operand);
                    print_const(pyc_output, constParam, mod);
                } catch (const std::out_of_range &) {
                    formatted_print(pyc_output, "%d <INVALID>", operand);
                }
                break;
            case Pyc::LOAD_GLOBAL_A:
                try {
                    // Special case for Python 3.11+
                    if (mod->verCompare(3, 11) >= 0) {
                        if (operand & 1)
                            formatted_print(pyc_output, "%d: NULL + %s", operand, code->getName(operand >> 1)->value());
                        else
                            formatted_print(pyc_output, "%d: %s", operand, code->getName(operand >> 1)->value());
                    } else {
                        formatted_print(pyc_output, "%d: %s", operand, code->getName(operand)->value());
                    }
                } catch (const std::out_of_range &) {
                    formatted_print(pyc_output, "%d <INVALID>", operand);
                }
                break;
            case Pyc::DELETE_ATTR_A:
            case Pyc::DELETE_GLOBAL_A:
            case Pyc::DELETE_NAME_A:
            case Pyc::IMPORT_FROM_A:
            case Pyc::IMPORT_NAME_A:
            case Pyc::LOAD_ATTR_A:
            case Pyc::LOAD_LOCAL_A:
            case Pyc::LOAD_NAME_A:
            case Pyc::STORE_ATTR_A:
            case Pyc::STORE_GLOBAL_A:
            case Pyc::STORE_NAME_A:
            case Pyc::STORE_ANNOTATION_A:
            case Pyc::LOAD_METHOD_A:
            case Pyc::LOAD_FROM_DICT_OR_GLOBALS_A:
                try {
                    auto arg = operand;
                    if (opcode == Pyc::LOAD_ATTR_A && mod->verCompare(3, 12) >= 0)
                        arg >>= 1;
                    formatted_print(pyc_output, "%d: %s", operand, code->getName(arg)->value());
                } catch (const std::out_of_range &) {
                    formatted_print(pyc_output, "%d <INVALID>", operand);
                }
                break;
            case Pyc::LOAD_SUPER_ATTR_A:
            case Pyc::INSTRUMENTED_LOAD_SUPER_ATTR_A:
                try {
                    formatted_print(pyc_output, "%d: %s", operand, code->getName(operand >> 2)->value());
                } catch (const std::out_of_range &) {
                    formatted_print(pyc_output, "%d <INVALID>", operand);
                }
                break;
            case Pyc::DELETE_FAST_A:
            case Pyc::LOAD_FAST_A:
            case Pyc::STORE_FAST_A:
            case Pyc::LOAD_FAST_CHECK_A:
            case Pyc::LOAD_FAST_AND_CLEAR_A:
                try {
                    formatted_print(pyc_output, "%d: %s", operand, code->getLocal(operand)->value());
                } catch (const std::out_of_range &) {
                    formatted_print(pyc_output, "%d <INVALID>", operand);
                }
                break;
            case Pyc::LOAD_FAST_LOAD_FAST_A:
            case Pyc::STORE_FAST_LOAD_FAST_A:
            case Pyc::STORE_FAST_STORE_FAST_A:
                try {
                    formatted_print(pyc_output, "%d: %s, %s", operand,
                                    code->getLocal(operand >> 4)->value(),
                                    code->getLocal(operand & 0xF)->value());
                } catch (const std::out_of_range &) {
                    formatted_print(pyc_output, "%d <INVALID>", operand);
                }
                break;
            case Pyc::LOAD_CLOSURE_A:
            case Pyc::LOAD_DEREF_A:
            case Pyc::STORE_DEREF_A:
            case Pyc::DELETE_DEREF_A:
            case Pyc::MAKE_CELL_A:
            case Pyc::CALL_FINALLY_A:
            case Pyc::LOAD_FROM_DICT_OR_DEREF_A:
                try {
                    formatted_print(pyc_output, "%d: %s", operand, code->getCellVar(mod, operand)->value());
                } catch (const std::out_of_range &) {
                    formatted_print(pyc_output, "%d <INVALID>", operand);
                }
                break;
            case Pyc::JUMP_FORWARD_A:
            case Pyc::JUMP_IF_FALSE_A:
            case Pyc::JUMP_IF_TRUE_A:
            case Pyc::SETUP_LOOP_A:
            case Pyc::SETUP_FINALLY_A:
            case Pyc::SETUP_EXCEPT_A:
            case Pyc::FOR_LOOP_A:
            case Pyc::FOR_ITER_A:
            case Pyc::SETUP_WITH_A:
            case Pyc::SETUP_ASYNC_WITH_A:
            case Pyc::POP_JUMP_FORWARD_IF_FALSE_A:
            case Pyc::POP_JUMP_FORWARD_IF_TRUE_A:
            case Pyc::SEND_A:
            case Pyc::POP_JUMP_FORWARD_IF_NOT_NONE_A:
            case Pyc::POP_JUMP_FORWARD_IF_NONE_A:
            case Pyc::POP_JUMP_IF_NOT_NONE_A:
            case Pyc::POP_JUMP_IF_NONE_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_NOT_NONE_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_NONE_A:
            case Pyc::INSTRUMENTED_JUMP_FORWARD_A:
            case Pyc::INSTRUMENTED_FOR_ITER_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_FALSE_A:
            case Pyc::INSTRUMENTED_POP_JUMP_IF_TRUE_A:
                {
                    int offs = operand;
                    if (mod->verCompare(3, 10) >= 0)
                        offs *= sizeof(uint16_t); // BPO-27129
                    formatted_print(pyc_output, "%d (to %d)", operand, pos+offs);
                }
                break;
            case Pyc::JUMP_BACKWARD_NO_INTERRUPT_A:
            case Pyc::JUMP_BACKWARD_A:
            case Pyc::POP_JUMP_BACKWARD_IF_NOT_NONE_A:
            case Pyc::POP_JUMP_BACKWARD_IF_NONE_A:
            case Pyc::POP_JUMP_BACKWARD_IF_FALSE_A:
            case Pyc::POP_JUMP_BACKWARD_IF_TRUE_A:
            case Pyc::INSTRUMENTED_JUMP_BACKWARD_A:
                {
                    // BACKWARD jumps were only introduced in Python 3.11
                    int offs = operand * sizeof(uint16_t); // BPO-27129
                    formatted_print(pyc_output, "%d (to %d)", operand, pos-offs);
                }
                break;
            case Pyc::POP_JUMP_IF_FALSE_A:
            case Pyc::POP_JUMP_IF_TRUE_A:
            case Pyc::JUMP_IF_FALSE_OR_POP_A:
            case Pyc::JUMP_IF_TRUE_OR_POP_A:
            case Pyc::JUMP_ABSOLUTE_A:
            case Pyc::JUMP_IF_NOT_EXC_MATCH_A:
                if (mod->verCompare(3, 12) >= 0) {
                    // These are now relative as well
                    int offs = operand * sizeof(uint16_t);
                    formatted_print(pyc_output, "%d (to %d)", operand, pos+offs);
                } else if (mod->verCompare(3, 10) >= 0) {
                    // BPO-27129
                    formatted_print(pyc_output, "%d (to %d)", operand,
                                    int(operand * sizeof(uint16_t)));
                } else {
                    formatted_print(pyc_output, "%d", operand);
                }
                break;
            case Pyc::COMPARE_OP_A:
                {
                    auto arg = operand;
                    if (mod->verCompare(3, 12) == 0)
                        arg >>= 4; // changed under GH-100923
                    else if (mod->verCompare(3, 13) >= 0)
                        arg >>= 5;
                    if (static_cast<size_t>(arg) < cmp_strings_len)
                        formatted_print(pyc_output, "%d (%s)", operand, cmp_strings[arg]);
                    else
                        formatted_print(pyc_output, "%d (UNKNOWN)", operand);
                }
                break;
            case Pyc::BINARY_OP_A:
                if (static_cast<size_t>(operand) < binop_strings_len)
                    formatted_print(pyc_output, "%d (%s)", operand, binop_strings[operand]);
                else
                    formatted_print(pyc_output, "%d (UNKNOWN)", operand);
                break;
            case Pyc::IS_OP_A:
                formatted_print(pyc_output, "%d (%s)", operand, (operand == 0) ? "is"
                                                      : (operand == 1) ? "is not"
                                                      : "UNKNOWN");
                break;
            case Pyc::CONTAINS_OP_A:
                formatted_print(pyc_output, "%d (%s)", operand, (operand == 0) ? "in"
                                                      : (operand == 1) ? "not in"
                                                      : "UNKNOWN");
                break;
            case Pyc::CALL_INTRINSIC_1_A:
                if (static_cast<size_t>(operand) < intrinsic1_names_len)
                    formatted_print(pyc_output, "%d (%s)", operand, intrinsic1_names[operand]);
                else
                    formatted_print(pyc_output, "%d (UNKNOWN)", operand);
                break;
            case Pyc::CALL_INTRINSIC_2_A:
                if (static_cast<size_t>(operand) < intrinsic2_names_len)
                    formatted_print(pyc_output, "%d (%s)", operand, intrinsic2_names[operand]);
                else
                    formatted_print(pyc_output, "%d (UNKNOWN)", operand);
                break;
            case Pyc::FORMAT_VALUE_A:
                {
                    auto conv = static_cast<size_t>(operand & 0x03);
                    const char *flag = (operand & 0x04) ? " | FVS_HAVE_SPEC" : "";
                    if (conv < format_value_names_len) {
                        formatted_print(pyc_output, "%d (%s%s)", operand,
                                        format_value_names[conv], flag);
                    } else {
                        formatted_print(pyc_output, "%d (UNKNOWN)", operand);
                    }
                }
                break;
            case Pyc::CONVERT_VALUE_A:
                if (static_cast<size_t>(operand) < format_value_names_len)
                    formatted_print(pyc_output, "%d (%s)", operand, format_value_names[operand]);
                else
                    formatted_print(pyc_output, "%d (UNKNOWN)", operand);
                break;
            case Pyc::SET_FUNCTION_ATTRIBUTE_A:
                // This looks like a bitmask, but CPython treats it as an exclusive lookup...
                switch (operand) {
                case 0x01:
                    formatted_print(pyc_output, "%d (MAKE_FUNCTION_DEFAULTS)", operand);
                    break;
                case 0x02:
                    formatted_print(pyc_output, "%d (MAKE_FUNCTION_KWDEFAULTS)", operand);
                    break;
                case 0x04:
                    formatted_print(pyc_output, "%d (MAKE_FUNCTION_ANNOTATIONS)", operand);
                    break;
                case 0x08:
                    formatted_print(pyc_output, "%d (MAKE_FUNCTION_CLOSURE)", operand);
                    break;
                default:
                    formatted_print(pyc_output, "%d (UNKNOWN)", operand);
                    break;
                }
                break;
            default:
                formatted_print(pyc_output, "%d", operand);
                break;
            }
        }
        pyc_output << "\n";
    }
}
