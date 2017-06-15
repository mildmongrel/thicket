#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QByteArray;
class QImage;
class QString;
QT_END_NAMESPACE

class ImageCache
{
public:

    virtual ~ImageCache() {}

    virtual bool tryReadFromCache( const QString& nameWithoutExt, QImage& image ) = 0;
    virtual bool tryWriteToCache( const QString& nameWithoutExt, const QString& extension, const QByteArray& byteArray ) = 0;

};

#endif  // IMAGELOADER_H
