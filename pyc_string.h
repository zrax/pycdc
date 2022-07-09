#ifndef _PYC_STRING_H
#define _PYC_STRING_H

#include "pyc_object.h"
#include "data.h"
#include <cstdio>
#include <string>
#include <stdexcept>
#include <memory>

class PycString : public PycObject {
public:
    PycString(int type = TYPE_STRING)
        : PycObject(type) { }

    bool isEqual(PycRef<PycObject> obj) const override;
    bool isEqual(const std::string& str) const { return m_value == str; }

    bool startsWith(const std::string& str) const
    {
        return m_value.substr(0, str.size()) == str;
    }

    void load(class PycData* stream, class PycModule* mod) override;

    int length() const { return (int)m_value.size(); }
    const char* value() const { return m_value.c_str(); }
    const std::string &strValue() const { return m_value; }

    void setValue(std::string str) { m_value = std::move(str); }

private:
    std::string m_value;
};

std::string OutputString(PycRef<PycString> str, char prefix = 0, bool triple = false,
                  const char* parent_f_string_quote = nullptr);

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

#endif
