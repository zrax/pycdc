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
        case (PY_1000 | Py1k::BINARY_ADD):
        case (PY_2000 | Py2k::BINARY_ADD):
        case (PY_3000 | Py3k::BINARY_ADD):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_ADD));
            }
            break;
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
        case (PY_1000 | Py1k::BINARY_DIVIDE):
        case (PY_2000 | Py2k::BINARY_DIVIDE):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case (PY_2000 | Py2k::BINARY_FLOOR_DIVIDE):
        case (PY_3000 | Py3k::BINARY_FLOOR_DIVIDE):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_FLOOR));
            }
            break;
        case (PY_1000 | Py1k::BINARY_LSHIFT):
        case (PY_2000 | Py2k::BINARY_LSHIFT):
        case (PY_3000 | Py3k::BINARY_LSHIFT):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_LSHIFT));
            }
            break;
        case (PY_1000 | Py1k::BINARY_MODULO):
        case (PY_2000 | Py2k::BINARY_MODULO):
        case (PY_3000 | Py3k::BINARY_MODULO):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_MODULO));
            }
            break;
        case (PY_1000 | Py1k::BINARY_MULTIPLY):
        case (PY_2000 | Py2k::BINARY_MULTIPLY):
        case (PY_3000 | Py3k::BINARY_MULTIPLY):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case (PY_1000 | Py1k::BINARY_OR):
        case (PY_2000 | Py2k::BINARY_OR):
        case (PY_3000 | Py3k::BINARY_OR):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_OR));
            }
            break;
        case (PY_1000 | Py1k::BINARY_POWER):
        case (PY_2000 | Py2k::BINARY_POWER):
        case (PY_3000 | Py3k::BINARY_POWER):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_POWER));
            }
            break;
        case (PY_1000 | Py1k::BINARY_RSHIFT):
        case (PY_2000 | Py2k::BINARY_RSHIFT):
        case (PY_3000 | Py3k::BINARY_RSHIFT):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_RSHIFT));
            }
            break;
        case (PY_1000 | Py1k::BINARY_SUBSCR):
        case (PY_2000 | Py2k::BINARY_SUBSCR):
        case (PY_3000 | Py3k::BINARY_SUBSCR):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_SUBSCR));
            }
            break;
        case (PY_1000 | Py1k::BINARY_SUBTRACT):
        case (PY_2000 | Py2k::BINARY_SUBTRACT):
        case (PY_3000 | Py3k::BINARY_SUBTRACT):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case (PY_2000 | Py2k::BINARY_TRUE_DIVIDE):
        case (PY_3000 | Py3k::BINARY_TRUE_DIVIDE):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_DIVIDE));
            }
            break;
        case (PY_1000 | Py1k::BINARY_XOR):
        case (PY_2000 | Py2k::BINARY_XOR):
        case (PY_3000 | Py3k::BINARY_XOR):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> left = stack.top();
                stack.pop();
                stack.push(new ASTBinary(left, right, ASTBinary::BIN_XOR));
            }
            break;
        case (PY_1000 | Py1k::BUILD_CLASS):
        case (PY_2000 | Py2k::BUILD_CLASS):
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
        case (PY_1000 | Py1k::BUILD_FUNCTION):
            {
                PycRef<ASTNode> code = stack.top();
                stack.pop();
                stack.push(new ASTFunction(code, ASTFunction::defarg_t()));
            }
            break;
        case (PY_1000 | Py1k::BUILD_LIST):
        case (PY_2000 | Py2k::BUILD_LIST):
        case (PY_3000 | Py3k::BUILD_LIST):
            {
                ASTList::value_t values;
                for (int i=0; i<operand; i++) {
                    values.push_front(stack.top());
                    stack.pop();
                }
                stack.push(new ASTList(values));
            }
            break;
        case (PY_1000 | Py1k::BUILD_MAP):
        case (PY_2000 | Py2k::BUILD_MAP):
        case (PY_3000 | Py3k::BUILD_MAP):
            stack.push(new ASTMap());
            break;
        case (PY_1000 | Py1k::BUILD_TUPLE):
        case (PY_2000 | Py2k::BUILD_TUPLE):
        case (PY_3000 | Py3k::BUILD_TUPLE):
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
        case (PY_1000 | Py1k::DUP_TOP):
        case (PY_2000 | Py2k::DUP_TOP):
        case (PY_3000 | Py3k::DUP_TOP):
            stack.push(stack.top());
            break;
        case (PY_1000 | Py1k::IMPORT_NAME):
            stack.push(new ASTImport(new ASTName(code->getName(operand)), Node_NULL));
            break;
        case (PY_2000 | Py2k::IMPORT_NAME):
            {
                PycRef<ASTNode> fromlist = stack.top();
                stack.pop();
                if (mod->minorVer() >= 5)
                    stack.pop();    // Level -- we don't care
                stack.push(new ASTImport(new ASTName(code->getName(operand)), fromlist));
            }
            break;
        case (PY_3000 | Py3k::IMPORT_NAME):
            {
                PycRef<ASTNode> fromlist = stack.top();
                stack.pop();
                stack.pop();    // Level -- we don't care
                stack.push(new ASTImport(new ASTName(code->getName(operand)), fromlist));
            }
            break;
        case (PY_2000 | Py2k::IMPORT_STAR):
        case (PY_3000 | Py3k::IMPORT_STAR):
            {
                PycRef<ASTNode> import = stack.top();
                stack.pop();
                lines.push_back(new ASTStore(import, Node_NULL));
            }
            break;
        case (PY_2000 | Py2k::INPLACE_ADD):
        case (PY_3000 | Py3k::INPLACE_ADD):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                /* This is a problem, so fake it with a = a + b syntax */
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_AND));
            }
            break;
        case (PY_2000 | Py2k::INPLACE_SUBTRACT):
        case (PY_3000 | Py3k::INPLACE_SUBTRACT):
            {
                PycRef<ASTNode> right = stack.top();
                stack.pop();
                PycRef<ASTNode> src = stack.top();
                stack.pop();
                /* This is a problem, so fake it with a = a - b syntax */
                stack.push(new ASTBinary(src, right, ASTBinary::BIN_SUBTRACT));
            }
            break;
        case (PY_1000 | Py1k::LOAD_ATTR):
        case (PY_2000 | Py2k::LOAD_ATTR):
        case (PY_3000 | Py3k::LOAD_ATTR):
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                stack.push(new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR));
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
            stack.push(new ASTName(code->getName(operand)));
            break;
        case (PY_1000 | Py1k::LOAD_LOCALS):
        case (PY_2000 | Py2k::LOAD_LOCALS):
            stack.push(new ASTNode(ASTNode::NODE_LOCALS));
            break;
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
        case (PY_1000 | Py1k::POP_TOP):
        case (PY_2000 | Py2k::POP_TOP):
        case (PY_3000 | Py3k::POP_TOP):
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                if (value->type() == ASTNode::NODE_CALL || value->type() == ASTNode::NODE_JUMP)
                    lines.push_back(value);
            }
            break;
        case (PY_1000 | Py1k::PRINT_ITEM):
        case (PY_2000 | Py2k::PRINT_ITEM):
            lines.push_back(new ASTPrint(stack.top()));
            stack.pop();
            break;
        case (PY_1000 | Py1k::PRINT_NEWLINE):
        case (PY_2000 | Py2k::PRINT_NEWLINE):
            lines.push_back(new ASTPrint(Node_NULL));
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
        case (PY_1000 | Py1k::ROT_THREE):
        case (PY_2000 | Py2k::ROT_THREE):
        case (PY_3000 | Py3k::ROT_THREE):
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
        case (PY_1000 | Py1k::SET_LINENO):
        case (PY_2000 | Py2k::SET_LINENO):
            // Ignore
            break;
        case (PY_1000 | Py1k::STORE_ATTR):
        case (PY_2000 | Py2k::STORE_ATTR):
        case (PY_3000 | Py3k::STORE_ATTR):
            {
                PycRef<ASTNode> name = stack.top();
                stack.pop();
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> attr = new ASTBinary(name, new ASTName(code->getName(operand)), ASTBinary::BIN_ATTR);
                lines.push_back(new ASTStore(value, attr));
            }
            break;
        case (PY_1000 | Py1k::STORE_FAST):
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> name;
                if (mod->minorVer() < 3)
                    name = new ASTName(code->getName(operand));
                else
                    name = new ASTName(code->getVarName(operand));
                lines.push_back(new ASTStore(value, name));
            }
            break;
        case (PY_2000 | Py2k::STORE_FAST):
        case (PY_3000 | Py3k::STORE_FAST):
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> name = new ASTName(code->getVarName(operand));
                lines.push_back(new ASTStore(value, name));
            }
            break;
        case (PY_1000 | Py1k::STORE_GLOBAL):
        case (PY_2000 | Py2k::STORE_GLOBAL):
        case (PY_3000 | Py3k::STORE_GLOBAL):
            {
                PycRef<ASTNode> value = stack.top();
                stack.pop();
                PycRef<ASTNode> name = new ASTName(code->getName(operand));
                lines.push_back(new ASTStore(value, name));
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
        case (PY_1000 | Py1k::STORE_SUBSCR):
        case (PY_2000 | Py2k::STORE_SUBSCR):
        case (PY_3000 | Py3k::STORE_SUBSCR):
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
        case (PY_1000 | Py1k::UNARY_INVERT):
        case (PY_2000 | Py2k::UNARY_INVERT):
        case (PY_3000 | Py3k::UNARY_INVERT):
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_INVERT));
            }
            break;
        case (PY_1000 | Py1k::UNARY_NEGATIVE):
        case (PY_2000 | Py2k::UNARY_NEGATIVE):
        case (PY_3000 | Py3k::UNARY_NEGATIVE):
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NEGATIVE));
            }
            break;
        case (PY_1000 | Py1k::UNARY_NOT):
        case (PY_2000 | Py2k::UNARY_NOT):
        case (PY_3000 | Py3k::UNARY_NOT):
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_NOT));
            }
            break;
        case (PY_1000 | Py1k::UNARY_POSITIVE):
        case (PY_2000 | Py2k::UNARY_POSITIVE):
        case (PY_3000 | Py3k::UNARY_POSITIVE):
            {
                PycRef<ASTNode> arg = stack.top();
                stack.pop();
                stack.push(new ASTUnary(arg, ASTUnary::UN_POSITIVE));
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
            if (bin->op() == ASTBinary::BIN_SUBSCR)
                printf("]");
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
                    PycRef<PycTuple> fromlist = import->fromlist().cast<ASTObject>()->object().cast<PycTuple>();
                    if (fromlist != Pyc_None && fromlist->size() != 0) {
                        printf("from ");
                        print_src(import->name(), mod, indent);
                        printf(" import ");
                        bool first = true;
                        PycTuple::value_t::const_iterator ii = fromlist->values().begin();
                        for (; ii != fromlist->values().end(); ++ii) {
                            if (!first) printf(", ");
                            printf("%s", ii->cast<PycString>()->value());
                            first = false;
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
                print_src(dest, mod, indent);
                printf(" = ");
                print_src(src, mod, indent);
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
