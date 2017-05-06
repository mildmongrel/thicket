#ifndef QTUTILS_CORE_H
#define QTUTILS_CORE_H

#include <QString>
#include <sstream>
#include <iomanip>
#include <memory>


// Qt stream output operator.  Very helpful when using QStrings as spdlog arguments.
inline std::ostream& operator <<( std::ostream &os, const QString &str )
{
   return (os << str.toStdString());
}


// Allow std::shared_ptr to be used in QSet, etc.
template <typename T>
inline uint qHash( const std::shared_ptr<T>& ptr, uint seed = 0 ) Q_DECL_NOTHROW
{
    return qHash( ptr.get(), seed );
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


// Stringify QList types.
template<typename T>
std::string
stringify( const QList<T>& list )
{
    std::ostringstream out;
    out << "[";
    for( const auto& item : list )
    {
        out << item;
        if( item != list.last() )
        {
            out << ", ";
        }
    }
    out << "]";
    return out.str();
}

#endif  // QTUTILS_CORE_H
