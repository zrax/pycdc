#ifndef _PYC_SEQUENCE_H
#define _PYC_SEQUENCE_H

#include "object.h"
#include <vector>
#include <list>

class PycTuple : public PycObject {
public:
    typedef std::vector<PycRef<PycObject> > value_t;

    PycTuple(int type = TYPE_TUPLE)
        : PycObject(type), m_size(0) { }

    void load(class PycData* stream, class PycModule* mod);

    int size() const { return m_size; }
    value_t values() const { return m_values; }

private:
    int m_size;
    value_t m_values;
};

class PycList : public PycObject {
public:
    typedef std::list<PycRef<PycObject> > value_t;

    PycList(int type = TYPE_LIST)
        : PycObject(type), m_size(0) { }

    void load(class PycData* stream, class PycModule* mod);

    int size() const { return m_size; }
    value_t values() const { return m_values; }

private:
    int m_size;
    value_t m_values;
};

#endif
