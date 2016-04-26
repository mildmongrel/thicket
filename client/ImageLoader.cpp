#include "ImageLoader.h"

#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCryptographicHash>
#include <QUrl>

#include "ImageCache.h"
#include "qtutils_core.h"


ImageLoader::ImageLoader( ImageCache*      imageCache,
                          const QString&   cardImageUrlTemplateStr,
                          Logging::Config  loggingConfig,
                          QObject*         parent )
  : QObject( parent ),
    mImageCache( imageCache ),
    mCardImageUrlTemplateStr( cardImageUrlTemplateStr ),
    mLogger( loggingConfig.createLogger() )
{
    mNetworkAccessManager = new QNetworkAccessManager(this);
    connect(mNetworkAccessManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(networkAccessFinished(QNetworkReply*)));
}


void
ImageLoader::loadImage( int multiverseId )
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

    // Start a load from the web.  Use the URL template from settings and
    // substitute in the multiverse ID.
    QString imageUrlStr( mCardImageUrlTemplateStr );
    imageUrlStr.replace( "%muid%", QString::number( multiverseId ) );
    QUrl url( imageUrlStr );
    QNetworkRequest req( url );
    mLogger->debug( "starting picture download: {}", req.url().toString() );
    QNetworkReply* replyPtr;
    replyPtr = mNetworkAccessManager->get(req);
    mReplyToMuidMap.insert( replyPtr, multiverseId );
}


void
ImageLoader::networkAccessFinished( QNetworkReply *reply )
{
    // From the Qt docs:
    // Note: After the request has finished, it is the responsibility of the
    // user to delete the QNetworkReply object at an appropriate time. Do not
    // directly delete it inside the slot connected to finished(). You can use
    // the deleteLater() function.
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyScopedPtr( reply );

    QImage image;

    // Get the multiverse id for this reply and remove the association.
    if( !mReplyToMuidMap.contains( reply ) )
    {
        mLogger->warn( "no muid entry for reply!" );
        return;
    }
    int multiverseId = mReplyToMuidMap.value( reply );
    mReplyToMuidMap.remove( reply );

    mLogger->debug( "reply for picture download: {}", reply->url().toString() );

    // Detect errors in the reply.
    if (reply->error())
    {
        mLogger->warn( "Download failed: {}", reply->errorString() );
        emit imageLoaded( multiverseId, image );
        return;
    }

    // If this is a redirect, then make a new request.
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 301 || statusCode == 302)
    {
        QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        QNetworkRequest req(redirectUrl);
        mLogger->debug( "following redirect url: {}", req.url().toString() );
        // Need to put the new reply->muid in the map.
        QNetworkReply* newReplyPtr = mNetworkAccessManager->get(req);
        mReplyToMuidMap.insert( newReplyPtr, multiverseId );
        return;
    }

    // Use peek() here to keep the data in the buffer for later use by QImageReader
    const QByteArray &imageData = reply->peek(reply->size());

    QImageReader imgReader;
    imgReader.setDecideFormatFromContent(true);
    imgReader.setDevice(reply);
    QString extension = "." + imgReader.format();
    if( extension == ".jpeg" )
        extension = ".jpg";
    
    if( imgReader.read(&image) )
    {
        if( mImageCache != 0 )
        {
            mImageCache->tryWriteToCache( multiverseId, extension, imageData );
        }
        emit imageLoaded( multiverseId, image );
    }
    else
    {
        mLogger->warn( "Failed to read image from network data" );
    } 
}

