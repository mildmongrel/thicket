#ifndef CACHEDIMAGELOADER_H
#define CACHEDIMAGELOADER_H

#include <QObject>

class ImageCache;
class NetworkFileLoader;

#include "Logging.h"

// This is an abstract base class used to handle cache management and
// loading of network images.  Derived classes must implement
// getCacheImageName() to return the cache image name given a token
// like the one submitted to loadImage().
class CachedImageLoader : public QObject
{
    Q_OBJECT

signals:
    void imageLoaded( const QVariant& token, const QImage& image );

private slots:
    void networkFileLoaded( const QVariant& token, const QByteArray& fileData );

protected:

    CachedImageLoader( ImageCache*     imageCache,
                       Logging::Config loggingConfig = Logging::Config(),
                       QObject*        parent = 0 );

    void loadImage( const QUrl& url, const QVariant& token );

    // Get cache image name for token.  Derived classes must implement this
    // to complete support for cache read/wrote.  Return an empty string if
    // the token can't be understood for any reason.
    virtual QString getCacheImageName( const QVariant& token ) = 0;

private:

    ImageCache* const        mImageCache;
    NetworkFileLoader*       mNetworkFileLoader;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CACHEDIMAGELOADER_H
