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

    bool isType(int type) const
    { return (type == TYPE_TUPLE) || PycObject::isType(type); }

    bool isEqual(PycRef<PycObject> obj) const;

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

    bool isType(int type) const
    { return (type == TYPE_LIST) || PycObject::isType(type); }

    bool isEqual(PycRef<PycObject> obj) const;

    void load(class PycData* stream, class PycModule* mod);

    int size() const { return m_size; }
    value_t values() const { return m_values; }

private:
    int m_size;
    value_t m_values;
};

class PycDict : public PycObject {
public:
    typedef std::list<PycRef<PycObject> > key_t;
    typedef std::list<PycRef<PycObject> > value_t;

    PycDict(int type = TYPE_DICT)
        : PycObject(type), m_size(0) { }

    bool isType(int type) const
    { return (type == TYPE_DICT) || PycObject::isType(type); }

    bool isEqual(PycRef<PycObject> obj) const;

    void load(class PycData* stream, class PycModule* mod);

    int size() const { return m_size; }
    PycRef<PycObject> get(PycRef<PycObject> key) const;
    key_t keys() const { return m_keys; }
    value_t values() const { return m_values; }

private:
    int m_size;
    key_t m_keys;
    value_t m_values;
};

#endif
