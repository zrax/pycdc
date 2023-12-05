#include "bytecode.h"

#define BEGIN_MAP(maj, min) \
    int python_##maj##_##min##_map(int id) \
    { \
        switch (id) {

#define MAP_OP(op, name) \
        case op: return Pyc::name;

#define END_MAP() \
        default: return Pyc::PYC_INVALID_OPCODE; \
        } \
    }
