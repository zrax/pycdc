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

void OutputString(std::ostream &pyc_output, PycRef<PycString> str, char prefix,
                  bool triple, const char* parent_f_string_quote)
{
    if (prefix != 0)
        pyc_output << prefix;

    const char* ch = str->value();
    int len = str->length();
    if (ch == 0) {
        pyc_output << "''";
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
            pyc_output << (useQuotes ? "\"\"\"" : "'''");
        else
            pyc_output << (useQuotes ? '"' : '\'');
    }
    while (len--) {
        if ((unsigned char)(*ch) < 0x20 || *ch == 0x7F) {
            if (*ch == '\r') {
                pyc_output << "\\r";
            } else if (*ch == '\n') {
                if (triple)
                    pyc_output << '\n';
                else
                    pyc_output << "\\n";
            } else if (*ch == '\t') {
                pyc_output << "\\t";
            } else {
                formatted_print(pyc_output, "\\x%02x", (*ch & 0xFF));
            }
        } else if ((unsigned char)(*ch) >= 0x80) {
            if (str->type() == PycObject::TYPE_UNICODE) {
                // Unicode stored as UTF-8...  Let the stream interpret it
                pyc_output << *ch;
            } else {
                formatted_print(pyc_output, "\\x%x", (*ch & 0xFF));
            }
        } else {
            if (!useQuotes && *ch == '\'')
                pyc_output << "\\'";
            else if (useQuotes && *ch == '"')
                pyc_output << "\\\"";
            else if (*ch == '\\')
                pyc_output << "\\\\";
            else if (parent_f_string_quote && *ch == '{')
                pyc_output << "{{";
            else if (parent_f_string_quote && *ch == '}')
                pyc_output << "}}";
            else
                pyc_output << *ch;
        }
        ch++;
    }
    if (!parent_f_string_quote) {
        if (triple)
            pyc_output << (useQuotes ? "\"\"\"" : "'''");
        else
            pyc_output << (useQuotes ? '"' : '\'');
    }
}
