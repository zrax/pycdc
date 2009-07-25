#ifndef _PYC_STRING_H
#define _PYC_STRING_H

#include "object.h"
#include <cstdio>

enum QuoteStyle {
    QS_Single, QS_Double, QS_BlockSingle, QS_BlockDouble
};

class PycString : public PycObject {
public:
    PycString(int type = TYPE_STRING)
        : PycObject(type), m_value(0), m_length(0) { }

    ~PycString() { if (m_value) delete[] m_value; }

    bool isEqual(PycRef<PycObject> obj) const;

    void load(class PycData* stream, class PycModule* mod);

    int length() const { return m_length; }
    const char* value() const { return m_value; }

private:
    char* m_value;
    int m_length;
};

void OutputString(PycRef<PycString> str, QuoteStyle style, FILE* F = stdout);

#endif
