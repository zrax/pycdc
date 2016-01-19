#include <cstring>
#include "ASTree.h"

#ifdef WIN32
#  define PATHSEP '\\'
#else
#  define PATHSEP '/'
#endif


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
		exitMain( ERR_InputFileNotSet )
    }

	try {

		PycModule mod;
		mod.loadFromFile( argv[1] );
		if ( !mod.isValid() || 
			 (mod.code() == Pyc_NULL) ) {

			logErr( "Could not load file %s\n", argv[1]);
			exitMain( ERR_ParsingInputFile )

		}

	 // output logo
		const char* dispname = strrchr(argv[1], PATHSEP);
		dispname = (dispname == NULL) ? argv[1] : dispname + 1;
		log ( "# Source Generated with Decompyle++\n");
		log ( "# File: %s (Python %d.%d%s)\n\n", dispname, mod.majorVer(), mod.minorVer(),
				( (mod.majorVer() < 3) && 
				   mod.isUnicode()        ) ? " Unicode" : "");

	 // Start decompile
		decompyle(mod.code(), &mod);

		exitMain( Success )

	}
	catch (ExitCodes exitCode) {
		logVerb("ExitCode: %i\n", exitCode);
		exitMain( exitCode )
	}
	catch (...) {
		// catch unexpected error - mostly some nullpointer or accessviolations
		logErr("Whoops an unexpected error occured.\n");
		exitMain( ERR_InputFileNotSet )

	}
}
