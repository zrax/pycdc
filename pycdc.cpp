#include <cstring>
#include "ASTree.h"

#ifdef WIN32
#  define PATHSEP '\\'
#else
#  define PATHSEP '/'
#endif

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
    const char* dispname = strrchr(argv[1], PATHSEP);
    dispname = (dispname == NULL) ? argv[1] : dispname + 1;
    fprintf(pyc_output, "# Source Generated with Decompyle++\n");
    fprintf(pyc_output, "# File: %s (Python %d.%d%s)\n\n", dispname, mod.majorVer(), mod.minorVer(),
            (mod.majorVer() < 3 && mod.isUnicode()) ? " Unicode" : "");
    decompyle(mod.code(), &mod);

    return 0;
}
