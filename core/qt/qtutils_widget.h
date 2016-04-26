#ifndef QTUTILS_WIDGET_H
#define QTUTILS_WIDGET_H

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

#endif  // QTUTILS_WIDGET_H
