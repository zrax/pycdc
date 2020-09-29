#include "pyc_module.h"
#include "data.h"
#include <stdexcept>

void PycModule::setVersion(unsigned int magic)
{
    // Default for versions that don't support unicode selection
    m_unicode = false;

    switch (magic) {
    case MAGIC_1_0:
        m_maj = 1;
        m_min = 0;
        break;
    case MAGIC_1_1:
        m_maj = 1;
        m_min = 1;
        break;
    case MAGIC_1_3:
        m_maj = 1;
        m_min = 3;
        break;
    case MAGIC_1_4:
        m_maj = 1;
        m_min = 4;
        break;
    case MAGIC_1_5:
        m_maj = 1;
        m_min = 5;
        break;

    /* Starting with 1.6, Python adds +1 for unicode mode (-U) */
    case MAGIC_1_6+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_1_6:
        m_maj = 1;
        m_min = 6;
        break;
    case MAGIC_2_0+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_0:
        m_maj = 2;
        m_min = 0;
        break;
    case MAGIC_2_1+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_1:
        m_maj = 2;
        m_min = 1;
        break;
    case MAGIC_2_2+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_2:
        m_maj = 2;
        m_min = 2;
        break;
    case MAGIC_2_3+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_3:
        m_maj = 2;
        m_min = 3;
        break;
    case MAGIC_2_4+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_4:
        m_maj = 2;
        m_min = 4;
        break;
    case MAGIC_2_5+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_5:
        m_maj = 2;
        m_min = 5;
        break;
    case MAGIC_2_6+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_6:
        m_maj = 2;
        m_min = 6;
        break;
    case MAGIC_2_7+1:
        m_unicode = true;
        /* Fall through */
    case MAGIC_2_7:
        m_maj = 2;
        m_min = 7;
        break;

    /* 3.0 and above are always unicode */
    case MAGIC_3_0+1:
        m_maj = 3;
        m_min = 0;
        m_unicode = true;
        break;
    case MAGIC_3_1+1:
        m_maj = 3;
        m_min = 1;
        m_unicode = true;
        break;

    /* 3.2 stops using the unicode increment */
    case MAGIC_3_2:
        m_maj = 3;
        m_min = 2;
        m_unicode = true;
        break;

    case MAGIC_3_3:
        m_maj = 3;
        m_min = 3;
        m_unicode = true;
        break;

    case MAGIC_3_4:
        m_maj = 3;
        m_min = 4;
        m_unicode = true;
        break;

    case MAGIC_3_5:
        /* fall through */

    case MAGIC_3_5_3:
        m_maj = 3;
        m_min = 5;
        m_unicode = true;
        break;

    case MAGIC_3_6:
        m_maj = 3;
        m_min = 6;
        m_unicode = true;
        break;

    case MAGIC_3_7:
        m_maj = 3;
        m_min = 7;
        m_unicode = true;
        break;

    case MAGIC_3_8:
        m_maj = 3;
        m_min = 8;
        m_unicode = true;
        break;

    case MAGIC_3_9:
        m_maj = 3;
        m_min = 9;
        m_unicode = true;
        break;

    /* Bad Magic detected */
    default:
        m_maj = -1;
        m_min = -1;
    }
}

void PycModule::loadFromFile(const char* filename)
{
    PycFile in(filename);
    if (!in.isOpen()) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return;
    }
    setVersion(in.get32());
    if (!isValid()) {
        fputs("Bad MAGIC!\n", stderr);
        return;
    }

    int flags = 0;
    if (verCompare(3, 7) >= 0)
        flags = in.get32();

    if (flags & 0x1) {
        // Optional checksum added in Python 3.7
        in.get32();
        in.get32();
    } else {
        in.get32(); // Timestamp -- who cares?

        if (verCompare(3, 3) >= 0)
            in.get32(); // Size parameter added in Python 3.3
    }

    m_code = LoadObject(&in, this).require_cast<PycCode>();
}

PycRef<PycString> PycModule::getIntern(int ref) const
{
    if (ref < 0)
        throw std::out_of_range("Intern index out of range");

    auto it = m_interns.cbegin();
    while (ref-- && it != m_interns.cend())
        ++it;
    if (it == m_interns.cend())
        throw std::out_of_range("Intern index out of range");
    return *it;
}

PycRef<PycObject> PycModule::getRef(int ref) const
{
    if (ref < 0)
        throw std::out_of_range("Ref index out of range");

    auto it = m_refs.cbegin();
    while (ref-- && it != m_refs.cend())
        ++it;
    if (it == m_refs.cend())
        throw std::out_of_range("Ref index out of range");
    return *it;
}
