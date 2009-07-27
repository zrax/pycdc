#ifndef _PYC_ASTNODE_H
#define _PYC_ASTNODE_H

#include "module.h"
#include <list>

/* Similar interface to PycObject, so PycRef can work on it... *
 * However, this does *NOT* mean the two are interchangeable!  */
class ASTNode {
public:
    enum Type {
        NODE_INVALID, NODE_LIST, NODE_OBJECT, NODE_UNARY, NODE_BINARY,
        NODE_COMPARE, NODE_STORE, NODE_RETURN, NODE_NAME, NODE_DELETE,
        NODE_FUNCTION, NODE_CLASS, NODE_CALL, NODE_PASS
    };

    ASTNode(int type = NODE_INVALID) : m_refs(0), m_type(type) { }
    virtual ~ASTNode() { }

    int type() const { return (this) ? m_type : NODE_INVALID; }

private:
    int m_refs;
    int m_type;

public:
    void addRef() { if (this) ++m_refs; }
    void delRef() { if (this && --m_refs == 0) delete this; }
};

/* A NULL node for comparison */
extern PycRef<ASTNode> Node_NULL;


class ASTNodeList : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode> > list_t;

    ASTNodeList(list_t nodes)
        : ASTNode(NODE_LIST), m_nodes(nodes) { }

    const list_t& nodes() const { return m_nodes; }
    void removeFirst();
    void removeLast();
    void append(PycRef<ASTNode> node) { m_nodes.push_back(node); }

private:
    list_t m_nodes;
};


class ASTObject : public ASTNode {
public:
    ASTObject(PycRef<PycObject> obj)
        : ASTNode(NODE_OBJECT), m_obj(obj) { }

    PycRef<PycObject> object() const { return m_obj; }

private:
    PycRef<PycObject> m_obj;
};


class ASTUnary : public ASTNode {
public:
    ASTUnary(PycRef<ASTNode> operand)
        : ASTNode(NODE_UNARY), m_operand(operand) { }

    PycRef<ASTNode> operand() const { return m_operand; }

private:
    PycRef<ASTNode> m_operand;
};


class ASTBinary : public ASTNode {
public:
    enum BinOp {
        BIN_POWER, BIN_MULTIPLY, BIN_DIVIDE, BIN_MODULO, BIN_ADD,
        BIN_SUBTRACT, BIN_LSHIFT, BIN_RSHIFT, BIN_AND, BIN_XOR, BIN_OR
    };

    ASTBinary(PycRef<ASTNode> left, PycRef<ASTNode> right, int op,
              int type = NODE_BINARY)
        : ASTNode(type), m_op(op), m_left(left), m_right(right) { }

    PycRef<ASTNode> left() const { return m_left; }
    PycRef<ASTNode> right() const { return m_right; }
    int op() const { return m_op; }
    virtual const char* op_str() const;

protected:
    int m_op;

private:
    PycRef<ASTNode> m_left;
    PycRef<ASTNode> m_right;
};


class ASTCompare : public ASTBinary {
public:
    enum CompareOp {
        CMP_LESS, CMP_LESS_EQUAL, CMP_EQUAL, CMP_NOT_EQUAL, CMP_GREATER,
        CMP_GREATER_EQUAL, CMP_IN, CMP_NOT_IN, CMP_IS, CMP_IS_NOT,
        CMP_EXCEPTION, CMP_BAD
    };

    ASTCompare(PycRef<ASTNode> left, PycRef<ASTNode> right, int op)
        : ASTBinary(left, right, op, NODE_COMPARE) { }

    const char* op_str() const;
};


class ASTStore : public ASTNode {
public:
    ASTStore(PycRef<ASTNode> src, PycRef<ASTNode> dest)
        : ASTNode(NODE_STORE), m_src(src), m_dest(dest) { }

    PycRef<ASTNode> src() const { return m_src; }
    PycRef<ASTNode> dest() const { return m_dest; }

private:
    PycRef<ASTNode> m_src;
    PycRef<ASTNode> m_dest;
};


class ASTReturn : public ASTNode {
public:
    ASTReturn(PycRef<ASTNode> value)
        : ASTNode(NODE_RETURN), m_value(value) { }

    PycRef<ASTNode> value() const { return m_value; }

private:
    PycRef<ASTNode> m_value;
};


class ASTName : public ASTNode {
public:
    typedef std::list<PycRef<PycString> > name_t;

    ASTName(PycRef<PycString> name)
        : ASTNode(NODE_NAME) { m_name.push_back(name); }

    const name_t& name() const { return m_name; }
    void add(PycRef<PycString> name) { m_name.push_back(name); }

private:
    name_t m_name;
};


class ASTDelete : public ASTNode {
public:
    ASTDelete(PycRef<ASTNode> value)
        : ASTNode(NODE_DELETE), m_value(value) { }

    PycRef<ASTNode> value() const { return m_value; }

private:
    PycRef<ASTNode> m_value;
};


class ASTFunction : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode> > defarg_t;

    ASTFunction(PycRef<ASTNode> code, defarg_t defArgs)
        : ASTNode(NODE_FUNCTION), m_code(code) { }

    PycRef<ASTNode> code() const { return m_code; }
    const defarg_t& defargs() const { return m_defargs; }

private:
    PycRef<ASTNode> m_code;
    defarg_t m_defargs;
};


class ASTClass : public ASTNode {
public:
    ASTClass(PycRef<ASTNode> code)
        : ASTNode(NODE_CLASS), m_code(code) { }

    PycRef<ASTNode> code() const { return m_code; }

private:
    PycRef<ASTNode> m_code;
};


class ASTCall : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode> > pparam_t;
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode> > > kwparam_t;

    ASTCall(PycRef<ASTNode> func, pparam_t pparams, kwparam_t kwparams)
        : ASTNode(NODE_CALL), m_func(func), m_pparams(pparams), m_kwparams(kwparams) { }

    PycRef<ASTNode> func() const { return m_func; }
    const pparam_t& pparams() const { return m_pparams; }
    const kwparam_t& kwparams() const { return m_kwparams; }

private:
    PycRef<ASTNode> m_func;
    pparam_t m_pparams;
    kwparam_t m_kwparams;
};

#endif
