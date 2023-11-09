#ifndef _PYC_MODULE_H
#define _PYC_MODULE_H

#include "pyc_code.h"
#include <vector>

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
    MAGIC_3_5 = 0x0A0D0D16,
    MAGIC_3_5_3 = 0x0A0D0D17,
    MAGIC_3_6 = 0x0A0D0D33,
    MAGIC_3_7 = 0x0A0D0D42,
    MAGIC_3_8 = 0x0A0D0D55,
    MAGIC_3_9 = 0x0A0D0D61,
    MAGIC_3_10 = 0x0A0D0D6F,
    MAGIC_3_11 = 0x0A0D0DA7,
    MAGIC_3_12 = 0x0A0D0DCB,

    INVALID = 0,
};

class PycModule {
public:
    PycModule() : m_maj(-1), m_min(-1), m_unicode(false) { }

    void loadFromFile(const char* filename);
    void loadFromMarshalledFile(const char *filename, int major, int minor);
    bool isValid() const { return (m_maj >= 0) && (m_min >= 0); }

    int majorVer() const { return m_maj; }
    int minorVer() const { return m_min; }

    int verCompare(int maj, int min) const
    {
        if (m_maj == maj)
            return m_min - min;
        return m_maj - maj;
    }

    bool isUnicode() const { return m_unicode; }

    bool strIsUnicode() const
    {
        return (m_maj >= 3) || (m_code->flags() & PycCode::CO_FUTURE_UNICODE_LITERALS) != 0;
    }

    bool internIsBytes() const
    {
        return (m_maj < 3) && (m_code->flags() & PycCode::CO_FUTURE_UNICODE_LITERALS) != 0;
    }

    PycRef<PycCode> code() const { return m_code; }

    void intern(PycRef<PycString> str) { m_interns.emplace_back(std::move(str)); }
    PycRef<PycString> getIntern(int ref) const;

    void refObject(PycRef<PycObject> obj) { m_refs.emplace_back(std::move(obj)); }
    PycRef<PycObject> getRef(int ref) const;

    static bool isSupportedVersion(int major, int minor);

private:
    void setVersion(unsigned int magic);

private:
    int m_maj, m_min;
    bool m_unicode;

    PycRef<PycCode> m_code;
    std::vector<PycRef<PycString>> m_interns;
    std::vector<PycRef<PycObject>> m_refs;
};

#endif
