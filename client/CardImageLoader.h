#ifndef CARDIMAGELOADER_H
#define CARDIMAGELOADER_H

#include "CachedImageLoader.h"

class ImageCache;
class NetworkFileLoader;

#include "Logging.h"

class CardImageLoader : public CachedImageLoader
{
    Q_OBJECT

public:
    CardImageLoader( ImageCache*     imageCache,
                     const QString&  urlTemplateStr,
                     Logging::Config loggingConfig = Logging::Config(),
                     QObject*        parent = 0 );

    void loadImage( int multiverseId );

signals:
    void imageLoaded( int multiverseId, const QImage& image );


protected:

    // Get cache image name for token.  Derived classes must implement this
    // to complete support for cache read/wrote.
    virtual QString getCacheImageName( const QVariant& token ) override;

private:

    const QString            mUrlTemplateStr;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CARDIMAGELOADER_H
