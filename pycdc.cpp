#include <cstring>
#include <fstream>
#include <iostream>
#include "ASTree.h"

#ifdef WIN32
#  define PATHSEP '\\'
#else
#  define PATHSEP '/'
#endif

int main(int argc, char* argv[])
{
    const char* infile = nullptr;
    bool marshalled = false;
    const char* version = nullptr;
    std::ostream* pyc_output = &std::cout;
    std::ofstream out_file;

    for (int arg = 1; arg < argc; ++arg) {
        if (strcmp(argv[arg], "-o") == 0) {
            if (arg + 1 < argc) {
                const char* filename = argv[++arg];
                out_file.open(filename, std::ios_base::out);
                if (out_file.fail()) {
                    fprintf(stderr, "Error opening file '%s' for writing\n",
                            filename);
                    return 1;
                }
                pyc_output = &out_file;
            } else {
                fputs("Option '-o' requires a filename\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[arg], "-c") == 0) {
            marshalled = true;
        } else if (strcmp(argv[arg], "-v") == 0) {
            if (arg + 1 < argc) {
                version = argv[++arg];
            } else {
                fputs("Option '-v' requires a version\n", stderr);
                return 1;
            }
        } else if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            fprintf(stderr, "Usage:  %s [options] input.pyc\n\n", argv[0]);
            fputs("Options:\n", stderr);
            fputs("  -o <filename>  Write output to <filename> (default: stdout)\n", stderr);
            fputs("  -c             Specify loading a compiled code object. Requires the version to be set\n", stderr);
            fputs("  -v <x.y>       Specify a Python version for loading a compiled code object\n", stderr);
            fputs("  --help         Show this help text and then exit\n", stderr);
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
    if (!marshalled) {
        try {
            mod.loadFromFile(infile);
        } catch (std::exception& ex) {
            fprintf(stderr, "Error loading file %s: %s\n", infile, ex.what());
            return 1;
        }
    } else {
        if (!version) {
            fputs("Opening raw code objects requires a version to be specified\n", stderr);
            return 1;
        }
        std::string s(version);
        auto dot = s.find('.');
        if (dot == std::string::npos || dot == s.size()-1) {
            fputs("Unable to parse version string (use the format x.y)\n", stderr);
            return 1;
        }
        int major = std::stoi(s.substr(0, dot));
        int minor = std::stoi(s.substr(dot+1, s.size()));
        mod.loadFromMarshalledFile(infile, major, minor);
    }

    if (!mod.isValid()) {
        fprintf(stderr, "Could not load file %s\n", infile);
        return 1;
    }
    const char* dispname = strrchr(infile, PATHSEP);
    dispname = (dispname == NULL) ? infile : dispname + 1;
    *pyc_output << "# Source Generated with Decompyle++\n";
    formatted_print(*pyc_output, "# File: %s (Python %d.%d%s)\n\n", dispname,
                    mod.majorVer(), mod.minorVer(),
                    (mod.majorVer() < 3 && mod.isUnicode()) ? " Unicode" : "");
    try {
        decompyle(mod.code(), &mod, *pyc_output);
    } catch (std::exception& ex) {
        fprintf(stderr, "Error decompyling %s: %s\n", infile, ex.what());
        return 1;
    }

    return 0;
}
