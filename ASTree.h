#ifndef _PYC_ASTREE_H
#define _PYC_ASTREE_H

#include "ASTNode.h"

PycRef<ASTNode> BuildFromCode(PycRef<PycCode> code, PycModule* mod);
void print_src(PycRef<ASTNode> node, PycModule* mod);

void decompyle(PycRef<PycCode> code, PycModule* mod);

#endif
