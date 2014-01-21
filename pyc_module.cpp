#include "pyc_module.h"
#include "data.h"

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
        fprintf(stderr, "Bad MAGIC!\n");
        return;
    }
    in.get32(); // Timestamp -- who cares?

    if (verCompare(3, 3) >= 0)
        in.get32(); // Size parameter added in Python 3.3

    m_code = LoadObject(&in, this).cast<PycCode>();
}

PycRef<PycString> PycModule::getIntern(int ref) const
{
    std::list<PycRef<PycString> >::const_iterator it = m_interns.begin();
    while (ref--)
        ++it;
    return *it;
}

PycRef<PycObject> PycModule::getRef(int ref) const
{
    std::list<PycRef<PycObject> >::const_iterator it = m_refs.begin();
    while (ref--)
        ++it;
    return *it;
}
