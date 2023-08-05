#ifndef _PYC_FILE_H
#define _PYC_FILE_H

#include <cstdio>
#include <ostream>

#ifdef WIN32
typedef __int64 Pyc_INT64;
#else
typedef long long Pyc_INT64;
#endif

class PycData {
public:
    PycData() { }
    virtual ~PycData() { }

    virtual bool isOpen() const = 0;
    virtual bool atEof() const = 0;

    virtual int getByte() = 0;
    virtual int getBuffer(int bytes, void* buffer) = 0;
    int get16();
    int get32();
    Pyc_INT64 get64();
};

class PycFile : public PycData {
public:
    PycFile(const char* filename);
    ~PycFile() { if (m_stream) fclose(m_stream); }

    bool isOpen() const override { return (m_stream != 0); }
    bool atEof() const override;

    int getByte() override;
    int getBuffer(int bytes, void* buffer) override;

private:
    FILE* m_stream;
};

class PycBuffer : public PycData {
public:
    PycBuffer(const void* buffer, int size)
        : m_buffer((const unsigned char*)buffer), m_size(size), m_pos(0) { }
    ~PycBuffer() { }

    bool isOpen() const override { return (m_buffer != 0); }
    bool atEof() const override { return (m_pos == m_size); }

    int getByte() override;
    int getBuffer(int bytes, void* buffer) override;

private:
    const unsigned char* m_buffer;
    int m_size, m_pos;
};

int formatted_print(std::ostream& stream, const char* format, ...);
int formatted_printv(std::ostream& stream, const char* format, va_list args);

#endif
