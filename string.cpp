#include "string.h"
#include "data.h"
#include "module.h"
#include <cstring>

/* PycString */
void PycString::load(PycData* stream, PycModule* mod)
{
    if (m_value) delete[] m_value;

    if (type() == TYPE_STRINGREF) {
        PycRef<PycString> str = mod->getIntern(stream->get32());
        m_length = str->length();
        if (m_length) {
            m_value = new char[m_length+1];
            memcpy(m_value, str->value(), m_length);
            m_value[m_length] = 0;
        } else {
            m_value = 0;
        }
    } else {
        m_length = stream->get32();
        if (m_length) {
            m_value = new char[m_length+1];
            stream->getBuffer(m_length, m_value);
            m_value[m_length] = 0;
        } else {
            m_value = 0;
        }

        if (type() == TYPE_INTERNED)
            mod->intern(this);
    }
}
