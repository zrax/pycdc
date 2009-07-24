#include "sequence.h"
#include "data.h"
#include "module.h"

/* PycTuple */
void PycTuple::load(PycData* stream, PycModule* mod)
{
    m_size = stream->get32();
    m_values.resize(m_size);
    for (int i=0; i<m_size; i++)
        m_values[i] = LoadObject(stream, mod);
}


/* PycList */
void PycList::load(PycData* stream, PycModule* mod)
{
    m_size = stream->get32();
    for (int i=0; i<m_size; i++)
        m_values.push_back(LoadObject(stream, mod));
}
