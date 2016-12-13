#ifndef QTUTILS_CORE_H
#define QTUTILS_CORE_H

#include <QString>
#include <sstream>
#include <iomanip>

// Qt stream output operator.  Very helpful when using QStrings as spdlog arguments.
inline std::ostream& operator <<( std::ostream &os, const QString &str )
{
   return (os << str.toStdString());
}

// Pretty printing for QByteArray with abbreviation.
inline std::string hexStringify( const QByteArray& ba, int abbrevLen = -1 )
{
    int limit = (abbrevLen > 0) ? std::min( abbrevLen, ba.size() ) : ba.size();
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill( '0' );

    for( int i = 0; i < limit; ++i )
    {
        ss << std::setw( 2 ) << ((unsigned int)(ba[i]) & 0xFF) << ' ';
    }
    if( limit < ba.size() )
        ss << "...";
    return ss.str();
}

#endif  // QTUTILS_CORE_H
