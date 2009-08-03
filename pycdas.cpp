#include <cstdio>
#include <cstdarg>
#include "module.h"
#include "bytecode.h"
#include "numeric.h"

static void ivprintf(int indent, const char* fmt, va_list varargs)
{
    for (int i=0; i<indent; i++)
        printf("    ");
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
            iprintf(indent + 1, "Flags: 0x%08X\n", codeObj->flags());

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
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0);
        printf("\n");
        break;
    case PycObject::TYPE_UNICODE:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u');
        printf("\n");
        break;
    case PycObject::TYPE_TUPLE:
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
    printf("%s (Python %d.%d%s)\n", argv[1], mod.majorVer(), mod.minorVer(),
           (mod.majorVer() < 3 && mod.isUnicode()) ? " -U" : "");
    output_object(mod.code().cast<PycObject>(), &mod, 0);

    return 0;
}
