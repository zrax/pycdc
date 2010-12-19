#ifndef _PYC_FASTSTACK_H
#define _PYC_FASTSTACK_H

#include "ASTNode.h"
#include <stack>

class FastStack {
public:
    FastStack(int size) : m_size(size), m_ptr(-1)
    { m_stack = new PycRef<ASTNode>[m_size]; }

    FastStack(const FastStack& copy) : m_size(copy.m_size), m_ptr(copy.m_ptr)
    {
        m_stack = new PycRef<ASTNode>[m_size];
        for (int i=0; i<m_size; i++)
            m_stack[i] = copy.m_stack[i];
    }

    ~FastStack()
    { delete[] m_stack; }

    void push(PycRef<ASTNode> node)
    { m_stack[++m_ptr] = node; }

    void pop()
    { if (m_ptr > -1) m_stack[m_ptr--] = Node_NULL; }

    PycRef<ASTNode> top() const
    {
        if (m_ptr > -1)
            return m_stack[m_ptr];
        else
            return Node_NULL;
    }

    void replace(const FastStack& copy)
    {
        for (int i=0; i<=copy.m_ptr; i++)
            m_stack[i] = copy.m_stack[i];
        for (int i=copy.m_ptr+1; i<=m_ptr; i++)
            m_stack[i] = Node_NULL;
        m_ptr = copy.m_ptr;
    }

private:
    PycRef<ASTNode>* m_stack;
    int m_size, m_ptr;
};

typedef std::stack<FastStack> stackhist_t;

#endif
