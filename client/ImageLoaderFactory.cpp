#include "ImageLoaderFactory.h"

#include "CardImageLoader.h"

ImageLoaderFactory::ImageLoaderFactory( ImageCache*     imageCache,
                                        const QString&  cardImageUrlTemplateStr,
                                        QObject*        parent )
  : QObject( parent ),
    mImageCache( imageCache ),
    mCardImageUrlTemplateStr( cardImageUrlTemplateStr )
{} 


CardImageLoader*
ImageLoaderFactory::createImageLoader( Logging::Config loggingConfig,
                                       QObject*        parent )
{
    return new CardImageLoader( mImageCache, mCardImageUrlTemplateStr, Logging::Config(), parent );
}
