#include "data.h"
#include <cstring>
#include <cstdarg>
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
    m_pos += bytes;
    return bytes;
}

int formatted_print(std::ostream& stream, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = formatted_printv(stream, format, args);
    va_end(args);
    return result;
}

int formatted_printv(std::ostream& stream, const char* format, va_list args)
{
    va_list saved_args;
    va_copy(saved_args, args);
    int len = std::vsnprintf(nullptr, 0, format, args);
    if (len < 0)
        return len;
    std::vector<char> vec(static_cast<size_t>(len) + 1);
    int written = std::vsnprintf(&vec[0], vec.size(), format, saved_args);
    va_end(saved_args);

    if (written >= 0)
        stream << &vec[0];
    return written;
}
