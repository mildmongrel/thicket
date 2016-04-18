#ifndef QTUTILS_H
#define QTUTILS_H

#include <QString>
#include <QImage>

QT_BEGIN_NAMESPACE
class QLayout;
QT_END_NAMESPACE

namespace qtutils
{

    // A great little trick to convert images/pixmaps to HTML text that
    // can be inserted into an HTML-capable QLabel or tooltip.
    QString getImageAsHtmlText( const QImage& image );
    QString getPixmapAsHtmlText( const QPixmap& pixmap );

    // Clear a QLayout.
    void clearLayout( QLayout* layout );

    // Compute the default font height.
    int getDefaultFontHeight();
}

// Qt stream output operator.  Very helpful when using QStrings as spdlog arguments.
inline std::ostream& operator <<( std::ostream &os, const QString &str )
{
   return (os << str.toStdString());
}

#endif  // QTUTILS_H
