#ifndef UNLIMITEDIMAGECACHE_H
#define UNLIMITEDIMAGECACHE_H

#include "ImageCache.h"

#include <QDir>

#include "Logging.h"

class UnlimitedImageCache : public ImageCache
{
public:

    UnlimitedImageCache( const QDir&     imageCacheDir,
                         Logging::Config loggingConfig = Logging::Config() );

    virtual bool tryReadFromCache( const QString& nameWithoutExt, QImage& image ) override;
    virtual bool tryWriteToCache( const QString& nameWithoutExt, const QString& extension, const QByteArray& byteArray ) override;

private:

    QDir                            mCacheDir;
    std::shared_ptr<spdlog::logger> mLogger;

};

#endif  // UNLIMITEDIMAGECACHE_H
