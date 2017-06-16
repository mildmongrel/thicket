#include "ImageLoaderFactory.h"

#include "CardImageLoader.h"
#include "ExpSymImageLoader.h"

ImageLoaderFactory::ImageLoaderFactory( AllSetsDataSharedPtr allSetsData,
                                        ImageCache*          cardImageCache,
                                        const QString&       cardImageUrlTemplateStr,
                                        ImageCache*          expSymImageCache,
                                        const QString&       expSymImageUrlTemplateStr,
                                        QObject*             parent )
  : QObject( parent ),
    mAllSetsData( allSetsData ),
    mCardImageCache( cardImageCache ),
    mCardImageUrlTemplateStr( cardImageUrlTemplateStr ),
    mExpSymImageCache( expSymImageCache ),
    mExpSymImageUrlTemplateStr( expSymImageUrlTemplateStr )
{}


CardImageLoader*
ImageLoaderFactory::createCardImageLoader( Logging::Config loggingConfig,
                                           QObject*        parent )
{
    return new CardImageLoader( mCardImageCache, mCardImageUrlTemplateStr, loggingConfig, parent );
}


ExpSymImageLoader*
ImageLoaderFactory::createExpSymImageLoader( Logging::Config loggingConfig,
                                             QObject*        parent )
{
    return new ExpSymImageLoader( mExpSymImageCache, mExpSymImageUrlTemplateStr, mAllSetsData, loggingConfig, parent );
}
