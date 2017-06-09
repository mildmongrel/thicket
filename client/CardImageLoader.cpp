#include "CardImageLoader.h"

#include <QImage>
#include <QUrl>

#include "ImageCache.h"
#include "NetworkImageLoader.h"
#include "qtutils_core.h"


CardImageLoader::CardImageLoader( ImageCache*      imageCache,
                                  const QString&   cardImageUrlTemplateStr,
                                  Logging::Config  loggingConfig,
                                  QObject*         parent )
  : QObject( parent ),
    mImageCache( imageCache ),
    mCardImageUrlTemplateStr( cardImageUrlTemplateStr ),
    mLogger( loggingConfig.createLogger() )
{
    mNetworkImageLoader = new NetworkImageLoader( loggingConfig.createChildConfig( "netloader" ), this );
    connect( mNetworkImageLoader, &NetworkImageLoader::imageLoaded, this, &CardImageLoader::networkImageLoaded );
}


void
CardImageLoader::loadImage( int multiverseId )
{
    QImage image;

    // Try reading from the cache; if we find something we're done.
    if( mImageCache != 0 )
    {
        bool readResult = mImageCache->tryReadFromCache( multiverseId, image );
        if( readResult )
        {
            emit imageLoaded( multiverseId, image );
            return;
        }
    }

    // Start a network image load.  Use the URL template from settings and
    // substitute in the multiverse ID.
    QString imageUrlStr( mCardImageUrlTemplateStr );
    imageUrlStr.replace( "%muid%", QString::number( multiverseId ) );
    QUrl url( imageUrlStr );
    mNetworkImageLoader->loadImage( url, QVariant( multiverseId ) );
}


void
CardImageLoader::networkImageLoaded( const QVariant& token, const QImage& image )
{
    emit imageLoaded( token.toInt(), image );
}
