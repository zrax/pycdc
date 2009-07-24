#ifndef _PYC_STRING_H
#define _PYC_STRING_H

#include "object.h"

class PycString : public PycObject {
public:
    PycString(int type = TYPE_STRING)
        : PycObject(type), m_value(0), m_length(0) { }

    ~PycString() { if (m_value) delete[] m_value; }

    bool isType(int type) const
    {
        return (type == TYPE_STRING) || (type == TYPE_INTERNED) ||
               (type == TYPE_STRINGREF) || PycObject::isType(type);
    }

    void load(class PycData* stream, class PycModule* mod);

    int length() const { return m_length; }
    const char* value() const { return m_value; }

private:
    char* m_value;
    int m_length;
};

#endif
