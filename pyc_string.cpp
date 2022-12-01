#include "pyc_string.h"
#include "pyc_module.h"
#include "data.h"
#include <stdexcept>

static bool check_ascii(const std::string& data)
{
    auto cp = reinterpret_cast<const unsigned char*>(data.c_str());
    while (*cp) {
        if (*cp & 0x80)
            return false;
        ++cp;
    }
    return true;
}

/* PycString */
void PycString::load(PycData* stream, PycModule* mod)
{
    if (type() == TYPE_STRINGREF) {
        PycRef<PycString> str = mod->getIntern(stream->get32());
        m_type = str->m_type;
        m_value = str->m_value;
    } else {
        int length;
        if (type() == TYPE_SHORT_ASCII || type() == TYPE_SHORT_ASCII_INTERNED)
            length = stream->getByte();
        else
            length = stream->get32();

        if (length < 0)
            throw std::bad_alloc();

        m_value.resize(length);
        if (length) {
            stream->getBuffer(length, &m_value.front());
            if (type() == TYPE_ASCII || type() == TYPE_ASCII_INTERNED ||
                    type() == TYPE_SHORT_ASCII || type() == TYPE_SHORT_ASCII_INTERNED) {
                if (!check_ascii(m_value))
                    throw std::runtime_error("Invalid bytes in ASCII string");
            }
        }

        if (type() == TYPE_INTERNED || type() == TYPE_ASCII_INTERNED ||
                type() == TYPE_SHORT_ASCII_INTERNED)
            mod->intern(this);
    }
}

bool PycString::isEqual(PycRef<PycObject> obj) const
{
    if (type() != obj.type())
        return false;

    PycRef<PycString> strObj = obj.cast<PycString>();
    return isEqual(strObj->m_value);
}

void OutputString(PycRef<PycString> str, char prefix, bool triple, FILE* F, const char* parent_f_string_quote)
{
    if (prefix != 0)
        fputc(prefix, F);

    const char* ch = str->value();
    int len = str->length();
    if (ch == 0) {
        fputs("''", F);
        return;
    }

    // Determine preferred quote style (Emulate Python's method)
    bool useQuotes = false;
    if (!parent_f_string_quote) {
        while (len--) {
            if (*ch == '\'') {
                useQuotes = true;
            } else if (*ch == '"') {
                useQuotes = false;
                break;
            }
            ch++;
        }
    } else {
        useQuotes = parent_f_string_quote[0] == '"';
    }
    ch = str->value();
    len = str->length();

    // Output the string
    if (!parent_f_string_quote) {
        if (triple)
            fputs(useQuotes ? "\"\"\"" : "'''", F);
        else
            fputc(useQuotes ? '"' : '\'', F);
    }
    while (len--) {
        if (*ch < 0x20 || *ch == 0x7F) {
            if (*ch == '\r') {
                fputs("\\r", F);
            } else if (*ch == '\n') {
                if (triple)
                    fputc('\n', F);
                else
                    fputs("\\n", F);
            } else if (*ch == '\t') {
                fputs("\\t", F);
            } else {
                fprintf(F, "\\x%02x", (*ch & 0xFF));
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
                fputs("\\'", F);
            else if (useQuotes && *ch == '"')
                fputs("\\\"", F);
            else if (*ch == '\\')
                fputs("\\\\", F);
            else if (parent_f_string_quote && *ch == '{')
                fputs("{{", F);
            else if (parent_f_string_quote && *ch == '}')
                fputs("}}", F);
            else
                fputc(*ch, F);
        }
        ch++;
    }
    if (!parent_f_string_quote) {
        if (triple)
            fputs(useQuotes ? "\"\"\"" : "'''", F);
        else
            fputc(useQuotes ? '"' : '\'', F);
    }
}
