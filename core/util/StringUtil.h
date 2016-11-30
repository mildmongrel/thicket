#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <string>

namespace StringUtil
{
    // Case-insensitive string comparison.
    bool icompare( std::string const& a, std::string const& b );

    // Trim whitespace on both ends of a string.
    std::string trim( const std::string& str, const std::string& whitespace = " \t");
}

#endif
