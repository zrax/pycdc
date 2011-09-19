#ifndef _PYC_FASTSTACK_H
#define _PYC_FASTSTACK_H

#include "ASTNode.h"
#include <stack>

class FastStack {
public:
    FastStack(int size) : m_size(size), m_ptr(-1) {
        m_stack = new PycRef<ASTNode>[m_size];
    }

    FastStack(const FastStack& copy) : m_size(copy.m_size), m_ptr(copy.m_ptr) {
        m_stack = new PycRef<ASTNode>[m_size];

        for (int i = 0; i <= m_ptr; i++)
            m_stack[i] = copy.m_stack[i];
    }

    ~FastStack() {
        delete[] m_stack;
    }

    FastStack& operator=(const FastStack& copy) {
        replace(copy);
        return *this;
    }

    void push(PycRef<ASTNode> node) {
        if (m_size == m_ptr + 1)
            grow(1);

        m_stack[++m_ptr] = node;
    }

    void pop() {
        if (m_ptr > -1)
            m_stack[m_ptr--] = Node_NULL;
    }

    PycRef<ASTNode> top() const
    {
        if (m_ptr > -1)
            return m_stack[m_ptr];
        else
            return Node_NULL;
    }

    void replace(const FastStack& copy)
    {
        if (&copy == this)
            return;
        delete[] m_stack;

        m_size = copy.m_size;
        m_ptr = copy.m_ptr;
        m_stack = new PycRef<ASTNode>[m_size];
        for (int i = 0; i <= m_ptr; i++)
            m_stack[i] = copy.m_stack[i];
    }

    void grow(int inc)
    {
        m_size += inc;
        PycRef<ASTNode>* tmp = new PycRef<ASTNode>[m_size];

        for (int i = 0; i <= m_ptr; i++)
            tmp[i] = m_stack[i];

        delete[] m_stack;
        m_stack = tmp;
    }

private:
    PycRef<ASTNode>* m_stack;
    int m_size, m_ptr;
};

typedef std::stack<FastStack> stackhist_t;

#endif
