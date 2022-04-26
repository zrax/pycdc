#include <cstring>
#include "ASTree.h"

#ifdef WIN32
#  define PATHSEP '\\'
#else
#  define PATHSEP '/'
#endif

int main(int argc, char* argv[])
{
    const char* infile = nullptr;
    for (int arg = 1; arg < argc; ++arg) {
        if (strcmp(argv[arg], "-o") == 0) {
            if (arg + 1 < argc) {
                const char* filename = argv[++arg];
                FILE* outfile = fopen(filename, "w");
                if (!outfile) {
                    fprintf(stderr, "Error opening file '%s' for writing\n",
                            argv[arg]);
                    return 1;
                }
                pyc_output = outfile;
            } else {
                fputs("Option '-o' requires a filename\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            fprintf(stderr, "Usage:  %s [options] input.pyc\n\n", argv[0]);
            fputs("Options:\n", stderr);
            fputs("  -o <filename>   Write output to <filename> (default: stdout)\n", stderr);
            fputs("  --help          Show this help text and then exit\n", stderr);
            return 0;
        } else {
            infile = argv[arg];
        }
    }

    if (!infile) {
        fputs("No input file specified\n", stderr);
        return 1;
    }

    PycModule mod;
    try {
        mod.loadFromFile(infile);
    } catch (std::exception& ex) {
        fprintf(stderr, "Error loading file %s: %s\n", infile, ex.what());
        return 1;
    }
    if (!mod.isValid()) {
        fprintf(stderr, "Could not load file %s\n", infile);
        return 1;
    }
    const char* dispname = strrchr(infile, PATHSEP);
    dispname = (dispname == NULL) ? infile : dispname + 1;
    fputs("# Source Generated with Decompyle++\n", pyc_output);
    fprintf(pyc_output, "# File: %s (Python %d.%d%s)\n\n", dispname, mod.majorVer(), mod.minorVer(),
            (mod.majorVer() < 3 && mod.isUnicode()) ? " Unicode" : "");
    try {
        decompyle(mod.code(), &mod);
    } catch (std::exception& ex) {
        fprintf(stderr, "Error decompyling %s: %s\n", infile, ex.what());
        return 1;
    }

    return 0;
}
