#ifndef IMAGELOADERFACTORY_H
#define IMAGELOADERFACTORY_H

#include <QObject>
#include "ImageLoader.h"
#include "Logging.h"

class ImageCache;


class ImageLoaderFactory : public QObject
{
    Q_OBJECT

public:

    explicit ImageLoaderFactory( ImageCache*     imageCache,
                                 const QString&  cardImageUrlTemplateStr,
                                 QObject*        parent = 0 );

    ImageLoader* createImageLoader( Logging::Config loggingConfig = Logging::Config(),
                                    QObject*        parent = 0 );

private:

    ImageCache* const        mImageCache;
    const QString            mCardImageUrlTemplateStr;
};

#endif  // IMAGELOADERFACTORY_H
