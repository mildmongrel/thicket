#include "CardImageLoader.h"

#include <QUrl>
#include <QVariant>

CardImageLoader::CardImageLoader( ImageCache*      imageCache,
                                  const QString&   urlTemplateStr,
                                  Logging::Config  loggingConfig,
                                  QObject*         parent )
  : CachedImageLoader( imageCache, loggingConfig, parent ),
    mUrlTemplateStr( urlTemplateStr ),
    mLogger( loggingConfig.createLogger() )
{
    connect( this, &CachedImageLoader::imageLoaded, [this](const QVariant& token, const QImage& image) {
            emit imageLoaded( token.toInt(), image );
        } );

}


void
CardImageLoader::loadImage( int multiverseId )
{
    QString imageUrlStr( mUrlTemplateStr );
    imageUrlStr.replace( "%muid%", QString::number( multiverseId ) );
    QUrl url( imageUrlStr );
    CachedImageLoader::loadImage( url, QVariant( multiverseId ) );
}


QString
CardImageLoader::getCacheImageName( const QVariant& token )
{
    // Return multiverse id as string.
    return QString::number( token.toInt() );
}
