#include "ASTree.h"
#include "FastStack.h"
#include "bytecode.h"

// These are used to avoid writing code 3 times for each of
// the different python generations
#define PY_1000 0x1000
#define PY_2000 0x2000
#define PY_3000 0x3000

/* Use this to determine if an error occurred (and therefore, if we should
 * avoid cleaning the output tree) */
static bool cleanBuild;

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
        case (PY_1000 | Py1k::BINARY_AND):
        case (PY_2000 | Py2k::BINARY_AND):
        case (PY_3000 | Py3k::BINARY_AND):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_AND));
            }
            break;
        case (PY_1000 | Py1k::BUILD_FUNCTION):
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                stack.push(new ASTFunction(code, ASTFunction::defarg_t()));
            }
            break;
        case (PY_1000 | Py1k::CALL_FUNCTION):
        case (PY_2000 | Py2k::CALL_FUNCTION):
        case (PY_3000 | Py3k::CALL_FUNCTION):
            {
                int kwparams = (operand & 0xFF00) >> 8;
                int pparams = (operand & 0xFF);
                ASTCall::kwparam_t kwparamList;
                ASTCall::pparam_t pparamList;
                for (int i=0; i<kwparams; i++) {
                    PycRef<ASTNode> val = stack.top();
                    stack.pop();
                    PycRef<ASTNode> key = stack.top();
                    stack.pop();
                    kwparamList.push_front(std::make_pair(key, val));
                }
                for (int i=0; i<pparams; i++) {
                    pparamList.push_front(stack.top());
                    stack.pop();
                }
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                stack.push(new ASTCall(func, pparamList, kwparamList));
            }
            break;
        case (PY_1000 | Py1k::COMPARE_OP):
        case (PY_2000 | Py2k::COMPARE_OP):
        case (PY_3000 | Py3k::COMPARE_OP):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTCompare(left, right, operand));
            }
            break;
        case (PY_1000 | Py1k::LOAD_CONST):
        case (PY_2000 | Py2k::LOAD_CONST):
        case (PY_3000 | Py3k::LOAD_CONST):
            stack.push(new ASTObject(code->getConst(operand)));
            break;
        case (PY_1000 | Py1k::LOAD_FAST):
            if (mod->minorVer()  < 3)
                stack.push(new ASTName(code->getName(operand)));
            else
                stack.push(new ASTName(code->getVarName(operand)));
            break;
        case (PY_2000 | Py2k::LOAD_FAST):
        case (PY_3000 | Py3k::LOAD_FAST):
            stack.push(new ASTName(code->getVarName(operand)));
            break;
        case (PY_1000 | Py1k::LOAD_GLOBAL):
        case (PY_2000 | Py2k::LOAD_GLOBAL):
        case (PY_3000 | Py3k::LOAD_GLOBAL):
        case (PY_1000 | Py1k::LOAD_NAME):
        case (PY_2000 | Py2k::LOAD_NAME):
        case (PY_3000 | Py3k::LOAD_NAME):
            stack.push(new ASTName(code->getName(operand)));
            break;
        case (PY_1000 | Py1k::MAKE_FUNCTION):
        case (PY_2000 | Py2k::MAKE_FUNCTION):
        case (PY_3000 | Py3k::MAKE_FUNCTION):
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                ASTFunction::defarg_t defArgs;
                for (int i=0; i<operand; i++) {
                    defArgs.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTFunction(code, defArgs));
            }
            break;
        case (PY_1000 | Py1k::RETURN_VALUE):
        case (PY_2000 | Py2k::RETURN_VALUE):
        case (PY_3000 | Py3k::RETURN_VALUE):
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                lines.push_back(new ASTReturn(value));
            }
            break;
        case (PY_1000 | Py1k::STORE_NAME):
        case (PY_2000 | Py2k::STORE_NAME):
        case (PY_3000 | Py3k::STORE_NAME):
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> name = new ASTName(code->getName(operand));
                lines.push_back(new ASTStore(value, name));
            }
            break;
        default:
            if (mod->majorVer() == 1)
                fprintf(stderr, "Unsupported opcode: %s\n", Py1k::OpcodeNames[opcode & 0xFF]);
            else if (mod->majorVer() == 2)
                fprintf(stderr, "Unsupported opcode: %s\n", Py2k::OpcodeNames[opcode & 0xFF]);
            else if (mod->majorVer() == 3)
                fprintf(stderr, "Unsupported opcode: %s\n", Py3k::OpcodeNames[opcode & 0xFF]);
            cleanBuild = false;
            return new ASTNodeList(lines);
        }
    }

    cleanBuild = true;
    return new ASTNodeList(lines);
}

static void start_indent(int indent)
{
    for (int i=0; i<indent; i++)
        printf("    ");
}

void print_src(PycRef<ASTNode> node, PycModule* mod, int indent)
{
    switch (node->type()) {
    case ASTNode::NODE_BINARY:
    case ASTNode::NODE_COMPARE:
        {
            PycRef<ASTBinary> bin = node.cast<ASTBinary>();
            printf("(");
            print_src(bin->left(), mod);
            printf(" %s ", bin->op_str());
            print_src(bin->right(), mod);
            printf(")");
        }
        break;
    case ASTNode::NODE_CALL:
        {
            PycRef<ASTCall> call = node.cast<ASTCall>();
            start_indent(indent);
            print_src(call->func(), mod);
            printf("(");
            bool first = true;
            for (ASTCall::pparam_t::const_iterator p = call->pparams().begin(); p != call->pparams().end(); ++p) {
                if (!first) printf(", ");
                print_src(*p, mod);
                first = false;
            }
            for (ASTCall::kwparam_t::const_iterator p = call->kwparams().begin(); p != call->kwparams().end(); ++p) {
                if (!first) printf(", ");
                print_src(p->first, mod);
                printf("=");
                print_src(p->second, mod);
                first = false;
            }
            printf(")");
        }
        break;
    case ASTNode::NODE_LIST:
        {
            ASTNodeList::list_t lines = node.cast<ASTNodeList>()->nodes();
            for (ASTNodeList::list_t::const_iterator ln = lines.begin(); ln != lines.end(); ++ln) {
                print_src(*ln, mod, indent);
                printf("\n");
            }
        }
        break;
    case ASTNode::NODE_STORE:
        {
            PycRef<ASTNode> src = node.cast<ASTStore>()->src();
            PycRef<ASTNode> dest = node.cast<ASTStore>()->dest();
            if (src->type() == ASTNode::NODE_FUNCTION) {
                printf("\n");
                start_indent(indent);
                printf("def ");
                print_src(dest, mod);
                printf("(");
                //TODO: Keyword and Default args
                PycRef<ASTNode> code = src.cast<ASTFunction>()->code();
                PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
                for (int i=0; i<code_src->argCount(); i++) {
                    if (i > 0) printf(", ");
                    printf("%s", code_src->getVarName(i)->value());
                }
                printf("):\n");
                print_src(code, mod, indent + 1);
            } else {
                start_indent(indent);
                print_src(dest, mod);
                printf(" = ");
                print_src(src, mod);
            }
        }
        break;
    case ASTNode::NODE_NAME:
        {
            ASTName::name_t name = node.cast<ASTName>()->name();
            ASTName::name_t::const_iterator n = name.begin();
            printf("%s", (*n)->value());
            while (++n != name.end())
                printf(".%s", (*n)->value());
        }
        break;
    case ASTNode::NODE_OBJECT:
        {
            PycRef<PycObject> obj = node.cast<ASTObject>()->object();
            if (obj->type() == PycObject::TYPE_CODE)
                decompyle(obj.cast<PycCode>(), mod, indent);
            else
                print_const(obj, mod);
        }
        break;
    case ASTNode::NODE_PASS:
        start_indent(indent);
        printf("pass");
        break;
    case ASTNode::NODE_RETURN:
        start_indent(indent);
        printf("return ");
        print_src(node.cast<ASTReturn>()->value(), mod);
        break;
    default:
        fprintf(stderr, "Unsupported Node type: %d\n", node->type());
    }
}

void decompyle(PycRef<PycCode> code, PycModule* mod, int indent)
{
    PycRef<ASTNode> source = BuildFromCode(code, mod);

    if (cleanBuild) {
        // The Python compiler adds some stuff that we don't really care
        // about, and would add extra code for re-compilation anyway.
        // We strip these lines out here, and then add a "pass" statement
        // if the cleaned up code is empty
        PycRef<ASTNodeList> clean = source.cast<ASTNodeList>();
        if (clean->nodes().front()->type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->src()->type() == ASTNode::NODE_NAME &&
                store->dest()->type() == ASTNode::NODE_NAME) {
                PycRef<ASTName> src = store->src().cast<ASTName>();
                PycRef<ASTName> dest = store->dest().cast<ASTName>();
                if (src->name().size() == 1 && dest->name().size() == 1 &&
                    src->name().front()->isEqual("__name__") &&
                    dest->name().front()->isEqual("__module__")) {
                    // __module__ = __name__
                    clean->removeFirst();
                }
            }
        }
        clean->removeLast();    // Always an extraneous return statement

        if (clean->nodes().size() == 0)
            clean->append(new ASTNode(ASTNode::NODE_PASS));
    }

    print_src(source, mod, indent);
}
