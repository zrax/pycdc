#ifndef _PYC_SEQUENCE_H
#define _PYC_SEQUENCE_H

#include "pyc_object.h"
#include <vector>
#include <list>
#include <set>

class PycSequence : public PycObject {
public:
    PycSequence(int type) : PycObject(type), m_size(0) { }

    int size() const { return m_size; }
    virtual PycRef<PycObject> get(int idx) const = 0;

protected:
    int m_size;
};

class PycTuple : public PycSequence {
public:
    typedef std::vector<PycRef<PycObject> > value_t;

    PycTuple(int type = TYPE_TUPLE) : PycSequence(type) { }

    bool isEqual(PycRef<PycObject> obj) const;

    void load(class PycData* stream, class PycModule* mod);

    const value_t& values() const { return m_values; }
    PycRef<PycObject> get(int idx) const { return m_values[idx]; }

private:
    value_t m_values;
};

class PycList : public PycSequence {
public:
    typedef std::list<PycRef<PycObject> > value_t;

    PycList(int type = TYPE_LIST) : PycSequence(type) { }

    bool isEqual(PycRef<PycObject> obj) const;

    void load(class PycData* stream, class PycModule* mod);

    const value_t& values() const { return m_values; }
    PycRef<PycObject> get(int idx) const
    {
        value_t::const_iterator it = m_values.begin();
        for (int i=0; i<idx; i++) ++it;
        return *it;
    }

private:
    value_t m_values;
};

class PycDict : public PycSequence {
public:
    typedef std::list<PycRef<PycObject> > key_t;
    typedef std::list<PycRef<PycObject> > value_t;

    PycDict(int type = TYPE_DICT) : PycSequence(type) { }

    bool isEqual(PycRef<PycObject> obj) const;

    void load(class PycData* stream, class PycModule* mod);

    PycRef<PycObject> get(PycRef<PycObject> key) const;
    const key_t& keys() const { return m_keys; }
    const value_t& values() const { return m_values; }

    PycRef<PycObject> get(int idx) const
    {
        value_t::const_iterator it = m_values.begin();
        for (int i=0; i<idx; i++) ++it;
        return *it;
    }

private:
    key_t m_keys;
    value_t m_values;
};

class PycSet : public PycSequence {
public:
    typedef std::set<PycRef<PycObject> > value_t;

    PycSet(int type = TYPE_SET) : PycSequence(type) { }

    bool isEqual(PycRef<PycObject> obj) const;

    void load(class PycData* stream, class PycModule* mod);

    const value_t& values() const { return m_values; }
    PycRef<PycObject> get(int idx) const
    {
        value_t::const_iterator it = m_values.begin();
        for (int i=0; i<idx; i++) ++it;
        return *it;
    }

private:
    value_t m_values;
};

#endif
