#include "ImageLoaderFactory.h"

#include "CardImageLoader.h"
#include "ExpSymImageLoader.h"

ImageLoaderFactory::ImageLoaderFactory( ImageCache*     cardImageCache,
                                        const QString&  cardImageUrlTemplateStr,
                                        ImageCache*     expSymImageCache,
                                        const QString&  expSymImageUrlTemplateStr,
                                        QObject*        parent )
  : QObject( parent ),
    mCardImageCache( cardImageCache ),
    mCardImageUrlTemplateStr( cardImageUrlTemplateStr ),
    mExpSymImageCache( expSymImageCache ),
    mExpSymImageUrlTemplateStr( expSymImageUrlTemplateStr )
{}


CardImageLoader*
ImageLoaderFactory::createCardImageLoader( Logging::Config loggingConfig,
                                           QObject*        parent )
{
    return new CardImageLoader( mCardImageCache, mCardImageUrlTemplateStr, Logging::Config(), parent );
}


ExpSymImageLoader*
ImageLoaderFactory::createExpSymImageLoader( Logging::Config loggingConfig,
                                             QObject*        parent )
{
    return new ExpSymImageLoader( mExpSymImageCache, mExpSymImageUrlTemplateStr, Logging::Config(), parent );
}
