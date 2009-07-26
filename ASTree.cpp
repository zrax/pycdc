#include "ASTree.h"
#include "FastStack.h"
#include "bytecode.h"

// These are used to avoid writing code 3 times for each of
// the different python generations
#define PY_1000 0x1000
#define PY_2000 0x2000
#define PY_3000 0x3000

PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod)
{
    PycBuffer source(code->code()->value(), code->code()->length());
    ASTNodeList::list_t lines;

    FastStack stack((mod->majorVer() == 1) ? 20 : code->stackSize());
    stackhist_t stack_hist;

    int opcode, operand;
    int pos = 0;

    int opadd = 0;
    if (mod->majorVer() == 1)
        opadd = PY_1000;
    else if (mod->majorVer() == 2)
        opadd = PY_2000;
    else if (mod->majorVer() == 3)
        opadd = PY_3000;

    while (!source.atEof()) {
        bc_next(source, mod, opcode, operand, pos);
        opcode |= opadd;

        switch (opcode) {
        //case Py2k::STOP_CODE:
        //case Py2k::POP_TOP:
        //case Py2k::ROT_TWO:
        //case Py2k::ROT_THREE:
        //case Py2k::DUP_TOP:
        //case Py2k::ROT_FOUR:
        //case Py2k::NOP:
        //case Py2k::UNARY_POSITIVE:
        //case Py2k::UNARY_NEGATIVE:
        //case Py2k::UNARY_NOT:
        //case Py2k::UNARY_CONVERT:
        //case Py2k::UNARY_INVERT:
        //case Py2k::LIST_APPEND:
        //case Py2k::BINARY_POWER:
        //case Py2k::BINARY_MULTIPLY:
        //case Py2k::BINARY_DIVIDE:
        //case Py2k::BINARY_MODULO:
        //case Py2k::BINARY_ADD:
        //case Py2k::BINARY_SUBTRACT:
        //case Py2k::BINARY_SUBSCR:
        //case Py2k::BINARY_FLOOR_DIVIDE:
        //case Py2k::BINARY_TRUE_DIVIDE:
        //case Py2k::INPLACE_FLOOR_DIVIDE:
        //case Py2k::INPLACE_TRUE_DIVIDE:
        //case Py2k::SLICE_0:
        //case Py2k::SLICE_1:
        //case Py2k::SLICE_2:
        //case Py2k::SLICE_3:
        //case Py2k::STORE_SLICE_0:
        //case Py2k::STORE_SLICE_1:
        //case Py2k::STORE_SLICE_2:
        //case Py2k::STORE_SLICE_3:
        //case Py2k::DELETE_SLICE_0:
        //case Py2k::DELETE_SLICE_1:
        //case Py2k::DELETE_SLICE_2:
        //case Py2k::DELETE_SLICE_3:
        //case Py2k::STORE_MAP:
        //case Py2k::INPLACE_ADD:
        //case Py2k::INPLACE_SUBTRACT:
        //case Py2k::INPLACE_MULTIPLY:
        //case Py2k::INPLACE_DIVIDE:
        //case Py2k::INPLACE_MODULO:
        //case Py2k::STORE_SUBSCR:
        //case Py2k::DELETE_SUBSCR:
        //case Py2k::BINARY_LSHIFT:
        //case Py2k::BINARY_RSHIFT:
        //case Py2k::BINARY_AND:
        //case Py2k::BINARY_XOR:
        //case Py2k::BINARY_OR:
        //case Py2k::INPLACE_POWER:
        //case Py2k::GET_ITER:
        //case Py2k::PRINT_EXPR:
        //case Py2k::PRINT_ITEM:
        //case Py2k::PRINT_NEWLINE:
        //case Py2k::PRINT_ITEM_TO:
        //case Py2k::PRINT_NEWLINE_TO:
        //case Py2k::INPLACE_LSHIFT:
        //case Py2k::INPLACE_RSHIFT:
        //case Py2k::INPLACE_AND:
        //case Py2k::INPLACE_XOR:
        //case Py2k::INPLACE_OR:
        //case Py2k::BREAK_LOOP:
        //case Py2k::WITH_CLEANUP:
        //case Py2k::LOAD_LOCALS:
        //case Py2k::RETURN_VALUE:
        //case Py2k::IMPORT_STAR:
        //case Py2k::EXEC_STMT:
        //case Py2k::YIELD_VALUE:
        //case Py2k::POP_BLOCK:
        //case Py2k::END_FINALLY:
        //case Py2k::BUILD_CLASS:
        case (PY_1000 | Py1k::STORE_NAME):
        case (PY_2000 | Py2k::STORE_NAME):
        case (PY_3000 | Py3k::STORE_NAME):
            {
                PycRef<ASTNode> value = stack.top();
                PycRef<ASTNode> name = new ASTName(code->getName(operand));
                stack.pop();
                lines.push_back(new ASTStore(value, name));
            }
            break;
        //case Py2k::DELETE_NAME:
        //case Py2k::UNPACK_SEQUENCE:
        //case Py2k::FOR_ITER:
        //case Py2k::STORE_ATTR:
        //case Py2k::DELETE_ATTR:
        //case Py2k::STORE_GLOBAL:
        //case Py2k::DELETE_GLOBAL:
        //case Py2k::DUP_TOPX:
        case (PY_1000 | Py1k::LOAD_CONST):
        case (PY_2000 | Py2k::LOAD_CONST):
        case (PY_3000 | Py3k::LOAD_CONST):
            stack.push(new ASTObject(code->getConst(operand)));
            break;
        //case Py2k::LOAD_NAME:
        //case Py2k::BUILD_TUPLE:
        //case Py2k::BUILD_LIST:
        //case Py2k::BUILD_MAP:
        //case Py2k::LOAD_ATTR:
        //case Py2k::COMPARE_OP:
        //case Py2k::IMPORT_NAME:
        //case Py2k::IMPORT_FROM:
        //case Py2k::JUMP_FORWARD:
        //case Py2k::JUMP_IF_FALSE:
        //case Py2k::JUMP_IF_TRUE:
        //case Py2k::JUMP_ABSOLUTE:
        //case Py2k::FOR_LOOP:
        //case Py2k::LOAD_GLOBAL:
        //case Py2k::CONTINUE_LOOP:
        //case Py2k::SETUP_LOOP:
        //case Py2k::SETUP_EXCEPT:
        //case Py2k::SETUP_FINALLY:
        //case Py2k::LOAD_FAST:
        //case Py2k::STORE_FAST:
        //case Py2k::DELETE_FAST:
        //case Py2k::SET_LINENO:
        //case Py2k::RAISE_VARARGS:
        //case Py2k::CALL_FUNCTION:
        //case Py2k::MAKE_FUNCTION:
        //case Py2k::BUILD_SLICE:
        //case Py2k::MAKE_CLOSURE:
        //case Py2k::LOAD_CLOSURE:
        //case Py2k::LOAD_DEREF:
        //case Py2k::STORE_DEREF:
        //case Py2k::CALL_FUNCTION_VAR:
        //case Py2k::CALL_FUNCTION_KW:
        //case Py2k::CALL_FUNCTION_VAR_KW:
        //case Py2k::EXTENDED_ARG:
        default:
            if (mod->majorVer() == 1)
                fprintf(stderr, "Unsupported opcode: %s\n", Py1k::OpcodeNames[opcode & 0xFF]);
            else if (mod->majorVer() == 2)
                fprintf(stderr, "Unsupported opcode: %s\n", Py2k::OpcodeNames[opcode & 0xFF]);
            else if (mod->majorVer() == 3)
                fprintf(stderr, "Unsupported opcode: %s\n", Py3k::OpcodeNames[opcode & 0xFF]);
            return new ASTNodeList(lines);
        }
    }

    return new ASTNodeList(lines);
}

static void start_indent(int indent)
{
    for (int i=0; i<indent; i++)
        printf("    ");
}

static void print_src(PycRef<ASTNode> node, PycModule* mod, int indent = 0)
{
    switch (node->type()) {
    case ASTNode::NODE_LIST:
        {
            ASTNodeList::list_t lines = node.cast<ASTNodeList>()->nodes();
            for (ASTNodeList::list_t::iterator ln = lines.begin(); ln != lines.end(); ++ln)
                print_src(*ln, mod, indent);
        }
        break;
    case ASTNode::NODE_STORE:
        {
            PycRef<ASTNode> src = node.cast<ASTStore>()->src();
            PycRef<ASTNode> dest = node.cast<ASTStore>()->dest();
            start_indent(indent);
            print_src(dest, mod);
            printf(" = ");
            print_src(src, mod);
            printf("\n");
        }
        break;
    case ASTNode::NODE_OBJECT:
        print_const(node.cast<ASTObject>()->object(), mod);
        break;
    case ASTNode::NODE_NAME:
        {
            ASTName::name_t name = node.cast<ASTName>()->name();
            ASTName::name_t::iterator n = name.begin();
            printf("%s", (*n)->value());
            while (++n != name.end())
                printf(".%s", (*n)->value());
        }
        break;
    default:
        printf("Unsupported Node type: %d\n", node->type());
    }
}

void ASTree::printSource(PycModule* mod) const
{
    print_src(m_root, mod, 0);
}
