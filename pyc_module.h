#ifndef _PYC_MODULE_H
#define _PYC_MODULE_H

#include "pyc_code.h"
#include <list>



const enum ExitCodes {
	Success,

	ERR_UnexpectedError,

	ERR_InputFileNotSet,
	ERR_OpenFileFailed,

	ERR_UnkownPycVersion,
	ERR_ParsingInputFile,



	ERR_UnSpecifiedError

};

const int MAGIC_Prefix = 0x0A0D0000;

// taken from from Python / import.c
// or importlib / _bootstrap.py

enum PycMagic {
    MAGIC_1_0 = 0x00999902,
    MAGIC_1_1 = 0x00999903, /* Also covers 1.2 */
    MAGIC_1_3 = MAGIC_Prefix | 11913	/*0x2E89*/,
    MAGIC_1_4 = MAGIC_Prefix | 5892		/*0x1704*/,
    MAGIC_1_5 = MAGIC_Prefix | 20121	/*0x4E99*/,

	/* Starting with 1.6, Python adds +1 for unicode mode (-U) */
    MAGIC_1_6 = MAGIC_Prefix | 50428	/*0xC4FC*/,

    MAGIC_2_0 = MAGIC_Prefix | 50823	/*0xC687*/,
    MAGIC_2_1 = MAGIC_Prefix | 60202	/*0xEB2A*/,
    MAGIC_2_2 = MAGIC_Prefix | 60717	/*0xED2D*/,
    MAGIC_2_3 = MAGIC_Prefix | 62011	/*0xF23B*/,
	//2_3	62021

	//2_4a0	62041
	//2_4a3	62051
    MAGIC_2_4 = MAGIC_Prefix | 62061	/*0xF26D	2.4b1// */,
    
	//2_5	62071,62081,62091,62092,62101,62111,62121
	MAGIC_2_5 = MAGIC_Prefix | 62131	/*0xF2B3*/,

	//62151
    MAGIC_2_6 = MAGIC_Prefix | 62161	/*0xF2D1*/,
    
	//62171,62181,62191,62201
	MAGIC_2_7 = MAGIC_Prefix | 62211	/*0xF303*/,

	/* 3.0 and above are always unicode */
	//3000,3010..3060,3061,3071..3111,3103
    MAGIC_3_0 = MAGIC_Prefix | 3130		/*0x0C3A*/,
    MAGIC_3_1 = MAGIC_Prefix | 3151		/*0x0C4E*/,
    MAGIC_3_2 = MAGIC_Prefix | 3180		/*0x0C6C*/,
    MAGIC_3_3 = MAGIC_Prefix | 3230		/*0x0C9E*/,
    MAGIC_3_4 = MAGIC_Prefix | 3310		/*0x0CEE*/,
    MAGIC_3_5 = MAGIC_Prefix | 3350		/*0x0D16*/,
};

#define PYC_VERSION(maj, min)  MAGIC_##maj##_##min


// Templates for logging / Text output

	template <typename ... Args> void
	logErr(char const *const	format,
		Args const& ...		args) noexcept
	{
		fprintf(stderr, format, args ...);
	}

	template <typename ... Args> void
	log(char const *const	format,
		Args const& ...		args) noexcept
	{
		fprintf(pyc_output, format, args ...);
	}

	template <typename ... Args> void
	logVerb(char const *const	format,
		Args const& ...		args) noexcept
	{
		fprintf(stderr, format, args ...);
	}



class PycModule {
public:
    PycModule() : m_maj(-1), m_min(-1), m_unicode(false) { }

    void loadFromFile(const char* filename);
    bool isValid() const { return (m_maj >= 0) && (m_min >= 0); }

    unsigned int majorVer() const { return m_maj; }
    unsigned int minorVer() const { return m_min; }

	// return how much current version is above or below the given one.
	// 0 => major & minor match
    int verCompare(int maj, int min) const
    {
		if (m_maj == maj)
			return m_min - min;
		else
			return m_maj - maj;
    }

    bool isUnicode() const { return m_unicode; }

    PycRef<PycCode> code() const { return m_code; }

    void intern(PycRef<PycString> str) { m_interns.push_back(str); }
    PycRef<PycString> getIntern(int ref) const;

    void refObject(PycRef<PycObject> str) { m_refs.push_back(str); }
    PycRef<PycObject> getRef(int ref) const;

private:
    void setVersion(unsigned int magic);

private:
    short m_maj, m_min;
    bool m_unicode;

	int build_magic;
	int timestamp;
	int sizeParameter;

    PycRef<PycCode> m_code;
    std::list<PycRef<PycString> > m_interns;
    std::list<PycRef<PycObject> > m_refs;
};

#endif
