#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <string>
#include <vector>

namespace StringUtil
{
    // Case-insensitive string comparison.
    bool icompare( std::string const& a, std::string const& b );

    // Trim whitespace on both ends of a string.
    std::string trim( const std::string& str, const std::string& whitespace = " \t");

    template<typename T> std::string stringify( const std::vector<T>& v );
}

#include <sstream>

template<typename T>
std::string
StringUtil::stringify( const std::vector<T>& v )
{
    std::ostringstream out;
    out << "[";
    size_t last = v.size() - 1;
    for(size_t i = 0; i < v.size(); ++i) {
        out << v[i];
        if (i != last) 
            out << ", ";
    }
    out << "]";
    return out.str();
}

#endif
