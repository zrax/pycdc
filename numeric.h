#ifndef _PYC_NUMERIC_H
#define _PYC_NUMERIC_H

#include "object.h"
#include <list>

class PycInt : public PycObject {
public:
    PycInt(int value = 0, int type = TYPE_INT)
        : PycObject(type), m_value(value) { }

    bool isType(int type) const
    { return (type == TYPE_INT) || PycObject::isType(type); }

    void load(class PycData* stream, class PycModule* mod);

    int value() const { return m_value; }

private:
    int m_value;
};

class PycLong : public PycObject {
public:
    PycLong(int type = TYPE_LONG)
        : PycObject(type), m_size(0) { }

    bool isType(int type) const
    { return (type == TYPE_LONG) || PycObject::isType(type); }

    void load(class PycData* stream, class PycModule* mod);

    int size() const { return m_size; }
    std::list<int> value() const { return m_value; }

private:
    int m_size;
    std::list<int> m_value;
};

class PycFloat : public PycObject {
public:
    PycFloat(int type = TYPE_FLOAT)
        : PycObject(type), m_value(0) { }

    ~PycFloat() { if (m_value) delete[] m_value; }

    bool isType(int type) const
    { return (type == TYPE_FLOAT) || PycObject::isType(type); }

    void load(class PycData* stream, class PycModule* mod);

    const char* value() const { return m_value; }

private:
    char* m_value;  // Floats are stored as strings
};

/* Static Singleton objects */
extern PycObjRef Pyc_False;
extern PycObjRef Pyc_True;

#endif
