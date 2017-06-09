#ifndef CARDIMAGELOADER_H
#define CARDIMAGELOADER_H

#include <QObject>

class ImageCache;
class NetworkImageLoader;

#include "Logging.h"

class CardImageLoader : public QObject
{
    Q_OBJECT

public:
    CardImageLoader( ImageCache*     imageCache,
                     const QString&  cardImageUrlTemplateStr,
                     Logging::Config loggingConfig = Logging::Config(),
                     QObject*        parent = 0 );

    void loadImage( int multiverseId );

signals:
    void imageLoaded( int multiverseId, const QImage& image );

private slots:
    void networkImageLoaded( const QVariant& token, const QImage& image );

private:

    ImageCache* const        mImageCache;
    const QString            mCardImageUrlTemplateStr;
    NetworkImageLoader*      mNetworkImageLoader;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CARDIMAGELOADER_H
