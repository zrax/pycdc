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
        fputs("No input file specified\n", stderr);
        return 1;
    }

    PycModule mod;
    try {
        mod.loadFromFile(argv[1]);
    } catch (std::exception& ex) {
        fprintf(stderr, "Error loading file %s: %s\n", argv[1], ex.what());
        return 1;
    }
    if (!mod.isValid()) {
        fprintf(stderr, "Could not load file %s\n", argv[1]);
        return 1;
    }
    const char* dispname = strrchr(argv[1], PATHSEP);
    dispname = (dispname == NULL) ? argv[1] : dispname + 1;
    fputs("# Source Generated with Decompyle++\n", pyc_output);
    fprintf(pyc_output, "# File: %s (Python %d.%d%s)\n\n", dispname, mod.majorVer(), mod.minorVer(),
            (mod.majorVer() < 3 && mod.isUnicode()) ? " Unicode" : "");
    try {
        decompyle(mod.code(), &mod);
    } catch (std::exception& ex) {
        fprintf(stderr, "Error decompyling %s: %s\n", argv[1], ex.what());
        return 1;
    }

    return 0;
}
