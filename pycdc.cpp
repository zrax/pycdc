#include "ASTree.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "No input file specified\n");
        return 1;
    }

    PycModule mod;
    mod.loadFromFile(argv[1]);
    if (!mod.isValid()) {
        fprintf(stderr, "Could not load file %s\n", argv[1]);
        return 1;
    }
    printf("# Source Generated with Decompyle++\n");
    printf("# File: %s (Python %d.%d%s)\n", argv[1], mod.majorVer(), mod.minorVer(),
           (mod.majorVer() < 3 && mod.isUnicode()) ? " Unicode" : "");
    decompyle(mod.code(), &mod);

    return 0;
}
