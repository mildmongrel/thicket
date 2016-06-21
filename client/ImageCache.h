#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QString>
#include <QDir>
#include <QDateTime>
#include "Logging.h"

QT_BEGIN_NAMESPACE
class QImage;
QT_END_NAMESPACE

class ImageCache
{
public:

    ImageCache( const QDir&     imageCacheDir,
                quint64         maxBytes,
                Logging::Config loggingConfig = Logging::Config() );
    virtual ~ImageCache();

    unsigned int getCount() { return mCacheIndex.size(); }
    quint64 getCurrentBytes() { return mCacheCurrentBytes; }

    bool tryReadFromCache( int multiverseId, QImage& image );
    bool tryWriteToCache( int multiverseId, const QString& extension, const QByteArray& byteArray );

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

#endif  // IMAGELOADER_H
