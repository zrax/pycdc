#include "numeric.h"
#include "data.h"
#include "module.h"
#include <cstring>

/* PycInt */
void PycInt::load(PycData* stream, PycModule*)
{
    m_value = stream->get32();
}


/* PycLong */
void PycLong::load(PycData* stream, PycModule*)
{
    m_size = stream->get32();
    int actualSize = m_size & 0x7FFFFFFF;
    for (int i=0; i<actualSize; i++)
        m_value.push_back(stream->get16());
}

bool PycLong::isEqual(PycRef<PycObject> obj) const
{
    PycRef<PycLong> longObj = obj.cast<PycLong>();
    if (m_size != longObj->m_size)
        return false;
    std::list<int>::const_iterator it1 = m_value.begin();
    std::list<int>::const_iterator it2 = longObj->m_value.begin();
    while (it1 != m_value.end()) {
        if (*it1 != *it2)
            return false;
        ++it1, ++it2;
    }
    return true;
}


/* PycFloat */
void PycFloat::load(PycData* stream, PycModule*)
{
    int len = stream->getByte();
    if (m_value) delete[] m_value;
    if (len > 0) {
        m_value = new char[len+1];
        stream->getBuffer(len, m_value);
        m_value[len] = 0;
    } else {
        m_value = 0;
    }
}

bool PycFloat::isEqual(PycRef<PycObject> obj) const
{
    PycRef<PycFloat> floatObj = obj.cast<PycFloat>();
    if (m_value == floatObj->m_value)
        return true;
    return (strcmp(m_value, floatObj->m_value) == 0);
}
