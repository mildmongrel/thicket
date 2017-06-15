#include "ImageLoaderFactory.h"

#include "CardImageLoader.h"
#include "ExpSymImageLoader.h"

ImageLoaderFactory::ImageLoaderFactory( ImageCache*     imageCache,
                                        const QString&  cardImageUrlTemplateStr,
                                        const QString&  expSymImageUrlTemplateStr,
                                        QObject*        parent )
  : QObject( parent ),
    mImageCache( imageCache ),
    mCardImageUrlTemplateStr( cardImageUrlTemplateStr ),
    mExpSymImageUrlTemplateStr( expSymImageUrlTemplateStr )
{}


CardImageLoader*
ImageLoaderFactory::createCardImageLoader( Logging::Config loggingConfig,
                                           QObject*        parent )
{
    return new CardImageLoader( mImageCache, mCardImageUrlTemplateStr, Logging::Config(), parent );
}


ExpSymImageLoader*
ImageLoaderFactory::createExpSymImageLoader( Logging::Config loggingConfig,
                                             QObject*        parent )
{
    return new ExpSymImageLoader( 0, mExpSymImageUrlTemplateStr, Logging::Config(), parent );
}
