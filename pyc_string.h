#ifndef _PYC_STRING_H
#define _PYC_STRING_H

#include "pyc_object.h"
#include "data.h"
#include <cstdio>
#include <string>

class PycString : public PycObject {
public:
    PycString(int type = TYPE_STRING)
        : PycObject(type) { }

    bool isEqual(PycRef<PycObject> obj) const override;
    bool isEqual(const std::string& str) const { return m_value == str; }

    void load(class PycData* stream, class PycModule* mod) override;

    int length() const { return (int)m_value.size(); }
    const char* value() const { return m_value.c_str(); }

private:
    std::string m_value;
};

void OutputString(PycRef<PycString> str, char prefix = 0,
                  bool triple = false, FILE* F = pyc_output);

#endif
