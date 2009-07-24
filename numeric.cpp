#include "numeric.h"
#include "data.h"
#include "module.h"

PycObjRef Pyc_False = new PycObject(PycObject::TYPE_FALSE);
PycObjRef Pyc_True = new PycObject(PycObject::TYPE_TRUE);

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
