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

    explicit ImageLoaderFactory( ImageCache*     cardImageCache,
                                 const QString&  cardImageUrlTemplateStr,
                                 ImageCache*     expSymImageCache,
                                 const QString&  expSymImageUrlTemplateStr,
                                 QObject*        parent = 0 );

    CardImageLoader* createCardImageLoader( Logging::Config loggingConfig = Logging::Config(),
                                            QObject*        parent = 0 );

    ExpSymImageLoader* createExpSymImageLoader( Logging::Config loggingConfig = Logging::Config(),
                                                QObject*        parent = 0 );
private:

    ImageCache* const        mCardImageCache;
    const QString            mCardImageUrlTemplateStr;
    ImageCache* const        mExpSymImageCache;
    const QString            mExpSymImageUrlTemplateStr;
};

#endif  // IMAGELOADERFACTORY_H
