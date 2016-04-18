#include "qtutils.h"

#include <QByteArray>
#include <QBuffer>
#include <QLayout>
#include <QLayoutItem>
#include <QWidget>


QString
qtutils::getImageAsHtmlText( const QImage& image )
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return QString("<img src=\"data:image/png;base64,%1\">").arg(QString(buffer.data().toBase64()));
}


QString
qtutils::getPixmapAsHtmlText( const QPixmap& pixmap )
{
    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    pixmap.save( &buffer, "PNG" );
    return QString( "<img src=\"data:image/png;base64,%1\">").arg( QString( buffer.data().toBase64() ) );
}


void
qtutils::clearLayout( QLayout* layout )
{
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
            item->layout()->deleteLater();
        }
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}


int
qtutils::getDefaultFontHeight()
{
    QFont f;
    QFontMetrics fm( f );
    return fm.height();
}
