#ifndef _PYC_MODULE_H
#define _PYC_MODULE_H

#include "pyc_code.h"
#include <list>

enum PycMagic {
    MAGIC_1_0 = 0x00999902,
    MAGIC_1_1 = 0x00999903, /* Also covers 1.2 */
    MAGIC_1_3 = 0x0A0D2E89,
    MAGIC_1_4 = 0x0A0D1704,
    MAGIC_1_5 = 0x0A0D4E99,
    MAGIC_1_6 = 0x0A0DC4FC,

    MAGIC_2_0 = 0x0A0DC687,
    MAGIC_2_1 = 0x0A0DEB2A,
    MAGIC_2_2 = 0x0A0DED2D,
    MAGIC_2_3 = 0x0A0DF23B,
    MAGIC_2_4 = 0x0A0DF26D,
    MAGIC_2_5 = 0x0A0DF2B3,
    MAGIC_2_6 = 0x0A0DF2D1,
    MAGIC_2_7 = 0x0A0DF303,

    MAGIC_3_0 = 0x0A0D0C3A,
    MAGIC_3_1 = 0x0A0D0C4E,
    MAGIC_3_2 = 0x0A0D0C6C,
    MAGIC_3_3 = 0x0A0D0C9E,
    MAGIC_3_4 = 0x0A0D0CEE,
};

#define PYC_VERSION(maj, min)  MAGIC_##maj##_##min

class PycModule {
public:
    PycModule() : m_maj(-1), m_min(-1), m_unicode(false) { }

    void loadFromFile(const char* filename);
    bool isValid() const { return (m_maj >= 0) && (m_min >= 0); }

    unsigned int majorVer() const { return m_maj; }
    unsigned int minorVer() const { return m_min; }

    int verCompare(int maj, int min) const
    {
        if (m_maj == maj)
            return m_min - min;
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

    PycRef<PycCode> m_code;
    std::list<PycRef<PycString> > m_interns;
    std::list<PycRef<PycObject> > m_refs;
};

#endif
