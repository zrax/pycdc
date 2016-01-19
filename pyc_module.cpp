#include "pyc_module.h"
#include "data.h"

void PycModule::setVersion(unsigned int magic)
{
    // Default for versions that don't support unicode selection
    m_unicode = false;

    switch (magic) {
    case MAGIC_1_0:		m_maj = 1;	 m_min = 0;
        break;
    case MAGIC_1_1:		m_maj = 1;	 m_min = 1;
        break;
    case MAGIC_1_3:		m_maj = 1;	 m_min = 3;
        break;
    case MAGIC_1_4:		m_maj = 1;	 m_min = 4;
        break;
	case MAGIC_1_5:		m_maj = 1;	 m_min = 5;
        break;

    /* Starting with 1.6, Python adds +1 for unicode mode (-U) */

    case MAGIC_1_6+1:	m_unicode = true;
    case MAGIC_1_6:		m_maj = 1;  m_min = 6;
        break;
    case MAGIC_2_0+1:	m_unicode = true;
    case MAGIC_2_0:		m_maj = 2; m_min = 0;
        break;
    case MAGIC_2_1+1:	m_unicode = true;
    case MAGIC_2_1:		m_maj = 2;	m_min = 1;
        break;
    case MAGIC_2_2+1:	m_unicode = true;
    case MAGIC_2_2:		m_maj = 2;	m_min = 2;
        break;
    case MAGIC_2_3+1:	m_unicode = true;
    case MAGIC_2_3:		m_maj = 2;	m_min = 3;
        break;
    case MAGIC_2_4+1:	m_unicode = true;
    case MAGIC_2_4:		m_maj = 2;	m_min = 4;
        break;
    case MAGIC_2_5+1:	m_unicode = true;
    case MAGIC_2_5:		m_maj = 2;	m_min = 5;
        break;
    case MAGIC_2_6+1:
        m_unicode = true;
    case MAGIC_2_6:		m_maj = 2;	m_min = 6;
        break;
    case MAGIC_2_7+1:	m_unicode = true;
    case MAGIC_2_7:		m_maj = 2;	m_min = 7;
        break;

    /* 3.0 and above are always unicode */
    case MAGIC_3_0+1:    m_maj = 3;	m_min = 0;	m_unicode = true;
        break;
    case MAGIC_3_1+1:   m_maj = 3;	m_min = 1;	m_unicode = true;
        break;

    /* 3.2 stops using the unicode increment */
    case MAGIC_3_2:		m_maj = 3;	m_min = 2;	m_unicode = true;
        break;

	case MAGIC_3_3:		m_maj = 3;	m_min = 3;	m_unicode = true;
		break;

	case MAGIC_3_4:		m_maj = 3;	m_min = 4;	m_unicode = true;
		break;

	case MAGIC_3_5:		m_maj = 3;	m_min = 5;	m_unicode = true;
		break;

    /* Bad Magic detected */
    default:			m_maj = -1;	m_min = -1;
    }
}

void PycModule::loadFromFile(const char* filename) 
{
    PycFile in(filename);
    if (!in.isOpen()) {
        logErr( "Error opening file %s\n", filename);
        throw ERR_OpenFileFailed;
    }

	build_magic = in.get32();	logVerb( "Build_magic:\t 0x%.8X\t@0x%.4X\n", build_magic, 0);
    setVersion( build_magic );
    if (!isValid()) {
		logErr( "Unknown MAGIC:\t 0x%.8X !\n", build_magic);
		throw ERR_UnkownPycVersion;
    }
    timestamp = in.get32();		logVerb(  "Timestamp:\t 0x%.8X\t@0x%.4X\n", timestamp, 4  );

	// Timestamp heuristic to fix bug in 'PyInstaller Extractor v1.3' that don't writes the sizeParameter field when extract
	//if ( (timestamp != 0)  && verCompare(3, 3) >= 0  /* build_magic > (MAGIC_Prefix | 3310) */ /*3.4rc2*/)
	if ( verCompare(3, 3) )

        sizeParameter = in.get32(); // Size parameter added in Python 3.3

    m_code = LoadObject(&in, this).cast<PycCode>();
}

PycRef<PycString> PycModule::getIntern(int ref) const
{
    std::list<PycRef<PycString> >::const_iterator it = m_interns.begin();
    while (ref--)
        ++it;
    return *it;
}

PycRef<PycObject> PycModule::getRef(int ref) const
{
    std::list<PycRef<PycObject> >::const_iterator it = m_refs.begin();
    while (ref--)
        ++it;
    return *it;
}
