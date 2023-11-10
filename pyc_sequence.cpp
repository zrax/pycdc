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
        m_values.emplace_back(std::make_tuple(key, val));
    }
}

bool PycDict::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycDict> dictObj = obj.cast<PycDict>();
    if (m_values.size() != dictObj->m_values.size())
        return false;

    auto it1 = m_values.cbegin();
    auto it2 = dictObj->m_values.cbegin();
    while (it1 != m_values.cend()) {
        if (!std::get<0>(*it1)->isEqual(std::get<0>(*it2)))
            return false;
        if (!std::get<1>(*it1)->isEqual(std::get<1>(*it2)))
            return false;
        ++it1, ++it2;
    }
    return true;
}
