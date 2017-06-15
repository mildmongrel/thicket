#include "CachedImageLoader.h"

#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QUrl>

#include "ImageCache.h"
#include "NetworkFileLoader.h"
#include "qtutils_core.h"


CachedImageLoader::CachedImageLoader( ImageCache*      imageCache,
                                      Logging::Config  loggingConfig,
                                      QObject*         parent )
  : QObject( parent ),
    mImageCache( imageCache ),
    mLogger( loggingConfig.createLogger() )
{
    mNetworkFileLoader = new NetworkFileLoader( loggingConfig.createChildConfig( "netloader" ), this );
    connect( mNetworkFileLoader, &NetworkFileLoader::fileLoaded, this, &CachedImageLoader::networkFileLoaded );
}


void
CachedImageLoader::loadImage( const QUrl& url, const QVariant& token )
{
    QImage image;

    // Try reading from the cache; if we find something we're done.
    if( mImageCache != 0 )
    {
        QString cacheImageName = getCacheImageName( token );
        if( !cacheImageName.isEmpty() )
        {
            bool readResult = mImageCache->tryReadFromCache( getCacheImageName( token ), image );
            if( readResult )
            {
                emit imageLoaded( token, image );
                return;
            }
        }
    }

    // Start a network image load.
    mNetworkFileLoader->loadFile( url, token );
}


void
CachedImageLoader::networkFileLoaded( const QVariant& token, const QByteArray& fileData )
{
    QBuffer buf;
    buf.setData( fileData );
    QImageReader imgReader;
    imgReader.setDecideFormatFromContent( true );
    imgReader.setDevice( &buf );

    QString extension = "." + imgReader.format();
    if( extension == ".jpeg" ) extension = ".jpg";

    QImage image;
    if( imgReader.read( &image ) )
    {
        if( (mImageCache != nullptr) && !image.isNull() )
        {
            const QString cacheImageName = getCacheImageName( token );
            if( !cacheImageName.isEmpty() )
            {
                mImageCache->tryWriteToCache( cacheImageName, extension, fileData );
            }
        }
        emit imageLoaded( token, image );
    }
    else
    {
        mLogger->warn( "Failed to read image from network data" );
    }
}
