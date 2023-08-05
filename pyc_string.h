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

    bool startsWith(const std::string& str) const
    {
        return m_value.substr(0, str.size()) == str;
    }

    void load(class PycData* stream, class PycModule* mod) override;

    int length() const { return (int)m_value.size(); }
    const char* value() const { return m_value.c_str(); }
    const std::string &strValue() const { return m_value; }

    void setValue(std::string str) { m_value = std::move(str); }

    void print(std::ostream& stream, class PycModule* mod, bool triple = false,
               const char* parent_f_string_quote = nullptr);

private:
    std::string m_value;
};

#endif
