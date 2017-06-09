#ifndef IMAGELOADERFACTORY_H
#define IMAGELOADERFACTORY_H

#include <QObject>
#include "Logging.h"

class ImageCache;
class CardImageLoader;


class ImageLoaderFactory : public QObject
{
    Q_OBJECT

public:

    explicit ImageLoaderFactory( ImageCache*     imageCache,
                                 const QString&  cardImageUrlTemplateStr,
                                 QObject*        parent = 0 );

    CardImageLoader* createImageLoader( Logging::Config loggingConfig = Logging::Config(),
                                        QObject*        parent = 0 );

private:

    ImageCache* const        mImageCache;
    const QString            mCardImageUrlTemplateStr;
};

#endif  // IMAGELOADERFACTORY_H
