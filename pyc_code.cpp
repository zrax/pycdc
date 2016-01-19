#include "pyc_code.h"
#include "pyc_module.h"
#include "data.h"

void PycCode::load(PycData* stream, PycModule* mod)
{

#define IsZeroOrAbove(Expr) \
		Expr >= 0

#define IsVerAbove(maj, min) \
		IsZeroOrAbove(	mod->verCompare(maj, min) )


#define StreamGet32_ForVer2_3And1_3( VAR ) \
		IsVerAbove(2, 3)  ?			\
			stream->get32() :		\
		IsVerAbove(1, 3) ?			\
			stream->get16() :	VAR;\

#define StreamGet32_ForVer2_3And1_5( VAR ) \
		IsVerAbove(2, 3)  ?			\
			stream->get32() :		\
		IsVerAbove(1, 5) ?			\
			stream->get16() :	VAR;\

////////////////////////////////////////////////////////////////////////

	m_argCount			=	StreamGet32_ForVer2_3And1_3( m_argCount ) // No ; <- Just a conventions to quickly see that's an macro
	m_kwOnlyArgCount	= 	IsVerAbove(3, 0) ? 
		stream->get32():	m_kwOnlyArgCount;

	m_numLocals			=	StreamGet32_ForVer2_3And1_3( m_numLocals )
	m_stackSize			=	StreamGet32_ForVer2_3And1_5( m_numLocals )
	m_flags				=	StreamGet32_ForVer2_3And1_3( m_flags )

/*
    if (mod->verCompare(1, 3) >= 0 && 
		mod->verCompare(2, 3) < 0)
        m_argCount = stream->get16();

    else if (mod->verCompare(2, 3) >= 0)
        m_argCount = stream->get32();


    if (mod->majorVer() >= 3)
        m_kwOnlyArgCount = stream->get32();

    if (mod->verCompare(1, 3) >= 0 && 
		mod->verCompare(2, 3) < 0)
        m_numLocals = stream->get16();

    else if (mod->verCompare(2, 3) >= 0)
        m_numLocals = stream->get32();


    if (mod->verCompare(1, 5) >= 0 && 
		mod->verCompare(2, 3) < 0)
        m_stackSize = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_stackSize = stream->get32();


    if (mod->verCompare(1, 3) >= 0 && 
		mod->verCompare(2, 3) < 0)
        m_flags = stream->get16();

    else if (mod->verCompare(2, 3) >= 0)
        m_flags = stream->get32();
	*/

#define StreamLoadObject( TYPE) \
	LoadObject(stream, mod).cast<TYPE>();

    m_code	 = StreamLoadObject( PycString )
    m_consts = StreamLoadObject( PycTuple )
    m_names  = StreamLoadObject( PycTuple )

		if ( IsVerAbove( 1, 3) )
    m_varNames = StreamLoadObject(PycTuple)

		if ( IsVerAbove(2, 1) )
    m_freeVars = StreamLoadObject(PycTuple)

		if (IsVerAbove(2, 1) )
    m_cellVars = StreamLoadObject(PycTuple)

    m_fileName	= StreamLoadObject(PycString)
    m_name		= StreamLoadObject(PycString)


	m_firstLine = StreamGet32_ForVer2_3And1_5(m_firstLine)

/*
    if (mod->verCompare(1, 5) >= 0 && 
		mod->verCompare(2, 3) < 0)
        m_firstLine = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_firstLine = stream->get32();
		*/

		if ( IsVerAbove(1, 5) )
    m_lnTable = StreamLoadObject(PycString)
}
