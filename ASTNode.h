#ifndef _PYC_ASTNODE_H
#define _PYC_ASTNODE_H

#include "module.h"
#include <list>

/* Similar interface to PycObject, so PycRef can work on it... *
 * However, this does *NOT* mean the two are interchangeable!  */
class ASTNode {
public:
    enum Type {
        NODE_INVALID, NODE_NODELIST, NODE_OBJECT, NODE_UNARY, NODE_BINARY,
        NODE_COMPARE, NODE_STORE, NODE_RETURN, NODE_NAME, NODE_DELETE,
        NODE_FUNCTION, NODE_CLASS, NODE_CALL, NODE_IMPORT, NODE_TUPLE,
        NODE_LIST, NODE_MAP, NODE_SUBSCR, NODE_PRINT,

        // Empty nodes
        NODE_PASS, NODE_LOCALS
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
        : ASTNode(NODE_NODELIST), m_nodes(nodes) { }

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
    enum UnOp {
        UN_POSITIVE, UN_NEGATIVE, UN_INVERT, UN_NOT
    };

    ASTUnary(PycRef<ASTNode> operand, int op)
        : ASTNode(NODE_UNARY), m_op(op), m_operand(operand) { }

    PycRef<ASTNode> operand() const { return m_operand; }
    int op() const { return m_op; }
    virtual const char* op_str() const;

protected:
    int m_op;

private:
    PycRef<ASTNode> m_operand;
};


class ASTBinary : public ASTNode {
public:
    enum BinOp {
        BIN_ATTR, BIN_POWER, BIN_MULTIPLY, BIN_DIVIDE, BIN_FLOOR, BIN_MODULO,
        BIN_ADD, BIN_SUBTRACT, BIN_LSHIFT, BIN_RSHIFT, BIN_AND, BIN_XOR,
        BIN_OR, BIN_LOG_AND, BIN_LOG_OR
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
    ASTName(PycRef<PycString> name)
        : ASTNode(NODE_NAME), m_name(name) { }

    PycRef<PycString> name() const { return m_name; }

private:
    PycRef<PycString> m_name;
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
        : ASTNode(NODE_FUNCTION), m_code(code), m_defargs(defArgs) { }

    PycRef<ASTNode> code() const { return m_code; }
    const defarg_t& defargs() const { return m_defargs; }

private:
    PycRef<ASTNode> m_code;
    defarg_t m_defargs;
};


class ASTClass : public ASTNode {
public:
    ASTClass(PycRef<ASTNode> code, PycRef<ASTNode> bases, PycRef<ASTNode> name)
        : ASTNode(NODE_CLASS), m_code(code), m_bases(bases), m_name(name) { }

    PycRef<ASTNode> code() const { return m_code; }
    PycRef<ASTNode> bases() const { return m_bases; }
    PycRef<ASTNode> name() const { return m_name; }

private:
    PycRef<ASTNode> m_code;
    PycRef<ASTNode> m_bases;
    PycRef<ASTNode> m_name;
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


class ASTImport : public ASTNode {
public:
    ASTImport(PycRef<ASTNode> name, PycRef<ASTNode> fromlist)
        : ASTNode(NODE_IMPORT), m_name(name), m_fromlist(fromlist) { }

    PycRef<ASTNode> name() const { return m_name; }
    PycRef<ASTNode> fromlist() const { return m_fromlist; }

private:
    PycRef<ASTNode> m_name;
    PycRef<ASTNode> m_fromlist;
};


class ASTTuple : public ASTNode {
public:
    typedef std::vector<PycRef<ASTNode> > value_t;

    ASTTuple(value_t values)
        : ASTNode(NODE_TUPLE), m_values(values) { }

    const value_t& values() const { return m_values; }

private:
    value_t m_values;
};


class ASTList : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode> > value_t;

    ASTList(value_t values)
        : ASTNode(NODE_LIST), m_values(values) { }

    const value_t& values() const { return m_values; }

private:
    value_t m_values;
};


class ASTMap : public ASTNode {
public:
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode> > > map_t;

    ASTMap() : ASTNode(NODE_MAP) { }

    void add(PycRef<ASTNode> key, PycRef<ASTNode> value)
    { m_values.push_back(std::make_pair(key, value)); }

    const map_t& values() const { return m_values; }

private:
    map_t m_values;
};


class ASTSubscr : public ASTNode {
public:
    ASTSubscr(PycRef<ASTNode> name, PycRef<ASTNode> key)
        : ASTNode(NODE_SUBSCR), m_name(name), m_key(key) { }

    PycRef<ASTNode> name() const { return m_name; }
    PycRef<ASTNode> key() const { return m_key; }

private:
    PycRef<ASTNode> m_name;
    PycRef<ASTNode> m_key;
};


class ASTPrint : public ASTNode {
public:
    ASTPrint(PycRef<ASTNode> value)
        : ASTNode(NODE_PRINT), m_value(value) { }

    PycRef<ASTNode> value() const { return m_value; }

private:
    PycRef<ASTNode> m_value;
};

#endif
