#ifndef _PYC_OBJECT_H
#define _PYC_OBJECT_H

#include <typeinfo>

template <class _Obj>
class PycRef {
public:
    PycRef() noexcept : m_obj() { }

    PycRef(_Obj* obj) noexcept : m_obj(obj)
    {
        if (m_obj)
            m_obj->addRef();
    }

    PycRef(const PycRef<_Obj>& obj) noexcept : m_obj(obj.m_obj)
    {
        if (m_obj)
            m_obj->addRef();
    }

    PycRef(PycRef<_Obj>&& obj) noexcept : m_obj(obj.m_obj)
    {
        obj.m_obj = nullptr;
    }

    ~PycRef<_Obj>()
    {
        if (m_obj)
            m_obj->delRef();
    }

    PycRef<_Obj>& operator=(_Obj* obj)
    {
        if (obj)
            obj->addRef();
        if (m_obj)
            m_obj->delRef();
        m_obj = obj;
        return *this;
    }

    PycRef<_Obj>& operator=(const PycRef<_Obj>& obj)
    {
        if (obj.m_obj)
            obj.m_obj->addRef();
        if (m_obj)
            m_obj->delRef();
        m_obj = obj.m_obj;
        return *this;
    }

    PycRef<_Obj>& operator=(PycRef<_Obj>&& obj) noexcept
    {
        m_obj = obj.m_obj;
        obj.m_obj = nullptr;
        return *this;
    }

    bool operator==(_Obj* obj) const { return m_obj == obj; }
    bool operator==(const PycRef<_Obj>& obj) const { return m_obj == obj.m_obj; }
    bool operator!=(_Obj* obj) const { return m_obj != obj; }
    bool operator!=(const PycRef<_Obj>& obj) const { return m_obj != obj.m_obj; }

    _Obj& operator*() const { return *m_obj; }
    _Obj* operator->() const { return m_obj; }
    operator _Obj*() const { return m_obj; }

    inline int type() const;

    template <class _Cast>
    PycRef<_Cast> cast() const { return dynamic_cast<_Cast*>(m_obj); }

    template <class _Cast>
    PycRef<_Cast> require_cast() const
    {
        _Cast* result = dynamic_cast<_Cast*>(m_obj);
        if (!result)
            throw std::bad_cast();
        return result;
    }

    bool isIdent(const _Obj* obj) const { return m_obj == obj; }

private:
    _Obj* m_obj;
};


class PycData;
class PycModule;

/* Please only hold PycObjects inside PycRefs! */
class PycObject {
public:
    enum Type {
        // From the Python Marshallers
        TYPE_NULL = '0',
        TYPE_NONE = 'N',
        TYPE_FALSE = 'F',
        TYPE_TRUE = 'T',
        TYPE_STOPITER = 'S',
        TYPE_ELLIPSIS = '.',
        TYPE_INT = 'i',
        TYPE_INT64 = 'I',
        TYPE_FLOAT = 'f',
        TYPE_BINARY_FLOAT = 'g',
        TYPE_COMPLEX = 'x',
        TYPE_BINARY_COMPLEX = 'y',
        TYPE_LONG = 'l',
        TYPE_STRING = 's',
        TYPE_INTERNED = 't',
        TYPE_STRINGREF = 'R',
        TYPE_OBREF = 'r',
        TYPE_TUPLE = '(',
        TYPE_LIST = '[',
        TYPE_DICT = '{',
        TYPE_CODE = 'c',
        TYPE_CODE2 = 'C',   // Used in Python 1.0 - 1.2
        TYPE_UNICODE = 'u',
        TYPE_UNKNOWN = '?',
        TYPE_SET = '<',
        TYPE_FROZENSET = '>',
        TYPE_ASCII = 'a',
        TYPE_ASCII_INTERNED = 'A',
        TYPE_SMALL_TUPLE = ')',
        TYPE_SHORT_ASCII = 'z',
        TYPE_SHORT_ASCII_INTERNED = 'Z',
    };

    PycObject(int type = TYPE_UNKNOWN) : m_refs(0), m_type(type) { }
    virtual ~PycObject() { }

    int type() const { return m_type; }

    virtual bool isEqual(PycRef<PycObject> obj) const
    {
        return obj.isIdent(this);
    }

    virtual void load(PycData*, PycModule*) { }

private:
    int m_refs;
    int m_type;

public:
    void addRef() { ++m_refs; }
    void delRef() { if (--m_refs == 0) delete this; }
};

template <class _Obj>
int PycRef<_Obj>::type() const
{
    return m_obj ? m_obj->type() : PycObject::TYPE_NULL;
}

PycRef<PycObject> CreateObject(int type);
PycRef<PycObject> LoadObject(PycData* stream, PycModule* mod);

/* Static Singleton objects */
extern PycRef<PycObject> Pyc_None;
extern PycRef<PycObject> Pyc_Ellipsis;
extern PycRef<PycObject> Pyc_StopIteration;
extern PycRef<PycObject> Pyc_False;
extern PycRef<PycObject> Pyc_True;

#endif
