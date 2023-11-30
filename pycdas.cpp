#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <iostream>
#include <fstream>
#include "pyc_module.h"
#include "pyc_numeric.h"
#include "bytecode.h"

#ifdef WIN32
#  define PATHSEP '\\'
#else
#  define PATHSEP '/'
#endif

static const char* flag_names[] = {
    "CO_OPTIMIZED", "CO_NEWLOCALS", "CO_VARARGS", "CO_VARKEYWORDS",
    "CO_NESTED", "CO_GENERATOR", "CO_NOFREE", "CO_COROUTINE",
    "CO_ITERABLE_COROUTINE", "<0x200>", "<0x400>", "<0x800>",
    "CO_GENERATOR_ALLOWED", "CO_FUTURE_DIVISION",
    "CO_FUTURE_ABSOLUTE_IMPORT", "CO_FUTURE_WITH_STATEMENT",
    "CO_FUTURE_PRINT_FUNCTION", "CO_FUTURE_UNICODE_LITERALS",
    "CO_FUTURE_BARRY_AS_BDFL", "CO_FUTURE_GENERATOR_STOP",
    "<0x100000>", "<0x200000>", "<0x400000>", "<0x800000>",
    "<0x1000000>", "<0x2000000>", "<0x4000000>", "<0x8000000>",
    "<0x10000000>", "<0x20000000>", "<0x40000000>", "<0x80000000>"
};

static void print_coflags(unsigned long flags, std::ostream& pyc_output)
{
    if (flags == 0) {
        pyc_output << "\n";
        return;
    }

    pyc_output << " (";
    unsigned long f = 1;
    int k = 0;
    while (k < 32) {
        if ((flags & f) != 0) {
            flags &= ~f;
            if (flags == 0)
                pyc_output << flag_names[k];
            else
                pyc_output << flag_names[k] << " | ";
        }
        ++k;
        f <<= 1;
    }
    pyc_output << ")\n";
}

static void iputs(std::ostream& pyc_output, int indent, const char* text)
{
    for (int i=0; i<indent; i++)
        pyc_output << "    ";
    pyc_output << text;
}

static void ivprintf(std::ostream& pyc_output, int indent, const char* fmt,
                     va_list varargs)
{
    for (int i=0; i<indent; i++)
        pyc_output << "    ";
    formatted_printv(pyc_output, fmt, varargs);
}

static void iprintf(std::ostream& pyc_output, int indent, const char* fmt, ...)
{
    va_list varargs;
    va_start(varargs, fmt);
    ivprintf(pyc_output, indent, fmt, varargs);
    va_end(varargs);
}

void output_object(PycRef<PycObject> obj, PycModule* mod, int indent,
                   unsigned flags, std::ostream& pyc_output)
{
    if (obj == NULL) {
        iputs(pyc_output, indent, "<NULL>");
        return;
    }

    switch (obj->type()) {
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        {
            PycRef<PycCode> codeObj = obj.cast<PycCode>();
            iputs(pyc_output, indent, "[Code]\n");
            iprintf(pyc_output, indent + 1, "File Name: %s\n", codeObj->fileName()->value());
            iprintf(pyc_output, indent + 1, "Object Name: %s\n", codeObj->name()->value());
            if (mod->verCompare(3, 11) >= 0)
                iprintf(pyc_output, indent + 1, "Qualified Name: %s\n", codeObj->qualName()->value());
            iprintf(pyc_output, indent + 1, "Arg Count: %d\n", codeObj->argCount());
            if (mod->verCompare(3, 8) >= 0)
                iprintf(pyc_output, indent + 1, "Pos Only Arg Count: %d\n", codeObj->posOnlyArgCount());
            if (mod->majorVer() >= 3)
                iprintf(pyc_output, indent + 1, "KW Only Arg Count: %d\n", codeObj->kwOnlyArgCount());
            if (mod->verCompare(3, 11) < 0)
                iprintf(pyc_output, indent + 1, "Locals: %d\n", codeObj->numLocals());
            if (mod->verCompare(1, 5) >= 0)
                iprintf(pyc_output, indent + 1, "Stack Size: %d\n", codeObj->stackSize());
            if (mod->verCompare(1, 3) >= 0) {
                iprintf(pyc_output, indent + 1, "Flags: 0x%08X", codeObj->flags());
                print_coflags(codeObj->flags(), pyc_output);
            }

            iputs(pyc_output, indent + 1, "[Names]\n");
            for (int i=0; i<codeObj->names()->size(); i++)
                output_object(codeObj->names()->get(i), mod, indent + 2, flags, pyc_output);

            if (mod->verCompare(1, 3) >= 0) {
                if (mod->verCompare(3, 11) >= 0)
                    iputs(pyc_output, indent + 1, "[Locals+Names]\n");
                else
                    iputs(pyc_output, indent + 1, "[Var Names]\n");
                for (int i=0; i<codeObj->localNames()->size(); i++)
                    output_object(codeObj->localNames()->get(i), mod, indent + 2, flags, pyc_output);
            }

            if (mod->verCompare(3, 11) >= 0 && (flags & Pyc::DISASM_PYCODE_VERBOSE) != 0) {
                iputs(pyc_output, indent + 1, "[Locals+Kinds]\n");
                output_object(codeObj->localKinds().cast<PycObject>(), mod, indent + 2, flags, pyc_output);
            }

            if (mod->verCompare(2, 1) >= 0 && mod->verCompare(3, 11) < 0) {
                iputs(pyc_output, indent + 1, "[Free Vars]\n");
                for (int i=0; i<codeObj->freeVars()->size(); i++)
                    output_object(codeObj->freeVars()->get(i), mod, indent + 2, flags, pyc_output);

                iputs(pyc_output, indent + 1, "[Cell Vars]\n");
                for (int i=0; i<codeObj->cellVars()->size(); i++)
                    output_object(codeObj->cellVars()->get(i), mod, indent + 2, flags, pyc_output);
            }

            iputs(pyc_output, indent + 1, "[Constants]\n");
            for (int i=0; i<codeObj->consts()->size(); i++)
                output_object(codeObj->consts()->get(i), mod, indent + 2, flags, pyc_output);

            iputs(pyc_output, indent + 1, "[Disassembly]\n");
            bc_disasm(pyc_output, codeObj, mod, indent + 2, flags);

            if (mod->verCompare(1, 5) >= 0 && (flags & Pyc::DISASM_PYCODE_VERBOSE) != 0) {
                iprintf(pyc_output, indent + 1, "First Line: %d\n", codeObj->firstLine());
                iputs(pyc_output, indent + 1, "[Line Number Table]\n");
                output_object(codeObj->lnTable().cast<PycObject>(), mod, indent + 2, flags, pyc_output);
            }

            if (mod->verCompare(3, 11) >= 0 && (flags & Pyc::DISASM_PYCODE_VERBOSE) != 0) {
                iputs(pyc_output, indent + 1, "[Exception Table]\n");
                output_object(codeObj->exceptTable().cast<PycObject>(), mod, indent + 2, flags, pyc_output);
            }
        }
        break;
    case PycObject::TYPE_STRING:
    case PycObject::TYPE_UNICODE:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        iputs(pyc_output, indent, "");
        obj.cast<PycString>()->print(pyc_output, mod);
        pyc_output << "\n";
        break;
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            iputs(pyc_output, indent, "(\n");
            for (const auto& val : obj.cast<PycTuple>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, ")\n");
        }
        break;
    case PycObject::TYPE_LIST:
        {
            iputs(pyc_output, indent, "[\n");
            for (const auto& val : obj.cast<PycList>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, "]\n");
        }
        break;
    case PycObject::TYPE_DICT:
        {
            iputs(pyc_output, indent, "{\n");
            for (const auto& val : obj.cast<PycDict>()->values()) {
                output_object(std::get<0>(val), mod, indent + 1, flags, pyc_output);
                output_object(std::get<1>(val), mod, indent + 2, flags, pyc_output);
            }
            iputs(pyc_output, indent, "}\n");
        }
        break;
    case PycObject::TYPE_SET:
        {
            iputs(pyc_output, indent, "{\n");
            for (const auto& val : obj.cast<PycSet>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, "}\n");
        }
        break;
    case PycObject::TYPE_FROZENSET:
        {
            iputs(pyc_output, indent, "frozenset({\n");
            for (const auto& val : obj.cast<PycSet>()->values())
                output_object(val, mod, indent + 1, flags, pyc_output);
            iputs(pyc_output, indent, "})\n");
        }
        break;
    case PycObject::TYPE_NONE:
        iputs(pyc_output, indent, "None\n");
        break;
    case PycObject::TYPE_FALSE:
        iputs(pyc_output, indent, "False\n");
        break;
    case PycObject::TYPE_TRUE:
        iputs(pyc_output, indent, "True\n");
        break;
    case PycObject::TYPE_ELLIPSIS:
        iputs(pyc_output, indent, "...\n");
        break;
    case PycObject::TYPE_INT:
        iprintf(pyc_output, indent, "%d\n", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_LONG:
        iprintf(pyc_output, indent, "%s\n", obj.cast<PycLong>()->repr().c_str());
        break;
    case PycObject::TYPE_FLOAT:
        iprintf(pyc_output, indent, "%s\n", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        iprintf(pyc_output, indent, "(%s+%sj)\n", obj.cast<PycComplex>()->value(),
                                      obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        iprintf(pyc_output, indent, "%g\n", obj.cast<PycCFloat>()->value());
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        iprintf(pyc_output, indent, "(%g+%gj)\n", obj.cast<PycCComplex>()->value(),
                                      obj.cast<PycCComplex>()->imag());
        break;
    default:
        iprintf(pyc_output, indent, "<TYPE: %d>\n", obj->type());
    }
}

int main(int argc, char* argv[])
{
    const char* infile = nullptr;
    bool marshalled = false;
    const char* version = nullptr;
    unsigned disasm_flags = 0;
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
        } else if (strcmp(argv[arg], "--pycode-extra") == 0) {
            disasm_flags |= Pyc::DISASM_PYCODE_VERBOSE;
        } else if (strcmp(argv[arg], "--show-caches") == 0) {
            disasm_flags |= Pyc::DISASM_SHOW_CACHES;
        } else if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            fprintf(stderr, "Usage:  %s [options] input.pyc\n\n", argv[0]);
            fputs("Options:\n", stderr);
            fputs("  -o <filename>  Write output to <filename> (default: stdout)\n", stderr);
            fputs("  -c             Specify loading a compiled code object. Requires the version to be set\n", stderr);
            fputs("  -v <x.y>       Specify a Python version for loading a compiled code object\n", stderr);
            fputs("  --pycode-extra Show extra fields in PyCode object dumps\n", stderr);
            fputs("  --show-caches  Don't suprress CACHE instructions in Python 3.11+ disassembly\n", stderr);
            fputs("  --help         Show this help text and then exit\n", stderr);
            return 0;
        } else if (argv[arg][0] == '-') {
            fprintf(stderr, "Error: Unrecognized argument %s\n", argv[arg]);
            return 1;
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
        } catch (std::exception &ex) {
            fprintf(stderr, "Error disassembling %s: %s\n", infile, ex.what());
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
    const char* dispname = strrchr(infile, PATHSEP);
    dispname = (dispname == NULL) ? infile : dispname + 1;
    formatted_print(*pyc_output, "%s (Python %d.%d%s)\n", dispname,
                    mod.majorVer(), mod.minorVer(),
                    (mod.majorVer() < 3 && mod.isUnicode()) ? " -U" : "");
    try {
        output_object(mod.code().try_cast<PycObject>(), &mod, 0, disasm_flags,
                      *pyc_output);
    } catch (std::exception& ex) {
        fprintf(stderr, "Error disassembling %s: %s\n", infile, ex.what());
        return 1;
    }

    return 0;
}
