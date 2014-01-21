#include <cstdio>
#include <cstring>
#include <cstdarg>
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
    "CO_NESTED", "CO_GENERATOR", "CO_NOFREE", "<0x80>", "<0x100>", "<0x200>",
    "<0x400>", "<0x800>", "CO_GENERATOR_ALLOWED", "CO_FUTURE_DIVISION",
    "CO_FUTURE_ABSOLUTE_IMPORT", "CO_FUTURE_WITH_STATEMENT",
    "CO_FUTURE_PRINT_FUNCTION", "CO_FUTURE_UNICODE_LITERALS",
    "CO_FUTURE_BARRY_AS_BDFL", "<0x80000>", "<0x100000>", "<0x200000>",
    "<0x400000>", "<0x800000>", "<0x1000000>", "<0x2000000>", "<0x4000000>",
    "<0x8000000>", "<0x10000000>", "<0x20000000>", "<0x40000000>",
    "<0x80000000>"
};

static void print_coflags(unsigned long flags)
{
    if (flags == 0) {
        fprintf(pyc_output, "\n");
        return;
    }

    fprintf(pyc_output, " (");
    unsigned long f = 1;
    int k = 0;
    while (k < 32) {
        if ((flags & f) != 0) {
            flags &= ~f;
            if (flags == 0)
                fprintf(pyc_output, "%s", flag_names[k]);
            else
                fprintf(pyc_output, "%s | ", flag_names[k]);
        }
        ++k;
        f <<= 1;
    }
    fprintf(pyc_output, ")\n");
}

static void ivprintf(int indent, const char* fmt, va_list varargs)
{
    for (int i=0; i<indent; i++)
        fprintf(pyc_output, "    ");
    vprintf(fmt, varargs);
}

static void iprintf(int indent, const char* fmt, ...)
{
    va_list varargs;
    va_start(varargs, fmt);
    ivprintf(indent, fmt, varargs);
    va_end(varargs);
}

void output_object(PycRef<PycObject> obj, PycModule* mod, int indent)
{
    switch (obj->type()) {
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        {
            PycRef<PycCode> codeObj = obj.cast<PycCode>();
            iprintf(indent, "[Code]\n");
            iprintf(indent + 1, "File Name: %s\n", codeObj->fileName()->value());
            iprintf(indent + 1, "Object Name: %s\n", codeObj->name()->value());
            iprintf(indent + 1, "Arg Count: %d\n", codeObj->argCount());
            iprintf(indent + 1, "Locals: %d\n", codeObj->numLocals());
            iprintf(indent + 1, "Stack Size: %d\n", codeObj->stackSize());
            iprintf(indent + 1, "Flags: 0x%08X", codeObj->flags());
            print_coflags(codeObj->flags());

            if (codeObj->names() != Pyc_NULL) {
                iprintf(indent + 1, "[Names]\n");
                for (int i=0; i<codeObj->names()->size(); i++)
                    output_object(codeObj->names()->get(i), mod, indent + 2);
            }

            if (codeObj->varNames() != Pyc_NULL) {
                iprintf(indent + 1, "[Var Names]\n");
                for (int i=0; i<codeObj->varNames()->size(); i++)
                    output_object(codeObj->varNames()->get(i), mod, indent + 2);
            }

            if (codeObj->freeVars() != Pyc_NULL) {
                iprintf(indent + 1, "[Free Vars]\n");
                for (int i=0; i<codeObj->freeVars()->size(); i++)
                    output_object(codeObj->freeVars()->get(i), mod, indent + 2);
            }

            if (codeObj->cellVars() != Pyc_NULL) {
                iprintf(indent + 1, "[Cell Vars]\n");
                for (int i=0; i<codeObj->cellVars()->size(); i++)
                    output_object(codeObj->cellVars()->get(i), mod, indent + 2);
            }

            if (codeObj->consts() != Pyc_NULL) {
                iprintf(indent + 1, "[Constants]\n");
                for (int i=0; i<codeObj->consts()->size(); i++)
                    output_object(codeObj->consts()->get(i), mod, indent + 2);
            }

            iprintf(indent + 1, "[Disassembly]\n");
            bc_disasm(codeObj, mod, indent + 2);
        }
        break;
    case PycObject::TYPE_STRING:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0);
        fprintf(pyc_output, "\n");
        break;
    case PycObject::TYPE_UNICODE:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u');
        fprintf(pyc_output, "\n");
        break;
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), 0);
        fprintf(pyc_output, "\n");
        break;
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            iprintf(indent, "(\n");
            PycTuple::value_t values = obj.cast<PycTuple>()->values();
            for (PycTuple::value_t::const_iterator i = values.begin(); i != values.end(); i++)
                output_object(*i, mod, indent + 1);
            iprintf(indent, ")\n");
        }
        break;
    case PycObject::TYPE_LIST:
        {
            iprintf(indent, "[\n");
            PycList::value_t values = obj.cast<PycList>()->values();
            for (PycList::value_t::const_iterator i = values.begin(); i != values.end(); i++)
                output_object(*i, mod, indent + 1);
            iprintf(indent, "]\n");
        }
        break;
    case PycObject::TYPE_DICT:
        {
            iprintf(indent, "{\n");
            PycDict::key_t keys = obj.cast<PycDict>()->keys();
            PycDict::value_t values = obj.cast<PycDict>()->values();
            PycDict::key_t::const_iterator ki = keys.begin();
            PycDict::value_t::const_iterator vi = values.begin();
            while (ki != keys.end()) {
                output_object(*ki, mod, indent + 1);
                output_object(*vi, mod, indent + 2);
                ++ki, ++vi;
            }
            iprintf(indent, "}\n");
        }
        break;
    case PycObject::TYPE_SET:
        {
            iprintf(indent, "{\n");
            PycSet::value_t values = obj.cast<PycSet>()->values();
            for (PycSet::value_t::const_iterator i = values.begin(); i != values.end(); i++)
                output_object(*i, mod, indent + 1);
            iprintf(indent, "}\n");
        }
        break;
    case PycObject::TYPE_NONE:
        iprintf(indent, "None\n");
        break;
    case PycObject::TYPE_FALSE:
        iprintf(indent, "False\n");
        break;
    case PycObject::TYPE_TRUE:
        iprintf(indent, "True\n");
        break;
    case PycObject::TYPE_INT:
        iprintf(indent, "%d\n", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_LONG:
        iprintf(indent, "%s\n", obj.cast<PycLong>()->repr().c_str());
        break;
    case PycObject::TYPE_FLOAT:
        iprintf(indent, "%s\n", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        iprintf(indent, "(%s+%sj)\n", obj.cast<PycComplex>()->value(),
                                      obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        iprintf(indent, "%g\n", obj.cast<PycCFloat>()->value());
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        iprintf(indent, "(%g+%gj)\n", obj.cast<PycCComplex>()->value(),
                                      obj.cast<PycCComplex>()->imag());
        break;
    default:
        iprintf(indent, "<TYPE: %d>\n", obj->type());
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "No input file specified\n");
        return 1;
    }

    PycModule mod;
    mod.loadFromFile(argv[1]);
    const char* dispname = strrchr(argv[1], PATHSEP);
    dispname = (dispname == NULL) ? argv[1] : dispname + 1;
    fprintf(pyc_output, "%s (Python %d.%d%s)\n", dispname, mod.majorVer(), mod.minorVer(),
           (mod.majorVer() < 3 && mod.isUnicode()) ? " -U" : "");
    output_object(mod.code().cast<PycObject>(), &mod, 0);

    return 0;
}
