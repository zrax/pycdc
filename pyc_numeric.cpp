#include "pyc_numeric.h"
#include "pyc_module.h"
#include "data.h"
#include <cstring>

#ifdef _MSC_VER
#define snprintf sprintf_s
#endif

/* PycInt */
void PycInt::load(PycData* stream, PycModule*)
{
    m_value = stream->get32();
}


/* PycLong */
void PycLong::load(PycData* stream, PycModule*)
{
    if (type() == TYPE_INT64) {
        int lo = stream->get32();
        int hi = stream->get32();
        m_value.push_back((lo      ) & 0xFFFF);
        m_value.push_back((lo >> 16) & 0xFFFF);
        m_value.push_back((hi      ) & 0xFFFF);
        m_value.push_back((hi >> 16) & 0xFFFF);
        m_size = (hi & 0x80000000) != 0 ? -4 : 4;
    } else {
        m_size = stream->get32();
        int actualSize = m_size >= 0 ? m_size : -m_size;
        for (int i=0; i<actualSize; i++)
            m_value.push_back(stream->get16());
    }
}

bool PycLong::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj->type())
        return false;

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

std::string PycLong::repr() const
{
    // Longs are printed as hex, since it's easier (and faster) to convert
    // arbitrary-length integers to a power of two than an arbitrary base

    if (m_size == 0)
        return "0x0L";

    // Realign to 32 bits, since Python uses only 15
    std::list<unsigned> bits;
    std::list<int>::const_iterator bit;
    int shift = 0, temp = 0;
    for (bit = m_value.begin(); bit != m_value.end(); ++bit) {
        temp |= *bit << shift;
        shift += 15;
        if (shift >= 32) {
            bits.push_back(temp);
            shift -= 32;
            temp = *bit >> (15 - shift);
        }
    }
    if (temp)
        bits.push_back(temp);

    std::string accum;
    accum.resize(3 + (bits.size() * 8) + 2);
    char* aptr = &accum[0];

    if (m_size < 0)
        *aptr++ = '-';
    *aptr++ = '0';
    *aptr++ = 'x';

    std::list<unsigned>::const_reverse_iterator iter = bits.rbegin();
    aptr += snprintf(aptr, 9, "%X", *iter++);
    while (iter != bits.rend())
        aptr += snprintf(aptr, 9, "%08X", *iter++);
    *aptr++ = 'L';
    *aptr++ = 0;
    return accum;
}


/* PycFloat */
void PycFloat::load(PycData* stream, PycModule*)
{
    int len = stream->getByte();
    delete[] m_value;
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
    if (type() != obj->type())
        return false;

    PycRef<PycFloat> floatObj = obj.cast<PycFloat>();
    if (m_value == floatObj->m_value)
        return true;
    return (strcmp(m_value, floatObj->m_value) == 0);
}


/* PycComplex */
void PycComplex::load(PycData* stream, PycModule* mod)
{
    PycFloat::load(stream, mod);

    int len = stream->getByte();
    delete[] m_imag;
    if (len > 0) {
        m_imag = new char[len+1];
        stream->getBuffer(len, m_imag);
        m_imag[len] = 0;
    } else {
        m_imag = 0;
    }
}

bool PycComplex::isEqual(PycRef<PycObject> obj) const
{
    if (!PycFloat::isEqual(obj))
        return false;

    PycRef<PycComplex> floatObj = obj.cast<PycComplex>();
    if (m_imag == floatObj->m_imag)
        return true;
    return (strcmp(m_imag, floatObj->m_imag) == 0);
}


/* PycCFloat */
void PycCFloat::load(PycData* stream, PycModule*)
{
    m_value_i64 = stream->get64();
}


/* PycCComplex */
void PycCComplex::load(PycData* stream, PycModule* mod)
{
    PycCFloat::load(stream, mod);
    m_imag_i64 = stream->get64();
}
