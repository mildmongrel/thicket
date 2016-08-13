#include "ClientUpdateChecker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QProgressDialog>
#include <QUrl>

#include "qtutils_core.h"        // for logging QString
#include "WebServerInterface.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

ClientUpdateChecker::ClientUpdateChecker( const Logging::Config& loggingConfig,
                                          QObject*               parent )
  : QObject( parent ),
    mQueryStarted( false ),
    mProgressDialog( nullptr ),
    mNetworkReply( nullptr ),
    mLogger( loggingConfig.createLogger() )
{
    mNetworkAccessManager = new QNetworkAccessManager( this );
    connect( mNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(queryFinished(QNetworkReply*)) );
}


ClientUpdateChecker::~ClientUpdateChecker()
{
    // Possible that progress dialog was created without a parent.
    if( mProgressDialog ) mProgressDialog->deleteLater();
}


void
ClientUpdateChecker::check( const QString& webServiceBaseUrl,
                            const QString& clientCurrentVersion,
                            bool           background )
{
    if( mQueryStarted ) return;

    // If parent is a widget, progress dialog will be modal to it.  If not
    // then dialog will be constructed with a null parent.
    QWidget* parentWidget = qobject_cast<QWidget*>( parent() );

    mProgressDialog = new QProgressDialog( parentWidget );
    mProgressDialog->setWindowModality( Qt::WindowModal );
    mProgressDialog->setWindowTitle( "Client Update" );
    mProgressDialog->setLabelText( "Checking for client update..." );
    mProgressDialog->setMinimumDuration( 1000 );
    mProgressDialog->setValue( 0 );
    mProgressDialog->setRange( 0, 0 );  // do this *after* setting minDur and
                                        // val to get undetermined duration look

    // Handle cancel if user presses cancel or closes the dialog.
    connect( mProgressDialog, SIGNAL(canceled()), this, SLOT(cancel()) );
    connect( mProgressDialog, SIGNAL(finished(int)), this, SLOT(cancel()) );

    mQueryStarted = true;
    mBackground = background;
    QUrl url( WebServerInterface::getClientUpdateApiUrl( webServiceBaseUrl, clientCurrentVersion ) );
    QNetworkRequest request( url );
    mLogger->debug( "checking for client update at {}", url.toString() );
    mNetworkReply = mNetworkAccessManager->get( request );
}


void
ClientUpdateChecker::cancel()
{
    mLogger->debug( "update canceled" );
    mNetworkReply->abort();
}


void
ClientUpdateChecker::queryFinished( QNetworkReply* reply )
{
    // deleteLater the reply (required by Qt) and this object itself when
    // this method exits.
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyScopedPtr( reply );
    QScopedPointer<ClientUpdateChecker, QScopedPointerDeleteLater> thisScopedPtr( this );

    mProgressDialog->reset();

    if( reply->error() == QNetworkReply::NoError )
    {
        QString data( reply->readAll() );
        mLogger->info( "update response --> {}", data );
        processResponseData( data );
    }
    else
    {
        mLogger->warn( "network error: {}", reply->errorString() );
        if( !mBackground )
        {
            QMessageBox::critical( 0, tr("Network Error"), reply->errorString() );
        }
    }
        
    emit finished();
}


void
ClientUpdateChecker::processResponseData( const QString& data )
{
    // Parse JSON response.
    mLogger->debug( "parsing json" );
    rapidjson::Document doc;
    doc.Parse( data.toStdString().c_str() );

    if( doc.HasParseError() )
    {
        mLogger->warn( "error parsing JSON reply from server (offset {}): {}",
                doc.GetErrorOffset(), rapidjson::GetParseError_En( doc.GetParseError() ) );
        if( !mBackground )
        {
            QMessageBox::critical( 0, tr("Parsing Error"), tr("The reply from the server could not be parsed.") );
        }
        return;
    }

    // Extract JSON information and pop up dialog as needed.
    if( doc.HasMember( "version" ) )
    {
        const rapidjson::Value& versionVal = doc["version"];
        if( versionVal.IsNull() )
        {
            // no update required
            mLogger->info( "client up to date" );
            if( !mBackground )
            {
                QMessageBox::information( 0, tr("Check for Update"), tr("The client software is up to date.") );
            }
        }
        else if( versionVal.IsString() )
        {
            // update available
            QString version = QString::fromStdString( versionVal.GetString() );
            if( doc.HasMember( "site_url" ) )
            {
                const rapidjson::Value& siteUrlVal = doc["site_url"];
                if( siteUrlVal.IsString() )
                {
                    QString siteUrl = QString::fromStdString( siteUrlVal.GetString() );
                    mLogger->info( "client update available: version={} site_url={}", version, siteUrl );

                    QMessageBox::information( 0, tr("Update Available"),
                            tr("Updated version %1 is available.<p>You can download this version using the link:"
                               "<br><a href=\"%2\">%2</a>").arg( version ).arg( siteUrl ) );
                }
                else
                {
                    mLogger->notice( "JSON reply from server has invalid 'site_url' key" );
                }
            }
            else
            {
                mLogger->notice( "JSON reply from server missing 'site_url' key" );
            }
        }
        else
        {
            mLogger->notice( "JSON reply from server has invalid 'version' key" );
        }
    }
    else
    {
        mLogger->notice( "JSON reply from server missing 'version' key" );
    }
}
