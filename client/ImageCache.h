#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QString>
#include <QDir>
#include "Logging.h"

QT_BEGIN_NAMESPACE
class QImage;
QT_END_NAMESPACE

class ImageCache
{
public:
    ImageCache( const QDir&     imageCacheDir,
                Logging::Config loggingConfig = Logging::Config() );

    bool tryReadFromCache( int multiverseId, QImage& image );
    bool tryWriteToCache( int multiverseId, const QString& extension, const QByteArray& byteArray );

private:

    QDir                            mImageCacheDir;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // IMAGELOADER_H
