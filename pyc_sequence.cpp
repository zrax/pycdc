#include "pyc_sequence.h"
#include "pyc_module.h"
#include "data.h"
#include <stdexcept>

/* PycTuple */
void PycTuple::load(PycData* stream, PycModule* mod)
{
    if (type() == TYPE_SMALL_TUPLE)
        m_size = stream->getByte();
    else
        m_size = stream->get32();

    m_values.resize(m_size);
    for (int i=0; i<m_size; i++)
        m_values[i] = LoadObject(stream, mod);
}

bool PycTuple::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycTuple> tupleObj = obj.cast<PycTuple>();
    if (m_size != tupleObj->m_size)
        return false;
    value_t::const_iterator it1 = m_values.begin();
    value_t::const_iterator it2 = tupleObj->m_values.begin();
    while (it1 != m_values.end()) {
        if (!(*it1)->isEqual(*it2))
            return false;
        ++it1, ++it2;
    }
    return true;
}


/* PycList */
void PycList::load(PycData* stream, PycModule* mod)
{
    m_size = stream->get32();
    for (int i=0; i<m_size; i++)
        m_values.push_back(LoadObject(stream, mod));
}

bool PycList::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycList> listObj = obj.cast<PycList>();
    if (m_size != listObj->m_size)
        return false;
    value_t::const_iterator it1 = m_values.begin();
    value_t::const_iterator it2 = listObj->m_values.begin();
    while (it1 != m_values.end()) {
        if (!(*it1)->isEqual(*it2))
            return false;
        ++it1, ++it2;
    }
    return true;
}

PycRef<PycObject> PycList::get(int idx) const
{
    if (idx < 0)
        throw std::out_of_range("List index out of range");

    value_t::const_iterator it = m_values.begin();
    while (idx-- && it != m_values.end())
        ++it;
    if (it == m_values.end())
        throw std::out_of_range("List index out of range");
    return *it;
}



/* PycDict */
void PycDict::load(PycData* stream, PycModule* mod)
{
    PycRef<PycObject> key, val;
    for (;;) {
        key = LoadObject(stream, mod);
        if (key == NULL)
            break;
        val = LoadObject(stream, mod);
        m_keys.push_back(key);
        m_values.push_back(val);
    }
}

bool PycDict::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycDict> dictObj = obj.cast<PycDict>();
    if (m_size != dictObj->m_size)
        return false;

    key_t::const_iterator ki1 = m_keys.begin();
    key_t::const_iterator ki2 = dictObj->m_keys.begin();
    while (ki1 != m_keys.end()) {
        if (!(*ki1)->isEqual(*ki2))
            return false;
        ++ki1, ++ki2;
    }

    value_t::const_iterator vi1 = m_values.begin();
    value_t::const_iterator vi2 = dictObj->m_values.begin();
    while (vi1 != m_values.end()) {
        if (!(*vi1)->isEqual(*vi2))
            return false;
        ++vi1, ++vi2;
    }
    return true;
}

PycRef<PycObject> PycDict::get(PycRef<PycObject> key) const
{
    key_t::const_iterator ki = m_keys.begin();
    value_t::const_iterator vi = m_values.begin();
    while (ki != m_keys.end()) {
        if ((*ki)->isEqual(key))
            return *vi;
        ++ki, ++vi;
    }
    return NULL; // Disassembly shouldn't get non-existant keys
}

PycRef<PycObject> PycDict::get(int idx) const
{
    if (idx < 0)
        throw std::out_of_range("Dict index out of range");

    value_t::const_iterator it = m_values.begin();
    while (idx-- && it != m_values.end())
        ++it;
    if (it == m_values.end())
        throw std::out_of_range("Dict index out of range");
    return *it;
}


/* PycSet */
void PycSet::load(PycData* stream, PycModule* mod)
{
    m_size = stream->get32();
    for (int i=0; i<m_size; i++)
        m_values.insert(LoadObject(stream, mod));
}

bool PycSet::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycSet> setObj = obj.cast<PycSet>();
    if (m_size != setObj->m_size)
        return false;
    value_t::const_iterator it1 = m_values.begin();
    value_t::const_iterator it2 = setObj->m_values.begin();
    while (it1 != m_values.end()) {
        if (!(*it1)->isEqual(*it2))
            return false;
        ++it1, ++it2;
    }
    return true;
}

PycRef<PycObject> PycSet::get(int idx) const
{
    if (idx < 0)
        throw std::out_of_range("Set index out of range");

    value_t::const_iterator it = m_values.begin();
    while (idx-- && it != m_values.end())
        ++it;
    if (it == m_values.end())
        throw std::out_of_range("Set index out of range");
    return *it;
}
