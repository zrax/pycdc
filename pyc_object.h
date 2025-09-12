#ifndef PYC_OBJECT_H
#define PYC_OBJECT_H

#include <typeinfo>

template <class Obj> class PycRef {
public:
    PycRef() noexcept : m_obj() { }

    PycRef(Obj* obj) noexcept : m_obj(obj)
    {
        if (m_obj)
            m_obj->addRef();
    }

    PycRef(const PycRef<Obj>& obj) noexcept : m_obj(obj.m_obj)
    {
        if (m_obj)
            m_obj->addRef();
    }

    PycRef(PycRef<Obj>&& obj) noexcept : m_obj(obj.m_obj)
    {
        obj.m_obj = nullptr;
    }

    ~PycRef()
    {
        if (m_obj)
            m_obj->delRef();
    }

    PycRef<Obj>& operator=(Obj* obj)
    {
        if (obj)
            obj->addRef();
        if (m_obj)
            m_obj->delRef();
        m_obj = obj;
        return *this;
    }

    PycRef<Obj>& operator=(const PycRef<Obj>& obj)
    {
        if (this == &obj) {
            return *this;
        }

        if (obj.m_obj)
            obj.m_obj->addRef();
        if (m_obj)
            m_obj->delRef();
        m_obj = obj.m_obj;
        return *this;
    }

    PycRef<Obj>& operator=(PycRef<Obj>&& obj) noexcept
    {
        if (this == &obj) {
            return *this;
        }

        m_obj = obj.m_obj;
        obj.m_obj = nullptr;
        return *this;
    }

    bool operator==(Obj* obj) const { return m_obj == obj; }
    bool operator==(const PycRef<Obj>& obj) const { return m_obj == obj.m_obj; }
    bool operator!=(Obj* obj) const { return m_obj != obj; }
    bool operator!=(const PycRef<Obj>& obj) const { return m_obj != obj.m_obj; }

    Obj& operator*() const { return *m_obj; }
    Obj* operator->() const { return m_obj; }
    operator Obj*() const { return m_obj; }

    inline int type() const;

    template <class Cast> PycRef<Cast> try_cast() const
    {
        return dynamic_cast<Cast*>(m_obj);
    }

    template <class Cast> PycRef<Cast> cast() const
    {
        Cast* result = dynamic_cast<Cast*>(m_obj);
        if (!result)
            throw std::bad_cast();
        return result;
    }

    bool isIdent(const Obj* obj) const { return m_obj == obj; }

private:
    Obj* m_obj;
};

class PycData;
class PycModule;

/* Please only hold PycObjects inside PycRefs! */
class PycObject {
public:
    enum Type {
        // From the Python Marshallers
        TYPE_NULL = '0',                    // Python 1.0 ->
        TYPE_NONE = 'N',                    // Python 1.0 ->
        TYPE_FALSE = 'F',                   // Python 2.3 ->
        TYPE_TRUE = 'T',                    // Python 2.3 ->
        TYPE_STOPITER = 'S',                // Python 2.2 ->
        TYPE_ELLIPSIS = '.',                // Python 1.4 ->
        TYPE_INT = 'i',                     // Python 1.0 ->
        TYPE_INT64 = 'I',                   // Python 1.5 - 3.3
        TYPE_FLOAT = 'f',                   // Python 1.0 ->
        TYPE_BINARY_FLOAT = 'g',            // Python 2.5 ->
        TYPE_COMPLEX = 'x',                 // Python 1.4 ->
        TYPE_BINARY_COMPLEX = 'y',          // Python 2.5 ->
        TYPE_LONG = 'l',                    // Python 1.0 ->
        TYPE_STRING = 's',                  // Python 1.0 ->
        TYPE_INTERNED = 't',                // Python 2.4 - 2.7, 3.4 ->
        TYPE_STRINGREF = 'R',               // Python 2.4 - 2.7
        TYPE_OBREF = 'r',                   // Python 3.4 ->
        TYPE_TUPLE = '(',                   // Python 1.0 ->
        TYPE_LIST = '[',                    // Python 1.0 ->
        TYPE_DICT = '{',                    // Python 1.0 ->
        TYPE_CODE = 'c',                    // Python 1.3 ->
        TYPE_CODE2 = 'C',                   // Python 1.0 - 1.2
        TYPE_UNICODE = 'u',                 // Python 1.6 ->
        TYPE_UNKNOWN = '?',                 // Python 1.0 ->
        TYPE_SET = '<',                     // Python 2.5 ->
        TYPE_FROZENSET = '>',               // Python 2.5 ->
        TYPE_ASCII = 'a',                   // Python 3.4 ->
        TYPE_ASCII_INTERNED = 'A',          // Python 3.4 ->
        TYPE_SMALL_TUPLE = ')',             // Python 3.4 ->
        TYPE_SHORT_ASCII = 'z',             // Python 3.4 ->
        TYPE_SHORT_ASCII_INTERNED = 'Z',    // Python 3.4 ->
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

protected:
    int m_type;

public:
    void addRef() { ++m_refs; }
    void delRef()
    {
        if (--m_refs == 0)
            delete this;
    }
};

template <class Obj> int PycRef<Obj>::type() const
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
