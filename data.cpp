#include "data.h"
#include <cstring>
#include <ostream>
#include <vector>

/* PycData */
int PycData::get16()
{
    /* Ensure endianness */
    int result = getByte() & 0xFF;
    result |= (getByte() & 0xFF) << 8;
    return result;
}

int PycData::get32()
{
    /* Ensure endianness */
    int result = getByte() & 0xFF;
    result |= (getByte() & 0xFF) <<  8;
    result |= (getByte() & 0xFF) << 16;
    result |= (getByte() & 0xFF) << 24;
    return result;
}

Pyc_INT64 PycData::get64()
{
    /* Ensure endianness */
    Pyc_INT64 result = (Pyc_INT64)(getByte() & 0xFF);
    result |= (Pyc_INT64)(getByte() & 0xFF) <<  8;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 16;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 24;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 32;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 40;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 48;
    result |= (Pyc_INT64)(getByte() & 0xFF) << 56;
    return result;
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
    if (bytes != 0)
        memcpy(buffer, (m_buffer + m_pos), bytes);
    return bytes;
}



#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvarargs"
int	 formatted_print(std::ostream& stream, const std::string& format, ...) {
    va_list args;
    va_start(args, format);
    size_t len = std::vsnprintf(NULL, 0, format.c_str(), args);
    va_end(args);
    std::vector<char> vec(len + 1);
    va_start(args, format);
    std::vsnprintf(&vec[0], len + 1, format.c_str(), args);
    va_end(args);
    stream << &vec[0];
    return 0;
}
#pragma clang diagnostic pop