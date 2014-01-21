#ifndef _PYC_STRING_H
#define _PYC_STRING_H

#include "pyc_object.h"
#include "data.h"
#include <cstdio>

class PycString : public PycObject {
public:
    PycString(int type = TYPE_STRING)
        : PycObject(type), m_value(0), m_length(0) { }

    ~PycString() { delete[] m_value; }

    bool isEqual(PycRef<PycObject> obj) const;
    bool isEqual(const char* str) const;

    void load(class PycData* stream, class PycModule* mod);

    int length() const { return m_length; }
    const char* value() const { return m_value; }

private:
    char* m_value;
    int m_length;
};

void OutputString(PycRef<PycString> str, char prefix = 0,
                  bool triple = false, FILE* F = pyc_output);

#endif
