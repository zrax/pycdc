#include "pyc_sequence.h"
#include "pyc_module.h"
#include "data.h"
#include <stdexcept>

/* PycSimpleSequence */
void PycSimpleSequence::load(PycData* stream, PycModule* mod)
{
    m_size = stream->get32();
    m_values.reserve(m_size);
    for (int i=0; i<m_size; i++)
        m_values.push_back(LoadObject(stream, mod));
}

bool PycSimpleSequence::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycSimpleSequence> seqObj = obj.cast<PycSimpleSequence>();
    if (m_size != seqObj->m_size)
        return false;
    auto it1 = m_values.cbegin();
    auto it2 = seqObj->m_values.cbegin();
    while (it1 != m_values.cend()) {
        if (!(*it1)->isEqual(*it2))
            return false;
        ++it1, ++it2;
    }
    return true;
}


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

    auto ki1 = m_keys.cbegin();
    auto ki2 = dictObj->m_keys.cbegin();
    while (ki1 != m_keys.cend()) {
        if (!(*ki1)->isEqual(*ki2))
            return false;
        ++ki1, ++ki2;
    }

    auto vi1 = m_values.cbegin();
    auto vi2 = dictObj->m_values.cbegin();
    while (vi1 != m_values.cend()) {
        if (!(*vi1)->isEqual(*vi2))
            return false;
        ++vi1, ++vi2;
    }
    return true;
}

PycRef<PycObject> PycDict::get(PycRef<PycObject> key) const
{
    auto ki = m_keys.cbegin();
    auto vi = m_values.cbegin();
    while (ki != m_keys.cend()) {
        if ((*ki)->isEqual(key))
            return *vi;
        ++ki, ++vi;
    }
    return NULL; // Disassembly shouldn't get non-existent keys
}

PycRef<PycObject> PycDict::get(int idx) const
{
    if (idx < 0)
        throw std::out_of_range("Dict index out of range");

    auto it = m_values.cbegin();
    while (idx-- && it != m_values.cend())
        ++it;
    if (it == m_values.cend())
        throw std::out_of_range("Dict index out of range");
    return *it;
}
