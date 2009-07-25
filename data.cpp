#include "data.h"
#include <cstring>

/* PycData */
int PycData::get16()
{
    /* Ensure endianness */
    int val = ( ((getByte() & 0xFF)     )
              | ((getByte() & 0xFF) << 8)
              );

    /* Extend sign */
    return (val | -(val & 0x8000));
}

int PycData::get32()
{
    /* Ensure endianness */
    return (int)( ((getByte() & 0xFF)      )
                | ((getByte() & 0xFF) <<  8)
                | ((getByte() & 0xFF) << 16)
                | ((getByte() & 0xFF) << 24)
                );
}

Pyc_INT64 PycData::get64()
{
    /* Ensure endianness */
    return (Pyc_INT64)( ((Pyc_INT64)(getByte() & 0xFF)      )
                      | ((Pyc_INT64)(getByte() & 0xFF) <<  8)
                      | ((Pyc_INT64)(getByte() & 0xFF) << 16)
                      | ((Pyc_INT64)(getByte() & 0xFF) << 24)
                      | ((Pyc_INT64)(getByte() & 0xFF) << 32)
                      | ((Pyc_INT64)(getByte() & 0xFF) << 40)
                      | ((Pyc_INT64)(getByte() & 0xFF) << 48)
                      | ((Pyc_INT64)(getByte() & 0xFF) << 56)
                      );
}


/* PycFile */
PycFile::PycFile(const char* filename)
{
    m_stream = fopen(filename, "rb");
}

bool PycFile::atEof() const
{
    int ch = fgetc(m_stream);
    ungetc(ch, m_stream);
    return (ch == EOF);
}

int PycFile::getByte()
{
    int ch = fgetc(m_stream);
    if (ch == EOF)
        ungetc(ch, m_stream);
    return ch;
}

int PycFile::getBuffer(int bytes, void* buffer)
{
    return (int)fread(buffer, 1, bytes, m_stream);
}


/* PycBuffer */
int PycBuffer::getByte()
{
    if (atEof())
        return EOF;
    int ch = (int)(*(m_buffer + m_pos));
    ++m_pos;
    return ch & 0xFF;   // Make sure it's just a byte!
}

int PycBuffer::getBuffer(int bytes, void* buffer)
{
    if (m_pos + bytes > m_size)
        bytes = m_size - m_pos;
    memcpy(buffer, (m_buffer + m_pos), bytes);
    return bytes;
}
