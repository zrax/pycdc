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

bool PycString::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj->type())
        return false;

    PycRef<PycString> strObj = obj.cast<PycString>();
    if (m_value == strObj->m_value)
        return true;
    return (strcmp(m_value, strObj->m_value) == 0);
}


void OutputString(PycRef<PycString> str, QuoteStyle style, FILE* F)
{
    const char* ch = str->value();
    int len = str->length();
    if (ch == 0)
        return;
    while (len--) {
        if (*ch < 0x20 || *ch == 0x7F) {
            if (*ch == '\r') {
                fprintf(F, "\\r");
            } else if (*ch == '\n') {
                if (style == QS_BlockSingle || style == QS_BlockDouble)
                    fputc('\n', F);
                else
                    fprintf(F, "\\n");
            } else if (*ch == '\t') {
                fprintf(F, "\\t");
            } else {
                fprintf(F, "\\x%x", *ch);
            }
        } else if (*ch >= 0x80) {
            if (str->type() == PycObject::TYPE_UNICODE) {
                // Unicode stored as UTF-8...  Let the stream interpret it
                fputc(*ch, F);
            } else {
                fprintf(F, "\\x%x", *ch);
            }
        } else {
            if (style == QS_Single && *ch == '\'')
                fprintf(F, "\\'");
            else if (style == QS_Double && *ch == '"')
                fprintf(F, "\\\"");
            else
                fputc(*ch, F);
        }
        ch++;
    }
}
