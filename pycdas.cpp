#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "pyc_module.h"
#include "pyc_numeric.h"
#include "bytecode.h"

//#include "ASTree.h"


#ifdef WIN32
#  define PATHSEP '\\'
#else
#  define PATHSEP '/'
#endif

static const char* flag_names[] = {
	/*0x0000 0001*/ "CO_OPTIMIZED",				"CO_NEWLOCALS",		"CO_VARARGS",			"CO_VARKEYWORDS",
	/*0x0000 0010*/ "CO_NESTED",				"CO_GENERATOR",		"CO_NOFREE",			"CO_COROUTINE",
	/*0x0000 0100*/ "CO_ITERABLE_COROUTINE",	"<0x200>",			"<0x400>",				"<0x800>",
	
	/*0x0000 1000*/ "CO_GENERATOR_ALLOWED",		"CO_FUTURE_DIVISION",
	/*0x0000 8000*/ "CO_FUTURE_ABSOLUTE_IMPORT","CO_FUTURE_WITH_STATEMENT",
	
	/*0x0001 0000*/ "CO_FUTURE_PRINT_FUNCTION",	"CO_FUTURE_UNICODE_LITERALS",
	/*0x0008 0000*/ "CO_FUTURE_BARRY_AS_BDFL",	"CO_FUTURE_GENERATOR_STOP",

	/*0x0010 0000*/ "<0x100000>",			"<0x200000>",			"<0x400000>",			"<0x800000>",
	/*0x0100 0000*/ "<0x1000000>",			"<0x2000000>",			"<0x4000000>",			"<0x8000000>",
	/*0x1000 0000*/	"<0x10000000>",			"<0x20000000>",			"<0x40000000>",			"<0x80000000>",
};

static void print_coflags(unsigned long flags)
{
    if (flags != 0) {

		unsigned long selectorMask = 1;
		bool isFirstFlag = true;

		for (int k = 0; k < 32;		selectorMask <<= 1, k++ ) {

			// is flag[k] set ?
			if ( flags & selectorMask ) {

				if (isFirstFlag) { isFirstFlag = false;
					log(" (");
				}
	
				log( "%s" , flag_names[k]);

				// isLastFlag ?  (remove current flag in Flags and after doing so, check if Flags get Zero)
				if (!(flags &= ~selectorMask)) {
					log(")");
					break;
				}
				log(" | ");
			}
		}
	}

	log("\n");
}

template <typename ... Args> void
iprintf(int indent, char const *const	format,
	Args const& ...		args) noexcept
{
	for (int i = 0; i<indent; i++)
		log("    ");
	log( format, args ...);
}
/*
static void ivprintf(int indent, const char* fmt, va_list varargs)
{
    for (int i=0; i<indent; i++)
		log("    ");
    vprintf(fmt, varargs);
}

static void iprintf(int indent, const char* fmt, ...)
{
    va_list varargs;
    va_start(varargs, fmt);
    ivprintf(indent, fmt, varargs);
    va_end(varargs);
}
*/



void output_object(PycRef<PycObject> obj, PycModule* mod, int indent)
{
    switch (obj->type()) {
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        {
            PycRef<PycCode> codeObj = obj.cast<PycCode>();
            iprintf( indent, "[Code]\n");
            iprintf( indent + 1, "File Name: %s\n",		codeObj->fileName()->value()	);
            iprintf( indent + 1, "Object Name: %s\n",	codeObj->name()->value()	);
            iprintf( indent + 1, "Arg Count: %d\n",		codeObj->argCount()		);
            iprintf( indent + 1, "Locals: %d\n",		codeObj->numLocals()	);
            iprintf( indent + 1, "Stack Size: %d\n",	codeObj->stackSize()	);
            iprintf( indent + 1, "Flags: 0x%08X",		codeObj->flags()		);
            print_coflags( codeObj->flags() );


#define dump( STRING, OBJECT)											\
																		\
			if (codeObj->OBJECT() != Pyc_NULL) {						\
				iprintf(indent + 1, "["STRING"]\n");					\
				for (int i = 0; i < codeObj->OBJECT()->size();  i++)	\
					output_object( codeObj->OBJECT()->get(i),			\
									mod, indent + 2);					\
			}

			dump( "Names",		names)
			dump( "Var Names",	varNames)
			dump( "Free Vars",	freeVars)
			dump( "Cell Vars",	cellVars)
			dump( "Constants",	consts)

/*			
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
			*/
            iprintf(indent + 1, "[Disassembly]\n");

            bc_disasm(codeObj, mod, indent + 2);
        }
        break;

    case PycObject::TYPE_STRING:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0);
        break;

    case PycObject::TYPE_UNICODE:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u');
        break;

    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        iprintf(indent, "");
        OutputString(obj.cast<PycString>(), 0);
        break;

    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            iprintf(indent, "(\n");
            PycTuple::value_t values = obj.cast<PycTuple>()->values();
            for (PycTuple::value_t::const_iterator i = values.begin(); i != values.end(); i++)
                output_object(*i, mod, indent + 1);
            iprintf(indent, ")");
        }
        break;
    case PycObject::TYPE_LIST:
        {
            iprintf(indent, "[\n");
            PycList::value_t values = obj.cast<PycList>()->values();
            for (PycList::value_t::const_iterator i = values.begin(); i != values.end(); i++)
                output_object(*i, mod, indent + 1);
            iprintf(indent, "]");
        }
        break;
    case PycObject::TYPE_DICT:
        {
	
            iprintf(indent, "{\n");
            PycDict::key_t		keys	= obj.cast<PycDict>()->keys();
            PycDict::value_t	values	= obj.cast<PycDict>()->values();

			PycDict::value_t::const_iterator vi = values.begin();
			PycDict::key_t  ::const_iterator ki = values.begin();

			/* TODO: Get this into some nice For each
			int a[] = { 1, 2, 3 };
			for each (int x in a) { x << ' '; }

			std::vector<int> v(a, a + 3);
			for each (int x in v) { x << ' '; }
		
			for each (ki in obj.cast<PycDict>() ) {
				output_object(ki, mod, indent + 1);
			}
			*/
			
			for ( ;ki != keys.end(); ki++, vi++	) {
                output_object(*ki, mod, indent + 1);
                output_object(*vi, mod, indent + 2);
                
            }
            iprintf(indent, "}");
        }
        break;

    case PycObject::TYPE_SET:
        {
            iprintf(indent, "{\n");
            PycSet::value_t values = obj.cast<PycSet>()->values();
            for (PycSet::value_t::const_iterator i = values.begin(); i != values.end(); i++)
                output_object(*i, mod, indent + 1);
            iprintf(indent, "}");
        }
		break;

    case PycObject::TYPE_NONE:
        iprintf(indent, "None");
        break;

    case PycObject::TYPE_FALSE:
        iprintf(indent, "False");
        break;

    case PycObject::TYPE_TRUE:
        iprintf(indent, "True");
        break;

    case PycObject::TYPE_INT:
        iprintf(indent, "%d", obj.cast<PycInt>()->value());
        break;

    case PycObject::TYPE_LONG:
        iprintf(indent, "%s", obj.cast<PycLong>()->repr().c_str());
        break;

    case PycObject::TYPE_FLOAT:
        iprintf(indent, "%s", obj.cast<PycFloat>()->value());
        break;

    case PycObject::TYPE_COMPLEX:
        iprintf(indent, "(%s+%sj)", obj.cast<PycComplex>()->value(),
                                      obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        iprintf(indent, "%g", obj.cast<PycCFloat>()->value());
        break;

    case PycObject::TYPE_BINARY_COMPLEX:
        iprintf(indent, "(%g+%gj)", obj.cast<PycCComplex>()->value(),
                                      obj.cast<PycCComplex>()->imag());
        break;

    default:
        iprintf(indent, "<TYPE: %d>", obj->type());
    }
	log("\n");

}

#define exitMain( exitCode ) \
	logVerb("ExitCode: %i\n", ERR_InputFileNotSet );	\
	return exitCode;

int main(int argc, char* argv[])
{

	if (argc < 2) {
		logErr( " Decompyle++ [jan'16]"
				" -------------------"
				"\n"
				"   No input file specified\n");
		exitMain(ERR_InputFileNotSet)
	}

	try {

		PycModule mod;
		mod.loadFromFile(argv[1]);
		if (!mod.isValid() ||
			(mod.code() == Pyc_NULL)) {
			logErr("Could not load file %s\n", argv[1]);
			exitMain(ERR_ParsingInputFile)

		}

		// output logo
		const char* dispname = strrchr(argv[1], PATHSEP);
		dispname = (dispname == NULL) ? argv[1] : dispname + 1;
		log("%s (Python %d.%d%s)\n\n", dispname, mod.majorVer(), mod.minorVer(),
			((mod.majorVer() < 3) &&
				mod.isUnicode()) ? " -U" : "");

 // Start disasm
    output_object(mod.code().cast<PycObject>(), &mod, 0);

//	// Start decompile
//	decompyle(mod.code(), &mod);

	exitMain(Success)

	}
	catch (ExitCodes exitCode) {
		logVerb("ExitCode: %i\n", exitCode);
		exitMain(exitCode)
	}
	catch (...) {
		// catch unexpected error - mostly some nullpointer or accessviolations
		logErr("Whoops an unexpected error occured.\n");
		exitMain(ERR_InputFileNotSet)

	}
}
