#include "StringUtil.h"

static bool icompare_pred( unsigned char a, unsigned char b )
{
    return std::tolower( a ) == std::tolower( b );
}


bool
StringUtil::icompare( std::string const& a, std::string const& b )
{
    if( a.length() == b.length() )
    {
        return std::equal( b.begin(), b.end(),
                           a.begin(), icompare_pred );
    }
    else
    {
        return false;
    }
}


std::string
StringUtil::trim( const std::string& str, const std::string& whitespace )
{
    const auto strBegin = str.find_first_not_of( whitespace );
    if( strBegin == std::string::npos )
        return ""; // no content

    const auto strEnd = str.find_last_not_of( whitespace );
    const auto strRange = strEnd - strBegin + 1;

    return str.substr( strBegin, strRange );
}

