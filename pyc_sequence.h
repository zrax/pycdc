#ifndef _PYC_SEQUENCE_H
#define _PYC_SEQUENCE_H

#include "pyc_object.h"
#include <vector>

class PycSequence : public PycObject {
public:
    PycSequence(int type) : PycObject(type), m_size(0) { }

    int size() const { return m_size; }
    virtual PycRef<PycObject> get(int idx) const = 0;

protected:
    int m_size;
};

class PycSimpleSequence : public PycSequence {
public:
    typedef std::vector<PycRef<PycObject>> value_t;

    PycSimpleSequence(int type) : PycSequence(type) { }

    bool isEqual(PycRef<PycObject> obj) const override;

    void load(class PycData* stream, class PycModule* mod) override;

    const value_t& values() const { return m_values; }
    PycRef<PycObject> get(int idx) const override { return m_values.at(idx); }

protected:
    value_t m_values;
};

class PycTuple : public PycSimpleSequence {
public:
    typedef PycSimpleSequence::value_t value_t;
    PycTuple(int type = TYPE_TUPLE) : PycSimpleSequence(type) { }

    void load(class PycData* stream, class PycModule* mod) override;
};

class PycList : public PycSimpleSequence {
public:
    typedef PycSimpleSequence::value_t value_t;
    PycList(int type = TYPE_LIST) : PycSimpleSequence(type) { }
};

class PycDict : public PycSequence {
public:
    typedef std::vector<PycRef<PycObject>> key_t;
    typedef std::vector<PycRef<PycObject>> value_t;

    PycDict(int type = TYPE_DICT) : PycSequence(type) { }

    bool isEqual(PycRef<PycObject> obj) const override;

    void load(class PycData* stream, class PycModule* mod) override;

    PycRef<PycObject> get(PycRef<PycObject> key) const;
    const key_t& keys() const { return m_keys; }
    const value_t& values() const { return m_values; }

    PycRef<PycObject> get(int idx) const override;

private:
    key_t m_keys;
    value_t m_values;
};

class PycSet : public PycSimpleSequence {
public:
    typedef PycSimpleSequence::value_t value_t;
    PycSet(int type = TYPE_SET) : PycSimpleSequence(type) { }
};

#endif
