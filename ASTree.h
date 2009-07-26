#ifndef _PYC_ASTREE_H
#define _PYC_ASTREE_H

#include "ASTNode.h"

PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod);

class ASTree {
public:
    void load(PycModule* mod)
    { m_root = BuildFromCode(mod->code(), mod); }

    void printSource(PycModule* mod) const;

private:
    PycRef<ASTNode> m_root;
};

#endif
