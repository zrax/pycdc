#ifndef _PYC_ASTNODE_H
#define _PYC_ASTNODE_H

#include "pyc_module.h"
#include <list>
#include <deque>

/* Similar interface to PycObject, so PycRef can work on it... *
 * However, this does *NOT* mean the two are interchangeable!  */
class ASTNode {
public:
    enum Type {
        NODE_INVALID, NODE_NODELIST, NODE_OBJECT, NODE_UNARY, NODE_BINARY,
        NODE_COMPARE, NODE_SLICE, NODE_STORE, NODE_RETURN, NODE_NAME,
        NODE_DELETE, NODE_FUNCTION, NODE_CLASS, NODE_CALL, NODE_IMPORT,
        NODE_TUPLE, NODE_LIST, NODE_SET, NODE_MAP, NODE_SUBSCR, NODE_PRINT,
        NODE_CONVERT, NODE_KEYWORD, NODE_RAISE, NODE_EXEC, NODE_BLOCK,
        NODE_COMPREHENSION, NODE_LOADBUILDCLASS, NODE_AWAITABLE,
        NODE_FORMATTEDVALUE, NODE_JOINEDSTR, NODE_CONST_MAP,
        NODE_ANNOTATED_VAR, NODE_CHAINSTORE, NODE_TERNARY,
        NODE_KW_NAMES_MAP,

        // Empty node types
        NODE_LOCALS,
    };

    ASTNode(int type = NODE_INVALID) : m_refs(), m_type(type), m_processed() { }
    virtual ~ASTNode() { }

    int type() const { return internalGetType(this); }

    bool processed() const { return m_processed; }
    void setProcessed() { m_processed = true; }

private:
    int m_refs;
    int m_type;
    bool m_processed;

    // Hack to make clang happy :(
    static int internalGetType(const ASTNode *node)
    {
        return node ? node->m_type : NODE_INVALID;
    }

    static void internalAddRef(ASTNode *node)
    {
        if (node)
            ++node->m_refs;
    }

    static void internalDelRef(ASTNode *node)
    {
        if (node && --node->m_refs == 0)
            delete node;
    }

public:
    void addRef() { internalAddRef(this); }
    void delRef() { internalDelRef(this); }
};


class ASTNodeList : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> list_t;

    ASTNodeList(list_t nodes)
        : ASTNode(NODE_NODELIST), m_nodes(std::move(nodes)) { }

    const list_t& nodes() const { return m_nodes; }
    void removeFirst();
    void removeLast();
    void append(PycRef<ASTNode> node) { m_nodes.emplace_back(std::move(node)); }

protected:
    ASTNodeList(list_t nodes, ASTNode::Type type)
        : ASTNode(type), m_nodes(std::move(nodes)) { }

private:
    list_t m_nodes;
};


class ASTChainStore : public ASTNodeList {
public:
    ASTChainStore(list_t nodes, PycRef<ASTNode> src)
        : ASTNodeList(nodes, NODE_CHAINSTORE), m_src(std::move(src)) { }
    
    PycRef<ASTNode> src() const { return m_src; }

private:
    PycRef<ASTNode> m_src;
};


class ASTObject : public ASTNode {
public:
    ASTObject(PycRef<PycObject> obj)
        : ASTNode(NODE_OBJECT), m_obj(std::move(obj)) { }

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
        : ASTNode(NODE_UNARY), m_op(op), m_operand(std::move(operand)) { }

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
        BIN_ATTR, BIN_POWER, BIN_MULTIPLY, BIN_DIVIDE, BIN_FLOOR_DIVIDE,
        BIN_MODULO, BIN_ADD, BIN_SUBTRACT, BIN_LSHIFT, BIN_RSHIFT, BIN_AND,
        BIN_XOR, BIN_OR, BIN_LOG_AND, BIN_LOG_OR, BIN_MAT_MULTIPLY,
        /* Inplace operations */
        BIN_IP_ADD, BIN_IP_SUBTRACT, BIN_IP_MULTIPLY, BIN_IP_DIVIDE,
        BIN_IP_MODULO, BIN_IP_POWER, BIN_IP_LSHIFT, BIN_IP_RSHIFT, BIN_IP_AND,
        BIN_IP_XOR, BIN_IP_OR, BIN_IP_FLOOR_DIVIDE, BIN_IP_MAT_MULTIPLY,
        /* Error Case */
        BIN_INVALID
    };

    ASTBinary(PycRef<ASTNode> left, PycRef<ASTNode> right, int op,
              int type = NODE_BINARY)
        : ASTNode(type), m_op(op), m_left(std::move(left)), m_right(std::move(right)) { }

    PycRef<ASTNode> left() const { return m_left; }
    PycRef<ASTNode> right() const { return m_right; }
    int op() const { return m_op; }
    bool is_inplace() const { return m_op >= BIN_IP_ADD; }
    virtual const char* op_str() const;

    static BinOp from_opcode(int opcode);
    static BinOp from_binary_op(int operand);

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
        : ASTBinary(std::move(left), std::move(right), op, NODE_COMPARE) { }

    const char* op_str() const override;
};


class ASTSlice : public ASTBinary {
public:
    enum SliceOp {
        SLICE0, SLICE1, SLICE2, SLICE3
    };

    ASTSlice(int op, PycRef<ASTNode> left = {}, PycRef<ASTNode> right = {})
        : ASTBinary(std::move(left), std::move(right), op, NODE_SLICE) { }
};


class ASTStore : public ASTNode {
public:
    ASTStore(PycRef<ASTNode> src, PycRef<ASTNode> dest)
        : ASTNode(NODE_STORE), m_src(std::move(src)), m_dest(std::move(dest)) { }

    PycRef<ASTNode> src() const { return m_src; }
    PycRef<ASTNode> dest() const { return m_dest; }

private:
    PycRef<ASTNode> m_src;
    PycRef<ASTNode> m_dest;
};


class ASTReturn : public ASTNode {
public:
    enum RetType {
        RETURN, YIELD, YIELD_FROM
    };

    ASTReturn(PycRef<ASTNode> value, RetType rettype = RETURN)
        : ASTNode(NODE_RETURN), m_value(std::move(value)), m_rettype(rettype) { }

    PycRef<ASTNode> value() const { return m_value; }
    RetType rettype() const { return m_rettype; }

private:
    PycRef<ASTNode> m_value;
    RetType m_rettype;
};


class ASTName : public ASTNode {
public:
    ASTName(PycRef<PycString> name)
        : ASTNode(NODE_NAME), m_name(std::move(name)) { }

    PycRef<PycString> name() const { return m_name; }

private:
    PycRef<PycString> m_name;
};


class ASTDelete : public ASTNode {
public:
    ASTDelete(PycRef<ASTNode> value)
        : ASTNode(NODE_DELETE), m_value(std::move(value)) { }

    PycRef<ASTNode> value() const { return m_value; }

private:
    PycRef<ASTNode> m_value;
};


class ASTFunction : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> defarg_t;

    ASTFunction(PycRef<ASTNode> code, defarg_t defArgs, defarg_t kwDefArgs)
        : ASTNode(NODE_FUNCTION), m_code(std::move(code)),
          m_defargs(std::move(defArgs)), m_kwdefargs(std::move(kwDefArgs)) { }

    PycRef<ASTNode> code() const { return m_code; }
    const defarg_t& defargs() const { return m_defargs; }
    const defarg_t& kwdefargs() const { return m_kwdefargs; }

private:
    PycRef<ASTNode> m_code;
    defarg_t m_defargs;
    defarg_t m_kwdefargs;
};


class ASTClass : public ASTNode {
public:
    ASTClass(PycRef<ASTNode> code, PycRef<ASTNode> bases, PycRef<ASTNode> name)
        : ASTNode(NODE_CLASS), m_code(std::move(code)), m_bases(std::move(bases)),
          m_name(std::move(name)) { }

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
    typedef std::list<PycRef<ASTNode>> pparam_t;
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode>>> kwparam_t;

    ASTCall(PycRef<ASTNode> func, pparam_t pparams, kwparam_t kwparams)
        : ASTNode(NODE_CALL), m_func(std::move(func)), m_pparams(std::move(pparams)),
          m_kwparams(std::move(kwparams)) { }

    PycRef<ASTNode> func() const { return m_func; }
    const pparam_t& pparams() const { return m_pparams; }
    const kwparam_t& kwparams() const { return m_kwparams; }
    PycRef<ASTNode> var() const { return m_var; }
    PycRef<ASTNode> kw() const { return m_kw; }

    bool hasVar() const { return m_var != nullptr; }
    bool hasKW() const { return m_kw != nullptr; }

    void setVar(PycRef<ASTNode> var) { m_var = std::move(var); }
    void setKW(PycRef<ASTNode> kw) { m_kw = std::move(kw); }

private:
    PycRef<ASTNode> m_func;
    pparam_t m_pparams;
    kwparam_t m_kwparams;
    PycRef<ASTNode> m_var;
    PycRef<ASTNode> m_kw;
};


class ASTImport : public ASTNode {
public:
    typedef std::list<PycRef<ASTStore>> list_t;

    ASTImport(PycRef<ASTNode> name, PycRef<ASTNode> fromlist)
        : ASTNode(NODE_IMPORT), m_name(std::move(name)), m_fromlist(std::move(fromlist)) { }

    PycRef<ASTNode> name() const { return m_name; }
    list_t stores() const { return m_stores; }
    void add_store(PycRef<ASTStore> store) { m_stores.emplace_back(std::move(store)); }

    PycRef<ASTNode> fromlist() const { return m_fromlist; }

private:
    PycRef<ASTNode> m_name;
    list_t m_stores;

    PycRef<ASTNode> m_fromlist;
};


class ASTTuple : public ASTNode {
public:
    typedef std::vector<PycRef<ASTNode>> value_t;

    ASTTuple(value_t values)
        : ASTNode(NODE_TUPLE), m_values(std::move(values)),
          m_requireParens(true) { }

    const value_t& values() const { return m_values; }
    void add(PycRef<ASTNode> name) { m_values.emplace_back(std::move(name)); }

    void setRequireParens(bool require) { m_requireParens = require; }
    bool requireParens() const { return m_requireParens; }

private:
    value_t m_values;
    bool m_requireParens;
};


class ASTList : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> value_t;

    ASTList(value_t values)
        : ASTNode(NODE_LIST), m_values(std::move(values)) { }

    const value_t& values() const { return m_values; }

private:
    value_t m_values;
};

class ASTSet : public ASTNode {
public:
    typedef std::deque<PycRef<ASTNode>> value_t;

    ASTSet(value_t values)
        : ASTNode(NODE_SET), m_values(std::move(values)) { }

    const value_t& values() const { return m_values; }

private:
    value_t m_values;
};

class ASTMap : public ASTNode {
public:
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode>>> map_t;

    ASTMap() : ASTNode(NODE_MAP) { }

    void add(PycRef<ASTNode> key, PycRef<ASTNode> value)
    {
        m_values.emplace_back(std::move(key), std::move(value));
    }

    const map_t& values() const { return m_values; }

private:
    map_t m_values;
};

class ASTKwNamesMap : public ASTNode {
public:
    typedef std::list<std::pair<PycRef<ASTNode>, PycRef<ASTNode>>> map_t;

    ASTKwNamesMap() : ASTNode(NODE_KW_NAMES_MAP) { }

    void add(PycRef<ASTNode> key, PycRef<ASTNode> value)
    {
        m_values.emplace_back(std::move(key), std::move(value));
    }

    const map_t& values() const { return m_values; }

private:
    map_t m_values;
};

class ASTConstMap : public ASTNode {
public:
    typedef std::vector<PycRef<ASTNode>> values_t;

    ASTConstMap(PycRef<ASTNode> keys, const values_t& values)
        : ASTNode(NODE_CONST_MAP), m_keys(std::move(keys)), m_values(std::move(values)) { }

    const PycRef<ASTNode>& keys() const { return m_keys; }
    const values_t& values() const { return m_values; }

private:
    PycRef<ASTNode> m_keys;
    values_t m_values;
};


class ASTSubscr : public ASTNode {
public:
    ASTSubscr(PycRef<ASTNode> name, PycRef<ASTNode> key)
        : ASTNode(NODE_SUBSCR), m_name(std::move(name)), m_key(std::move(key)) { }

    PycRef<ASTNode> name() const { return m_name; }
    PycRef<ASTNode> key() const { return m_key; }

private:
    PycRef<ASTNode> m_name;
    PycRef<ASTNode> m_key;
};


class ASTPrint : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> values_t;

    ASTPrint(PycRef<ASTNode> value, PycRef<ASTNode> stream = {})
        : ASTNode(NODE_PRINT), m_stream(std::move(stream)), m_eol()
    {
        if (value != nullptr)
            m_values.emplace_back(std::move(value));
        else
            m_eol = true;
    }

    values_t values() const { return m_values; }
    PycRef<ASTNode> stream() const { return m_stream; }
    bool eol() const { return m_eol; }

    void add(PycRef<ASTNode> value) { m_values.emplace_back(std::move(value)); }
    void setEol(bool eol) { m_eol = eol; }

private:
    values_t m_values;
    PycRef<ASTNode> m_stream;
    bool m_eol;
};


class ASTConvert : public ASTNode {
public:
    ASTConvert(PycRef<ASTNode> name)
        : ASTNode(NODE_CONVERT), m_name(std::move(name)) { }

    PycRef<ASTNode> name() const { return m_name; }

private:
    PycRef<ASTNode> m_name;
};


class ASTKeyword : public ASTNode {
public:
    enum Word {
        KW_PASS, KW_BREAK, KW_CONTINUE
    };

    ASTKeyword(Word key) : ASTNode(NODE_KEYWORD), m_key(key) { }

    Word key() const { return m_key; }
    const char* word_str() const;

private:
    Word m_key;
};


class ASTRaise : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> param_t;

    ASTRaise(param_t params) : ASTNode(NODE_RAISE), m_params(std::move(params)) { }

    const param_t& params() const { return m_params; }

private:
    param_t m_params;
};


class ASTExec : public ASTNode {
public:
    ASTExec(PycRef<ASTNode> stmt, PycRef<ASTNode> glob, PycRef<ASTNode> loc)
        : ASTNode(NODE_EXEC), m_stmt(std::move(stmt)), m_glob(std::move(glob)),
          m_loc(std::move(loc)) { }

    PycRef<ASTNode> statement() const { return m_stmt; }
    PycRef<ASTNode> globals() const { return m_glob; }
    PycRef<ASTNode> locals() const { return m_loc; }

private:
    PycRef<ASTNode> m_stmt;
    PycRef<ASTNode> m_glob;
    PycRef<ASTNode> m_loc;
};


class ASTBlock : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> list_t;

    enum BlkType {
        BLK_MAIN, BLK_IF, BLK_ELSE, BLK_ELIF, BLK_TRY,
        BLK_CONTAINER, BLK_EXCEPT, BLK_FINALLY,
        BLK_WHILE, BLK_FOR, BLK_WITH, BLK_ASYNCFOR
    };

    ASTBlock(BlkType blktype, int end = 0, int inited = 0)
        : ASTNode(NODE_BLOCK), m_blktype(blktype), m_end(end), m_inited(inited) { }

    BlkType blktype() const { return m_blktype; }
    int end() const { return m_end; }
    const list_t& nodes() const { return m_nodes; }
    list_t::size_type size() const { return m_nodes.size(); }
    void removeFirst();
    void removeLast();
    void append(PycRef<ASTNode> node) { m_nodes.emplace_back(std::move(node)); }
    const char* type_str() const;

    virtual int inited() const { return m_inited; }
    virtual void init() { m_inited = 1; }
    virtual void init(int init) { m_inited = init; }

    void setEnd(int end) { m_end = end; }

private:
    BlkType m_blktype;
    int m_end;
    list_t m_nodes;

protected:
    int m_inited;   /* Is the block's definition "complete" */
};


class ASTCondBlock : public ASTBlock {
public:
    enum InitCond {
        UNINITED, POPPED, PRE_POPPED
    };

    ASTCondBlock(ASTBlock::BlkType blktype, int end, PycRef<ASTNode> cond,
                 bool negative = false)
        : ASTBlock(blktype, end), m_cond(std::move(cond)), m_negative(negative) { }

    PycRef<ASTNode> cond() const { return m_cond; }
    bool negative() const { return m_negative; }

private:
    PycRef<ASTNode> m_cond;
    bool m_negative;
};


class ASTIterBlock : public ASTBlock {
public:
    ASTIterBlock(ASTBlock::BlkType blktype, int start, int end, PycRef<ASTNode> iter)
        : ASTBlock(blktype, end), m_iter(std::move(iter)), m_idx(), m_comp(), m_start(start) { }

    PycRef<ASTNode> iter() const { return m_iter; }
    PycRef<ASTNode> index() const { return m_idx; }
    PycRef<ASTNode> condition() const { return m_cond; }
    bool isComprehension() const { return m_comp; }
    int start() const { return m_start; }

    void setIndex(PycRef<ASTNode> idx) { m_idx = std::move(idx); init(); }
    void setCondition(PycRef<ASTNode> cond) { m_cond = std::move(cond); }
    void setComprehension(bool comp) { m_comp = comp; }

private:
    PycRef<ASTNode> m_iter;
    PycRef<ASTNode> m_idx;
    PycRef<ASTNode> m_cond;
    bool m_comp;
    int m_start;
};

class ASTContainerBlock : public ASTBlock {
public:
    ASTContainerBlock(int finally, int except = 0)
        : ASTBlock(ASTBlock::BLK_CONTAINER, 0), m_finally(finally), m_except(except) { }

    bool hasFinally() const { return m_finally != 0; }
    bool hasExcept() const { return m_except != 0; }
    int finally() const { return m_finally; }
    int except() const { return m_except; }

    void setExcept(int except) { m_except = except; }

private:
    int m_finally;
    int m_except;
};

class ASTWithBlock : public ASTBlock {
public:
    ASTWithBlock(int end)
        : ASTBlock(ASTBlock::BLK_WITH, end) { }

    PycRef<ASTNode> expr() const { return m_expr; }
    PycRef<ASTNode> var() const { return m_var; }

    void setExpr(PycRef<ASTNode> expr) { m_expr = std::move(expr); init(); }
    void setVar(PycRef<ASTNode> var) { m_var = std::move(var); }

private:
    PycRef<ASTNode> m_expr;
    PycRef<ASTNode> m_var;      // optional value
};

class ASTComprehension : public ASTNode {
public:
    typedef std::list<PycRef<ASTIterBlock>> generator_t;

    ASTComprehension(PycRef<ASTNode> result)
        : ASTNode(NODE_COMPREHENSION), m_result(std::move(result)) { }

    PycRef<ASTNode> result() const { return m_result; }
    generator_t generators() const { return m_generators; }

    void addGenerator(PycRef<ASTIterBlock> gen) {
        m_generators.emplace_front(std::move(gen));
    }

private:
    PycRef<ASTNode> m_result;
    generator_t m_generators;

};

class ASTLoadBuildClass : public ASTNode {
public:
    ASTLoadBuildClass(PycRef<PycObject> obj)
        : ASTNode(NODE_LOADBUILDCLASS), m_obj(std::move(obj)) { }

    PycRef<PycObject> object() const { return m_obj; }

private:
    PycRef<PycObject> m_obj;
};

class ASTAwaitable : public ASTNode {
public:
    ASTAwaitable(PycRef<ASTNode> expr)
        : ASTNode(NODE_AWAITABLE), m_expr(std::move(expr)) { }

    PycRef<ASTNode> expression() const { return m_expr; }

private:
    PycRef<ASTNode> m_expr;
};

class ASTFormattedValue : public ASTNode {
public:
    enum ConversionFlag {
        NONE = 0,
        STR = 1,
        REPR = 2,
        ASCII = 3,
        FMTSPEC = 4
    };

    ASTFormattedValue(PycRef<ASTNode> val, ConversionFlag conversion,
                      PycRef<ASTNode> format_spec)
        : ASTNode(NODE_FORMATTEDVALUE),
          m_val(std::move(val)),
          m_conversion(conversion),
          m_format_spec(std::move(format_spec))
    { }

    PycRef<ASTNode> val() const { return m_val; }
    ConversionFlag conversion() const { return m_conversion; }
    PycRef<ASTNode> format_spec() const { return m_format_spec; }

private:
    PycRef<ASTNode> m_val;
    ConversionFlag m_conversion;
    PycRef<ASTNode> m_format_spec;
};

// Same as ASTList
class ASTJoinedStr : public ASTNode {
public:
    typedef std::list<PycRef<ASTNode>> value_t;

    ASTJoinedStr(value_t values)
        : ASTNode(NODE_JOINEDSTR), m_values(std::move(values)) { }

    const value_t& values() const { return m_values; }

private:
    value_t m_values;
};

class ASTAnnotatedVar : public ASTNode {
public:
    ASTAnnotatedVar(PycRef<ASTNode> name, PycRef<ASTNode> type)
        : ASTNode(NODE_ANNOTATED_VAR), m_name(std::move(name)), m_type(std::move(type)) { }

    PycRef<ASTNode> name() const noexcept { return m_name; }
    PycRef<ASTNode> annotation() const noexcept { return m_type; }

private:
    PycRef<ASTNode> m_name;
    PycRef<ASTNode> m_type;
};

class ASTTernary : public ASTNode
{
public:
    ASTTernary(PycRef<ASTNode> if_block, PycRef<ASTNode> if_expr,
               PycRef<ASTNode> else_expr)
        : ASTNode(NODE_TERNARY), m_if_block(std::move(if_block)),
          m_if_expr(std::move(if_expr)), m_else_expr(std::move(else_expr)) { }

    PycRef<ASTNode> if_block() const noexcept { return m_if_block; }
    PycRef<ASTNode> if_expr() const noexcept { return m_if_expr; }
    PycRef<ASTNode> else_expr() const noexcept { return m_else_expr; }

private:
    PycRef<ASTNode> m_if_block; // contains "condition" and "negative"
    PycRef<ASTNode> m_if_expr;
    PycRef<ASTNode> m_else_expr;
};

#endif
