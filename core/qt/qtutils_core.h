#ifndef QTUTILS_CORE_H
#define QTUTILS_CORE_H

#include <QString>

// Qt stream output operator.  Very helpful when using QStrings as spdlog arguments.
inline std::ostream& operator <<( std::ostream &os, const QString &str )
{
   return (os << str.toStdString());
}

#endif  // QTUTILS_CORE_H
