#include "ASTree.h"
#include "FastStack.h"
#include "bytecode.h"

/* Use this to determine if an error occurred (and therefore, if we should
 * avoid cleaning the output tree) */
static bool cleanBuild;

/* Keep track of whether we're in a print statement, so we can make
 * chained prints (print x, y, z) prettier */
static bool inPrint;

PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod)
{
    PycBuffer source(code->code()->value(), code->code()->length());
    ASTNodeList::list_t lines;

    FastStack stack((mod->majorVer() == 1) ? 20 : code->stackSize());
    stackhist_t stack_hist;

    int opcode, operand;
    int pos = 0;

    while (!source.atEof()) {
        bc_next(source, mod, opcode, operand, pos);

        switch (opcode) {
        case Pyc::BINARY_ADD:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_ADD));
            }
            break;
        case Pyc::BINARY_AND:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_AND));
            }
            break;
        case Pyc::BINARY_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case Pyc::BINARY_FLOOR_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_FLOOR));
            }
            break;
        case Pyc::BINARY_LSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_LSHIFT));
            }
            break;
        case Pyc::BINARY_MODULO:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_MODULO));
            }
            break;
        case Pyc::BINARY_MULTIPLY:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case Pyc::BINARY_OR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_OR));
            }
            break;
        case Pyc::BINARY_POWER:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_POWER));
            }
            break;
        case Pyc::BINARY_RSHIFT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_RSHIFT));
            }
            break;
        case Pyc::BINARY_SUBSCR:
            {
                PycRef<ASTNode> subscr = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                stack.push(new ASTSubscr(src, subscr));
            }
            break;
        case Pyc::BINARY_SUBTRACT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case Pyc::BINARY_TRUE_DIVIDE:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case Pyc::BINARY_XOR:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_XOR));
            }
            break;
        case Pyc::BUILD_CLASS:
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                PycRef<ASTNode> bases = stack.top();
                stack.pop();
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTClass(code, bases, name));
            }
            break;
        case Pyc::BUILD_FUNCTION:
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                stack.push(new ASTFunction(code, ASTFunction::defarg_t()));
            }
            break;
        case Pyc::BUILD_LIST_A:
            {
                ASTList::value_t values;
                for (int i=0; i<operand; i++) {
                    values.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTList(values));
            }
            break;
        case Pyc::BUILD_MAP_A:
            stack.push(new ASTMap());
            break;
        case Pyc::BUILD_TUPLE_A:
            {
                ASTTuple::value_t values;
                values.resize(operand);
                for (int i=0; i<operand; i++) {
                    values[operand-i-1] = stack.top();
                    stack.pop();
                }
                stack.push(new ASTTuple(values));
            }
            break;
        case Pyc::CALL_FUNCTION_A:
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
        case Pyc::COMPARE_OP_A:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTCompare(left, right, operand));
            }
            break;
        case Pyc::DUP_TOP:
            stack.push(stack.top());
            break;
        case Pyc::IMPORT_NAME_A:
            if (mod->majorVer() == 1) {
                stack.push(new ASTImport(new ASTName(code->getName(operand)), Node_NULL));
            } else {
                PycRef<ASTNode> fromlist = stack.top();
                stack.pop();
                if (mod->majorVer() > 2 || mod->minorVer() >= 5)
                    stack.pop();    // Level -- we don't care
                stack.push(new ASTImport(new ASTName(code->getName(operand)), fromlist));
            }
            break;
        case Pyc::IMPORT_STAR:
            {
                PycRef<ASTNode> import = stack.top();
                stack.pop();
                lines.push_back(new ASTStore(import, Node_NULL));
            }
            break;
        case Pyc::INPLACE_ADD:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                /* This is a problem, so fake it with a = a + b syntax */
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_AND));
            }
            break;
        case Pyc::INPLACE_SUBTRACT:
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                /* This is a problem, so fake it with a = a - b syntax */
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case Pyc::JUMP_IF_FALSE_A:
            {
                PycRef<ASTNode> cond = stack.top();
                stack.pop();
                stack.push(new ASTJump(operand, ASTJump::JMP_FALSE, cond));
            }
            break;
        case Pyc::JUMP_IF_TRUE_A:
            {
                PycRef<ASTNode> cond = stack.top();
                stack.pop();
                stack.push(new ASTJump(operand, ASTJump::JMP_TRUE, cond));
            }
            break;
        case Pyc::JUMP_FORWARD_A:
            {
                stack.push(new ASTJump(operand, ASTJump::JUMP, NULL));
            }
            break;
        case Pyc::LOAD_ATTR_A:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
            }
            break;
        case Pyc::LOAD_CONST_A:
            stack.push(new ASTObject(code->getConst(operand)));
            break;
        case Pyc::LOAD_FAST_A:
            if (mod->majorVer() == 1 && mod->minorVer()  < 3)
                stack.push(new ASTName(code->getName(operand)));
            else
                stack.push(new ASTName(code->getVarName(operand)));
            break;
        case Pyc::LOAD_GLOBAL_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::LOAD_LOCALS:
            stack.push(new ASTNode(ASTNode::NODE_LOCALS));
            break;
        case Pyc::LOAD_NAME_A:
            stack.push(new ASTName(code->getName(operand)));
            break;
        case Pyc::MAKE_FUNCTION_A:
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
        case Pyc::POP_TOP:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                if (value->type() == ASTNode::NODE_CALL || value->type() == ASTNode::NODE_JUMP)
                    lines.push_back(value);
            }
            break;
        case Pyc::PRINT_ITEM:
            lines.push_back(new ASTPrint(stack.top()));
            stack.pop();
            break;
        case Pyc::PRINT_NEWLINE:
            lines.push_back(new ASTPrint(Node_NULL));
            break;
        case Pyc::RETURN_VALUE:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                lines.push_back(new ASTReturn(value));
            }
            break;
        case Pyc::ROT_THREE:
            {
                PycRef<ASTNode> one = stack.top();
                stack.pop();
                PycRef<ASTNode> two = stack.top();
                stack.pop();
                PycRef<ASTNode> three = stack.top();
                stack.pop();
                stack.push(one);
                stack.push(three);
                stack.push(two);
            }
            break;
        case Pyc::SET_LINENO_A:
            // Ignore
            break;
        case Pyc::STORE_ATTR_A:
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);
                lines.push_back(new ASTStore(value, attr));
            }
            break;
        case Pyc::STORE_FAST_A:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> name;
                if (mod->majorVer() == 1 && mod->minorVer() < 3)
                    name = new ASTName(code->getName(operand));
                else
                    name = new ASTName(code->getVarName(operand));
                lines.push_back(new ASTStore(value, name));
            }
            break;
        case Pyc::STORE_GLOBAL_A:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> name = new ASTName(code->getName(operand));
                lines.push_back(new ASTStore(value, name));
            }
            break;
        case Pyc::STORE_NAME_A:
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> name = new ASTName(code->getName(operand));
                lines.push_back(new ASTStore(value, name));
            }
            break;
        case Pyc::STORE_SUBSCR:
            {
                PycRef<ASTNode> subscr = stack.top();
                stack.pop();
                PycRef<ASTNode> dest = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                if (dest->type() == ASTNode::NODE_MAP) {
                    dest.cast<ASTMap>()->add(subscr, src);
                } else {
                    lines.push_back(new ASTStore(src, new ASTSubscr(dest, subscr)));
                }
            }
            break;
        case Pyc::UNARY_CALL:
            {
                PycRef<ASTNode> func = stack.top();
                stack.pop();
                stack.push(new ASTCall(func, ASTCall::pparam_t(), ASTCall::kwparam_t()));
            }
            break;
        case Pyc::UNARY_INVERT:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_INVERT));
            }
            break;
        case Pyc::UNARY_NEGATIVE:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NEGATIVE));
            }
            break;
        case Pyc::UNARY_NOT:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NOT));
            }
            break;
        case Pyc::UNARY_POSITIVE:
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_POSITIVE));
            }
            break;
        default:
            if (mod->majorVer() == 1)
                fprintf(stderr, "Unsupported opcode: %s\n", Pyc::OpcodeName(opcode & 0xFF));
            else if (mod->majorVer() == 2)
                fprintf(stderr, "Unsupported opcode: %s\n", Pyc::OpcodeName(opcode & 0xFF));
            else if (mod->majorVer() == 3)
                fprintf(stderr, "Unsupported opcode: %s\n", Pyc::OpcodeName(opcode & 0xFF));
            cleanBuild = false;
            return new ASTNodeList(lines);
        }
    }

    cleanBuild = true;
    return new ASTNodeList(lines);
}

static int cmp_prec(PycRef<ASTNode> parent, PycRef<ASTNode> child)
{
    /* Determine whether the parent has higher precedence than therefore
       child, so we don't flood the source code with extraneous parens.
       Else we'd have expressions like (((a + b) + c) + d) when therefore
       equivalent, a + b + c + d would suffice. */

    if (parent->type() == ASTNode::NODE_UNARY && parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT)
        return 1;   // Always parenthesize not(x)
    if (child->type() == ASTNode::NODE_BINARY) {
        PycRef<ASTBinary> binChild = child.cast<ASTBinary>();
        if (parent->type() == ASTNode::NODE_BINARY)
            return binChild->op() - parent.cast<ASTBinary>()->op();
        else if (parent->type() == ASTNode::NODE_COMPARE)
            return (binChild->op() == ASTBinary::BIN_LOG_AND ||
                    binChild->op() == ASTBinary::BIN_LOG_OR) ? 1 : -1;
        else if (parent->type() == ASTNode::NODE_UNARY)
            return (binChild->op() == ASTBinary::BIN_POWER) ? -1 : 1;
    } else if (child->type() == ASTNode::NODE_UNARY) {
        PycRef<ASTUnary> unChild = child.cast<ASTUnary>();
        if (parent->type() == ASTNode::NODE_BINARY) {
            PycRef<ASTBinary> binParent = parent.cast<ASTBinary>();
            if (binParent->op() == ASTBinary::BIN_LOG_AND ||
                binParent->op() == ASTBinary::BIN_LOG_OR)
                return -1;
            else if (unChild->op() == ASTUnary::UN_NOT)
                return 1;
            else if (binParent->op() == ASTBinary::BIN_POWER)
                return 1;
            else
                return -1;
        } else if (parent->type() == ASTNode::NODE_COMPARE) {
            return (unChild->op() == ASTUnary::UN_NOT) ? 1 : -1;
        } else if (parent->type() == ASTNode::NODE_UNARY) {
            return unChild->op() - parent.cast<ASTUnary>()->op();
        }
    } else if (child->type() == ASTNode::NODE_COMPARE) {
        PycRef<ASTCompare> cmpChild = child.cast<ASTCompare>();
        if (parent->type() == ASTNode::NODE_BINARY)
            return (parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_AND ||
                    parent.cast<ASTBinary>()->op() == ASTBinary::BIN_LOG_OR) ? -1 : 1;
        else if (parent->type() == ASTNode::NODE_COMPARE)
            return cmpChild->op() - parent.cast<ASTCompare>()->op();
        else if (parent->type() == ASTNode::NODE_UNARY)
            return (parent.cast<ASTUnary>()->op() == ASTUnary::UN_NOT) ? -1 : 1;
    }

    /* For normal nodes, don't parenthesize anything */
    return -1;
}

static void print_ordered(PycRef<ASTNode> parent, PycRef<ASTNode> child,
                          PycModule* mod, int indent)
{
    if (child->type() == ASTNode::NODE_BINARY ||
        child->type() == ASTNode::NODE_COMPARE) {
        if (cmp_prec(parent, child) > 0) {
            printf("(");
            print_src(child, mod, indent);
            printf(")");
        } else {
            print_src(child, mod, indent);
        }
    } else if (child->type() == ASTNode::NODE_UNARY) {
        if (cmp_prec(parent, child) > 0) {
            printf("(");
            print_src(child, mod, indent);
            printf(")");
        } else {
            print_src(child, mod, indent);
        }
    } else {
        print_src(child, mod, indent);
    }
}

static void start_line(int indent)
{
    if (inPrint) return;
    for (int i=0; i<indent; i++)
        printf("    ");
}

static void end_line()
{
    if (inPrint) return;
    printf("\n");
}

int cur_indent = 0;
void print_src(PycRef<ASTNode> node, PycModule* mod, int indent)
{
    switch (node->type()) {
    case ASTNode::NODE_BINARY:
    case ASTNode::NODE_COMPARE:
        {
            PycRef<ASTBinary> bin = node.cast<ASTBinary>();
            print_ordered(node, bin->left(), mod, indent);
            printf("%s", bin->op_str());
            print_ordered(node, bin->right(), mod, indent);
        }
        break;
    case ASTNode::NODE_UNARY:
        {
            PycRef<ASTUnary> un = node.cast<ASTUnary>();
            printf("%s", un->op_str());
            print_ordered(node, un->operand(), mod, indent);
        }
        break;
    case ASTNode::NODE_CALL:
        {
            PycRef<ASTCall> call = node.cast<ASTCall>();
            print_src(call->func(), mod, indent);
            printf("(");
            bool first = true;
            for (ASTCall::pparam_t::const_iterator p = call->pparams().begin(); p != call->pparams().end(); ++p) {
                if (!first) printf(", ");
                print_src(*p, mod, indent);
                first = false;
            }
            for (ASTCall::kwparam_t::const_iterator p = call->kwparams().begin(); p != call->kwparams().end(); ++p) {
                if (!first) printf(", ");
                printf("%s = ", p->first.cast<ASTName>()->name()->value());
                print_src(p->second, mod, indent);
                first = false;
            }
            printf(")");
        }
        break;
    case ASTNode::NODE_LIST:
        {
            ASTList::value_t values = node.cast<ASTList>()->values();
            printf("[");
            bool first = true;
            for (ASTList::value_t::const_iterator b = values.begin(); b != values.end(); ++b) {
                if (first) printf("\n");
                else printf(",\n");
                start_line(cur_indent + indent + 1);
                print_src(*b, mod, indent + 1);
                first = false;
            }
            printf("]");
        }
        break;
    case ASTNode::NODE_MAP:
        {
            ASTMap::map_t values = node.cast<ASTMap>()->values();
            printf("{");
            bool first = true;
            for (ASTMap::map_t::const_iterator b = values.begin(); b != values.end(); ++b) {
                if (first) printf("\n");
                else printf(",\n");
                start_line(cur_indent + indent + 1);
                print_src(b->first, mod, indent + 1);
                printf(": ");
                print_src(b->second, mod, indent + 1);
                first = false;
            }
            printf(" }");
        }
        break;
    case ASTNode::NODE_NAME:
        printf("%s", node.cast<ASTName>()->name()->value());
        break;
    case ASTNode::NODE_NODELIST:
        {
            ASTNodeList::list_t lines = node.cast<ASTNodeList>()->nodes();
            for (ASTNodeList::list_t::const_iterator ln = lines.begin(); ln != lines.end(); ++ln) {
                start_line(cur_indent + indent);
                print_src(*ln, mod, indent);
                end_line();
            }
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
        printf("pass");
        break;
    case ASTNode::NODE_PRINT:
        if (node.cast<ASTPrint>()->value() == Node_NULL) {
            inPrint = false;
        } else if (!inPrint) {
            printf("print ");
            print_src(node.cast<ASTPrint>()->value(), mod, indent);
            inPrint = true;
        } else {
            printf(", ");
            print_src(node.cast<ASTPrint>()->value(), mod, indent);
        }
        break;
    case ASTNode::NODE_RETURN:
        printf("return ");
        print_src(node.cast<ASTReturn>()->value(), mod, indent);
        break;
    case ASTNode::NODE_STORE:
        {
            PycRef<ASTNode> src = node.cast<ASTStore>()->src();
            PycRef<ASTNode> dest = node.cast<ASTStore>()->dest();
            if (src->type() == ASTNode::NODE_FUNCTION) {
                cur_indent = 0;
                printf("\n");
                start_line(indent);
                printf("def ");
                print_src(dest, mod, indent);
                printf("(");
                PycRef<ASTNode> code = src.cast<ASTFunction>()->code();
                PycRef<PycCode> code_src = code.cast<ASTObject>()->object().cast<PycCode>();
                ASTFunction::defarg_t defargs = src.cast<ASTFunction>()->defargs();
                ASTFunction::defarg_t::iterator da = defargs.begin();
                for (int i=0; i<code_src->argCount(); i++) {
                    if (i > 0) printf(", ");
                    printf("%s", code_src->getVarName(i)->value());
                    if ((code_src->argCount() - i) <= (int)defargs.size()) {
                        printf(" = ");
                        print_src(*da++, mod, indent);
                    }
                }
                printf("):\n");
                print_src(code, mod, indent + 1);
            } else if (src->type() == ASTNode::NODE_CLASS) {
                printf("\n");
                start_line(cur_indent + indent);
                printf("class ");
                print_src(dest, mod, indent);
                PycRef<ASTTuple> bases = src.cast<ASTClass>()->bases().cast<ASTTuple>();
                if (bases->values().size() > 0) {
                    printf("(");
                    bool first = true;
                    for (ASTTuple::value_t::const_iterator b = bases->values().begin(); b != bases->values().end(); ++b) {
                        if (!first) printf(", ");
                        print_src(*b, mod, indent);
                        first = false;
                    }
                    printf("):\n");
                } else {
                    // Don't put parens if there are no base classes
                    printf(":\n");
                }
                PycRef<ASTNode> code = src.cast<ASTClass>()->code().cast<ASTCall>()
                                       ->func().cast<ASTFunction>()->code();
                print_src(code, mod, indent + 1);
            } else if (src->type() == ASTNode::NODE_IMPORT) {
                PycRef<ASTImport> import = src.cast<ASTImport>();
                if (import->fromlist() != Node_NULL) {
                    PycRef<PycObject> fromlist = import->fromlist().cast<ASTObject>()->object();
                    if (fromlist != Pyc_None) {
                        printf("from ");
                        if (import->name()->type() == ASTObject::NODE_IMPORT)
                            print_src(import->name().cast<ASTImport>()->name(), mod, indent);
                        else
                            print_src(import->name(), mod, indent);
                        printf(" import ");
                        if (fromlist->type() == ASTObject::NODE_TUPLE) {
                            bool first = true;
                            PycTuple::value_t::const_iterator ii = fromlist.cast<PycTuple>()->values().begin();
                            for (; ii != fromlist.cast<PycTuple>()->values().end(); ++ii) {
                                if (!first)
                                    printf(", ");
                                printf("%s", ii->cast<PycString>()->value());
                                first = false;
                            }
                        } else {
                            printf("%s", fromlist.cast<PycString>()->value());
                        }
                    } else {
                        printf("import ");
                        print_src(import->name(), mod, indent);
                    }
                } else {
                    printf("import ");
                    print_src(import->name(), mod, indent);
                }
            } else {
                if (dest->type() == ASTNode::NODE_NAME &&
                    dest.cast<ASTName>()->name()->isEqual("__doc__")) {
                    if (src->type() == ASTNode::NODE_OBJECT) {
                        PycRef<PycObject> obj = src.cast<ASTObject>()->object();
                        if (obj->type() == PycObject::TYPE_STRING ||
                            obj->type() == PycObject::TYPE_INTERNED ||
                            obj->type() == PycObject::TYPE_STRINGREF)
                            OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0, true);
                        else if (obj->type() == PycObject::TYPE_UNICODE)
                            OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u', true);
                    } else {
                        print_src(dest, mod, indent);
                        printf(" = ");
                        print_src(src, mod, indent);
                    }
                } else {
                    print_src(dest, mod, indent);
                    printf(" = ");
                    print_src(src, mod, indent);
                }
            }
        }
        break;
    case ASTNode::NODE_SUBSCR:
        print_src(node.cast<ASTSubscr>()->name(), mod, indent);
        printf("[");
        print_src(node.cast<ASTSubscr>()->key(), mod, indent);
        printf("]");
        break;
    case ASTNode::NODE_TUPLE:
        {
            ASTTuple::value_t values = node.cast<ASTTuple>()->values();
            printf("(");
            bool first = true;
            for (ASTTuple::value_t::const_iterator b = values.begin(); b != values.end(); ++b) {
                if (!first) printf(", ");
                print_src(*b, mod, indent);
                first = false;
            }
            if (values.size() == 1)
                printf(",)");
            else
                printf(")");
        }
        break;
    case ASTNode::NODE_JUMP:
        {
            ASTJump::Condition jtype = node.cast<ASTJump>()->jtype();
            if (jtype != ASTJump::JUMP) {
                printf("if (");
                if (jtype == ASTJump::JMP_TRUE)
                    printf("not ");
                print_src(node.cast<ASTJump>()->cond(), mod, indent);
                printf("):");
                cur_indent++;
            } else {
                if (node.cast<ASTJump>()->dest() != 1) {
                    printf("else:"); /* HAX! */
                    cur_indent++;
                }
            }
        }
        break;
    case ASTNode::NODE_POP_HACK:
        cur_indent--;
        break;
    default:
        printf("<NODE:%d>", node->type());
        fprintf(stderr, "Unsupported Node type: %d\n", node->type());
        cleanBuild = false;
        return;
    }

    cleanBuild = true;
}

void decompyle(PycRef<PycCode> code, PycModule* mod, int indent)
{
    PycRef<ASTNode> source = BuildFromCode(code, mod);

    PycRef<ASTNodeList> clean = source.cast<ASTNodeList>();
    if (cleanBuild) {
        // The Python compiler adds some stuff that we don't really care
        // about, and would add extra code for re-compilation anyway.
        // We strip these lines out here, and then add a "pass" statement
        // if the cleaned up code is empty
        if (clean->nodes().front()->type() == ASTNode::NODE_STORE) {
            PycRef<ASTStore> store = clean->nodes().front().cast<ASTStore>();
            if (store->src()->type() == ASTNode::NODE_NAME &&
                store->dest()->type() == ASTNode::NODE_NAME) {
                PycRef<ASTName> src = store->src().cast<ASTName>();
                PycRef<ASTName> dest = store->dest().cast<ASTName>();
                if (src->name()->isEqual("__name__") &&
                    dest->name()->isEqual("__module__")) {
                    // __module__ = __name__
                    clean->removeFirst();
                }
            }
        }
        clean->removeLast();    // Always an extraneous return statement
    }
    // This is outside the clean check so a source block will always
    // be compilable, even if decompylation failed.
    if (clean->nodes().size() == 0)
        clean->append(new ASTNode(ASTNode::NODE_PASS));

    inPrint = false;
    cur_indent = 0;
    bool part1clean = cleanBuild;
    print_src(source, mod, indent);

    if (!cleanBuild || !part1clean) {
        start_line(indent);
        printf("# WARNING: Decompyle incomplete\n");
        cur_indent = 0;
    }
}
