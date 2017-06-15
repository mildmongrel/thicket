#include "ExpSymImageLoader.h"

#include <QUrl>
#include <QVariant>

ExpSymImageLoader::ExpSymImageLoader( ImageCache*      imageCache,
                                      const QString&   urlTemplateStr,
                                      Logging::Config  loggingConfig,
                                      QObject*         parent )
  : CachedImageLoader( imageCache, loggingConfig, parent ),
    mUrlTemplateStr( urlTemplateStr ),
    mLogger( loggingConfig.createLogger() )
{
    connect( this, &CachedImageLoader::imageLoaded, [this](const QVariant& token, const QImage& image) {
            emit imageLoaded( token.toString(), image );
        } );

}


void
ExpSymImageLoader::loadImage( const QString& setCode )
{
    QString imageUrlStr( mUrlTemplateStr );
    imageUrlStr.replace( "%setcode%", setCode );
    QUrl url( imageUrlStr );
    CachedImageLoader::loadImage( url, QVariant( setCode ) );
}


QString
ExpSymImageLoader::getCacheImageName( const QVariant& token )
{
    // Return set code in token.
    return token.toString();
}
