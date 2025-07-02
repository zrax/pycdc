#ifndef _PYC_FASTSTACK_H
#define _PYC_FASTSTACK_H

#include "ASTNode.h"
#include <stack>

class FastStack {
public:
    FastStack(int size) : m_ptr(-1) { m_stack.resize(size); }

    FastStack(const FastStack& copy)
        : m_stack(copy.m_stack), m_ptr(copy.m_ptr) { }

    FastStack& operator=(const FastStack& copy)
    {
        m_stack = copy.m_stack;
        m_ptr = copy.m_ptr;
        return *this;
    }

    void push(PycRef<ASTNode> node)
    {
        if (static_cast<int>(m_stack.size()) == m_ptr + 1)
            m_stack.emplace_back(nullptr);

        m_stack[++m_ptr] = std::move(node);
    }

    void pop()
    {
        if (m_ptr > -1)
            m_stack[m_ptr--] = nullptr;
        else {
            #ifdef BLOCK_DEBUG
                fprintf(stderr, "pop from empty stack\n");
            #endif
        }
    }

    PycRef<ASTNode> top(int i = 1) const
    {
        if (i > 0) {
            int idx = m_ptr + 1 - i;
            if ((m_ptr > -1) && (idx >= 0))
                return m_stack[idx];
            else {
                #ifdef BLOCK_DEBUG
                    fprintf(stderr, "insufficient values on stack\n");
                #endif
                return nullptr;
            }
        }
        else {
            fprintf(stderr, "incorrect operand %i\n", i);
            return nullptr;
        }
    }

    bool empty() const
    {
        return m_ptr == -1;
    }

private:
    std::vector<PycRef<ASTNode>> m_stack;
    int m_ptr;
};

typedef std::stack<FastStack> stackhist_t;

#endif
