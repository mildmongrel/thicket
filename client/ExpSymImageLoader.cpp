#include "ExpSymImageLoader.h"

#include <QUrl>
#include <QVariant>

#include "AllSetsData.h"

ExpSymImageLoader::ExpSymImageLoader( ImageCache*          imageCache,
                                      const QString&       urlTemplateStr,
                                      AllSetsDataSharedPtr allSetsData,
                                      Logging::Config      loggingConfig,
                                      QObject*             parent )
  : CachedImageLoader( imageCache, loggingConfig, parent ),
    mUrlTemplateStr( urlTemplateStr ),
    mAllSetsData( allSetsData ),
    mLogger( loggingConfig.createLogger() )
{
    connect( this, &CachedImageLoader::imageLoaded, [this](const QVariant& token, const QImage& image) {
            emit imageLoaded( token.toString(), image );
        } );

}


void
ExpSymImageLoader::loadImage( const QString& setCode )
{
    std::string gathererCodeStdStr = mAllSetsData->getSetGathererCode( setCode.toStdString() );
    QString gathererCode = gathererCodeStdStr.empty() ? setCode
                                                      : QString::fromStdString( gathererCodeStdStr );

    QString imageUrlStr( mUrlTemplateStr );
    imageUrlStr.replace( "%setcode%", gathererCode );
    QUrl url( imageUrlStr );
    CachedImageLoader::loadImage( url, QVariant( setCode ) );
}


QString
ExpSymImageLoader::getCacheImageName( const QVariant& token )
{
    // Return set code in token.
    return token.toString();
}
