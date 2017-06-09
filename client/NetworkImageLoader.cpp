#include "NetworkImageLoader.h"

#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "qtutils_core.h"


NetworkImageLoader::NetworkImageLoader( Logging::Config  loggingConfig,
                                        QObject*         parent )
  : QObject( parent ),
    mLogger( loggingConfig.createLogger() )
{
    mNetworkAccessManager = new QNetworkAccessManager(this);
    connect(mNetworkAccessManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(networkAccessFinished(QNetworkReply*)));
}


void
NetworkImageLoader::loadImage( const QUrl& url, const QVariant& token )
{
    // Start a load from the web.
    QNetworkRequest req( url );
    mLogger->debug( "starting image download: {}", req.url().toString() );
    QNetworkReply* replyPtr;
    replyPtr = mNetworkAccessManager->get(req);

    mReplyToTokenMap.insert( replyPtr, token );
}


void
NetworkImageLoader::networkAccessFinished( QNetworkReply *reply )
{
    // From the Qt docs:
    // Note: After the request has finished, it is the responsibility of the
    // user to delete the QNetworkReply object at an appropriate time. Do not
    // directly delete it inside the slot connected to finished(). You can use
    // the deleteLater() function.
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyScopedPtr( reply );

    QImage image;

    // Get the multiverse id for this reply and remove the association.
    if( !mReplyToTokenMap.contains( reply ) )
    {
        mLogger->warn( "no map entry for reply!" );
        return;
    }
    QVariant token = mReplyToTokenMap.value( reply );
    mReplyToTokenMap.remove( reply );

    mLogger->debug( "reply for picture download: {}", reply->url().toString() );

    // Detect errors in the reply.
    if (reply->error())
    {
        mLogger->warn( "Download failed: {}", reply->errorString() );
        emit imageLoaded( token, image );
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
        mReplyToTokenMap.insert( newReplyPtr, token );
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
        emit imageLoaded( token, image );
    }
    else
    {
        mLogger->warn( "Failed to read image from network data" );
    } 
}
