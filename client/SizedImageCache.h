#ifndef SIZEDIMAGECACHE_H
#define SIZEDIMAGECACHE_H

#include "ImageCache.h"

#include <QDir>
#include <QDateTime>

#include "Logging.h"

class SizedImageCache : public ImageCache
{
public:

    SizedImageCache( const QDir&     imageCacheDir,
                     quint64         maxBytes,
                     Logging::Config loggingConfig = Logging::Config() );
    virtual ~SizedImageCache();

    unsigned int getCount() const { return mCacheIndex.size(); }
    quint64 getCurrentBytes() const { return mCacheCurrentBytes; }

    void setMaxBytes( quint64 maxBytes );

    virtual bool tryReadFromCache( const QString& nameWithoutExt, QImage& image ) override;
    virtual bool tryWriteToCache( const QString& nameWithoutExt, const QString& extension, const QByteArray& byteArray ) override;

    // Would be private except for stream operators.
    struct IndexEntry
    {
        QString   fileName;
        QDateTime fileAccessDateTime;
    };

private:

    bool deserializeCacheIndex();
    bool serializeCacheIndex();
    bool resizeCache( quint64 maxSize );

    QDir                            mCacheDir;
    quint64                         mCacheMaxBytes;
    QList<IndexEntry>               mCacheIndex;
    quint64                         mCacheCurrentBytes;

    std::shared_ptr<spdlog::logger> mLogger;

};

#endif  // SIZEDIMAGECACHE_H
