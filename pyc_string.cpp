#include "pyc_string.h"
#include "pyc_module.h"
#include "data.h"
#include <cstring>

static void ascii_to_utf8(char** data)
{
    size_t utf8len = 0, asciilen = 0;
    unsigned char* cp = reinterpret_cast<unsigned char*>(*data);
    while (*cp) {
        if (*cp & 0x80)
            utf8len += 2;
        else
            utf8len += 1;

        // Advance ASCII pointer
        ++asciilen;
        ++cp;
    }

    if (asciilen == utf8len) {
        // This can only happen if all characters are [0x00-0x7f].
        // If that happens, we don't need to do any conversion, nor
        // reallocate any buffers.  Woot!
        return;
    }

    char* utf8_buffer = new char[utf8len + 1];
    unsigned char* up = reinterpret_cast<unsigned char*>(utf8_buffer);
    cp = reinterpret_cast<unsigned char*>(*data);
    while (*cp) {
        if (*cp & 0x80) {
            *up++ = 0xC0 | ((*cp >> 6) & 0x1F);
            *up++ = 0x80 | ((*cp     ) & 0x3F);
        } else {
            *up++ = *cp;
        }
        ++cp;
    }

    utf8_buffer[utf8len] = 0;
    delete[] *data;
    *data = utf8_buffer;
}

/* PycString */
void PycString::load(PycData* stream, PycModule* mod)
{
    delete[] m_value;

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
        if (type() == TYPE_SHORT_ASCII || type() == TYPE_SHORT_ASCII_INTERNED)
            m_length = stream->getByte();
        else
            m_length = stream->get32();

        if (m_length) {
            m_value = new char[m_length+1];
            stream->getBuffer(m_length, m_value);
            m_value[m_length] = 0;

            if (type() == TYPE_ASCII || type() == TYPE_ASCII_INTERNED ||
                    type() == TYPE_SHORT_ASCII || type() == TYPE_SHORT_ASCII_INTERNED)
                ascii_to_utf8(&m_value);
        } else {
            m_value = 0;
        }

        if (type() == TYPE_INTERNED || type() == TYPE_ASCII_INTERNED ||
                type() == TYPE_SHORT_ASCII_INTERNED)
            mod->intern(this);
    }
}

bool PycString::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj->type())
        return false;

    PycRef<PycString> strObj = obj.cast<PycString>();
    return isEqual(strObj->m_value);
}

bool PycString::isEqual(const char* str) const
{
    if (m_value == str)
        return true;
    return (strcmp(m_value, str) == 0);
}

void OutputString(PycRef<PycString> str, char prefix, bool triple, FILE* F)
{
    if (prefix != 0)
        fputc(prefix, F);

    const char* ch = str->value();
    int len = str->length();
    if (ch == 0) {
        fprintf(F, "''");
        return;
    }

    // Determine preferred quote style (Emulate Python's method)
    bool useQuotes = false;
    while (len--) {
        if (*ch == '\'') {
            useQuotes = true;
        } else if (*ch == '"') {
            useQuotes = false;
            break;
        }
        ch++;
    }
    ch = str->value();
    len = str->length();

    // Output the string
    if (triple)
        fprintf(F, useQuotes ? "\"\"\"" : "'''");
    else
        fputc(useQuotes ? '"' : '\'', F);
    while (len--) {
        if (*ch < 0x20 || *ch == 0x7F) {
            if (*ch == '\r') {
                fprintf(F, "\\r");
            } else if (*ch == '\n') {
                if (triple)
                    fputc('\n', F);
                else
                    fprintf(F, "\\n");
            } else if (*ch == '\t') {
                fprintf(F, "\\t");
            } else {
                fprintf(F, "\\x%x", (*ch & 0xFF));
            }
        } else if ((unsigned char)(*ch) >= 0x80) {
            if (str->type() == PycObject::TYPE_UNICODE) {
                // Unicode stored as UTF-8...  Let the stream interpret it
                fputc(*ch, F);
            } else {
                fprintf(F, "\\x%x", (*ch & 0xFF));
            }
        } else {
            if (!useQuotes && *ch == '\'')
                fprintf(F, "\\'");
            else if (useQuotes && *ch == '"')
                fprintf(F, "\\\"");
            else if (*ch == '\\')
                fprintf(F, "\\\\");
            else
                fputc(*ch, F);
        }
        ch++;
    }
    if (triple)
        fprintf(F, useQuotes ? "\"\"\"" : "'''");
    else
        fputc(useQuotes ? '"' : '\'', F);
}
