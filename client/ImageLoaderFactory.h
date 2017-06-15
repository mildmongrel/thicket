#ifndef IMAGELOADERFACTORY_H
#define IMAGELOADERFACTORY_H

#include <QObject>
#include "Logging.h"

class ImageCache;
class CardImageLoader;
class ExpSymImageLoader;


class ImageLoaderFactory : public QObject
{
    Q_OBJECT

public:

    explicit ImageLoaderFactory( ImageCache*     imageCache,
                                 const QString&  cardImageUrlTemplateStr,
                                 const QString&  expSymImageUrlTemplateStr,
                                 QObject*        parent = 0 );

    CardImageLoader* createCardImageLoader( Logging::Config loggingConfig = Logging::Config(),
                                            QObject*        parent = 0 );

    ExpSymImageLoader* createExpSymImageLoader( Logging::Config loggingConfig = Logging::Config(),
                                                QObject*        parent = 0 );
private:

    ImageCache* const        mImageCache;
    const QString            mCardImageUrlTemplateStr;
    const QString            mExpSymImageUrlTemplateStr;
};

#endif  // IMAGELOADERFACTORY_H
