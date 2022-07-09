#include "pyc_string.h"
#include "pyc_module.h"
#include "data.h"
#include <cstring>
#include <limits>

static void ascii_to_utf8(std::string* data)
{
    size_t utf8len = 0, asciilen = 0;
    auto cp = reinterpret_cast<const unsigned char*>(data->c_str());
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

    std::string utf8_buffer;
    utf8_buffer.resize(utf8len);
    auto up = reinterpret_cast<unsigned char*>(&utf8_buffer.front());
    cp = reinterpret_cast<const unsigned char*>(data->c_str());
    while (*cp) {
        if (*cp & 0x80) {
            *up++ = 0xC0 | ((*cp >> 6) & 0x1F);
            *up++ = 0x80 | ((*cp     ) & 0x3F);
        } else {
            *up++ = *cp;
        }
        ++cp;
    }

    *data = std::move(utf8_buffer);
}

/* PycString */
void PycString::load(PycData* stream, PycModule* mod)
{
    if (type() == TYPE_STRINGREF) {
        PycRef<PycString> str = mod->getIntern(stream->get32());
        m_value.resize(str->length());

        if (str->length())
            std::char_traits<char>::copy(&m_value.front(), str->value(), str->length());
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
                    type() == TYPE_SHORT_ASCII || type() == TYPE_SHORT_ASCII_INTERNED)
                ascii_to_utf8(&m_value);
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

std::string OutputString(PycRef<PycString> str, char prefix, bool triple, const char* parent_f_string_quote)
{
    std::string result;
    if (prefix != 0)
        result += prefix;

    const char* ch = str->value();
    int len = str->length();
    if (ch == 0) {
        result += "''";
        return result;
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
            result += useQuotes ? "\"\"\"" : "'''";
        else
            result += useQuotes ? '"' : '\'';
    }
    while (len--) {
        if (*ch < 0x20 || *ch == 0x7F) {
            if (*ch == '\r') {
                result += "\\r";
            } else if (*ch == '\n') {
                if (triple)
                    result += '\n';
                else
                    result += "\\n";
            } else if (*ch == '\t') {
                result += "\\t";
            } else {
                result += string_format("\\x%02x", (*ch & 0xFF));
            }
        } else if ((unsigned char)(*ch) >= 0x80) {
            if (str->type() == PycObject::TYPE_UNICODE) {
                // Unicode stored as UTF-8...  Let the stream interpret it
                result += *ch;
            } else {
                result += string_format("\\x%x", (*ch & 0xFF));
            }
        } else {
            if (!useQuotes && *ch == '\'')
                result += "\\'";
            else if (useQuotes && *ch == '"')
                result += "\\\"";
            else if (*ch == '\\')
                result += "\\\\";
            else if (parent_f_string_quote && *ch == '{')
                result += "{{";
            else if (parent_f_string_quote && *ch == '}')
                result += "}}";
            else
                result += *ch;
        }
        ch++;
    }
    if (!parent_f_string_quote) {
        if (triple)
            result += useQuotes ? "\"\"\"" : "'''";
        else
            result += useQuotes ? '"' : '\'';
    }
    return result;
}
