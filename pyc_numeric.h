#ifndef _PYC_NUMERIC_H
#define _PYC_NUMERIC_H

#include "pyc_object.h"
#include "data.h"
#include <vector>
#include <string>

class PycInt : public PycObject {
public:
    PycInt(int _value = 0, int _type = TYPE_INT)
        : PycObject(_type), m_value(_value) { }

    bool isEqual(PycRef<PycObject> obj) const override
    {
        return (type() == obj.type()) &&
               (m_value == obj.cast<PycInt>()->m_value);
    }

    void load(class PycData* stream, class PycModule* mod) override;

    int value() const { return m_value; }

private:
    int m_value;
};

class PycLong : public PycObject {
public:
    PycLong(int _type = TYPE_LONG)
        : PycObject(_type), m_size(0) { }

    bool isEqual(PycRef<PycObject> obj) const override;

    void load(class PycData* stream, class PycModule* mod) override;

    int size() const { return m_size; }
    const std::vector<int>& value() const { return m_value; }

    std::string repr() const;

private:
    int m_size;
    std::vector<int> m_value;
};

class PycFloat : public PycObject {
public:
    PycFloat(int _type = TYPE_FLOAT)
        : PycObject(_type) { }

    bool isEqual(PycRef<PycObject> obj) const override;

    void load(class PycData* stream, class PycModule* mod) override;

    const char* value() const { return m_value.c_str(); }

private:
    std::string m_value;  // Floats are stored as strings
};

class PycComplex : public PycFloat {
public:
    PycComplex(int _type = TYPE_COMPLEX)
        : PycFloat(_type) { }

    bool isEqual(PycRef<PycObject> obj) const override;

    void load(class PycData* stream, class PycModule* mod) override;

    const char* imag() const { return m_imag.c_str(); }

private:
    std::string m_imag;
};

class PycCFloat : public PycObject {
public:
    PycCFloat(int _type = TYPE_BINARY_FLOAT)
        : PycObject(_type), m_value(0.0) { }

    bool isEqual(PycRef<PycObject> obj) const override
    {
        return (type() == obj.type()) &&
               (m_value == obj.cast<PycCFloat>()->m_value);
    }

    void load(class PycData* stream, class PycModule* mod) override;

    double value() const { return m_value; }

private:
    double m_value;
};

class PycCComplex : public PycCFloat {
public:
    PycCComplex(int _type = TYPE_BINARY_COMPLEX)
        : PycCFloat(_type), m_imag(0.0) { }

    bool isEqual(PycRef<PycObject> obj) const override
    {
        return (PycCFloat::isEqual(obj)) &&
               (m_imag == obj.cast<PycCComplex>()->m_imag);
    }

    void load(class PycData* stream, class PycModule* mod) override;

    double imag() const { return m_imag; }

private:
    double m_imag;
};

#endif
