#include "ImageLoaderFactory.h"

ImageLoaderFactory::ImageLoaderFactory( ImageCache*     imageCache,
                                        const QString&  cardImageUrlTemplateStr,
                                        QObject*        parent )
  : QObject( parent ),
    mImageCache( imageCache ),
    mCardImageUrlTemplateStr( cardImageUrlTemplateStr )
{} 


ImageLoader*
ImageLoaderFactory::createImageLoader( Logging::Config loggingConfig,
                                       QObject*        parent )
{
    return new ImageLoader( mImageCache, mCardImageUrlTemplateStr, Logging::Config(), parent );
}
