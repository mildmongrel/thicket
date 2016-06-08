#include "DeckStatsLauncher.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegExp>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrlQuery>
#include <QUrl>
#include <QByteArray>

#include "Decklist.h"


DeckStatsLauncher::DeckStatsLauncher( QObject *parent )
  : QObject( parent )
{
    mNetworkAccessManager = new QNetworkAccessManager( this );
    connect( mNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(queryFinished(QNetworkReply*)) );
}


void
DeckStatsLauncher::launch( const Decklist& decklist )
{
    QUrlQuery urlQuery;
    urlQuery.addQueryItem( "deck", QString::fromStdString( decklist.getFormattedString() ) );
    urlQuery.addQueryItem( "decktitle", "Thicket Draft Deck" );

    QUrl params;
    params.setQuery( urlQuery );

    QByteArray data;
    data.append( params.query( QUrl::EncodeReserved ) );

    QNetworkRequest request( QUrl( "http://deckstats.net/index.php" ) );
    request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
    
    mNetworkAccessManager->post( request, data );
}


void
DeckStatsLauncher::queryFinished( QNetworkReply* reply )
{
    // deleteLater the reply (required by Qt) and this object itself when
    // this method exits.
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyScopedPtr( reply );
    QScopedPointer<DeckStatsLauncher, QScopedPointerDeleteLater> thisScopedPtr( this );

    if( reply->error() != QNetworkReply::NoError )
    {
        QMessageBox::critical( 0, tr("Error"), reply->errorString() );
        return;
    }
        
    QString data( reply->readAll() );
    
    QRegExp regex( "<meta property=\"og:url\" content=\"([^\"]+)\"/>" );
    if( !regex.indexIn(data) )
    {
        QMessageBox::critical( 0, tr("Error"), tr("The reply from the server could not be parsed.") );
        return;
    }
    
    QString deckUrl = regex.cap(1);
    QDesktopServices::openUrl( deckUrl );
}

