#include "MtgJsonAllSetsUpdater.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QProgressDialog>
#include <QUrl>
#include <QTemporaryFile>
#include <QtConcurrent>

#include "qtutils_core.h"        // for logging QString

#include "version.h"
#include "ClientSettings.h"
#include "WebServerInterface.h"
#include "MtgJsonAllSetsFileCache.h"
#include "MtgJsonAllSetsData.h"

#include "rapidjson/error/en.h"

static const QString CHANNEL_STABLE  = "stable";
static const QString CHANNEL_MTGJSON = "mtgjson";

MtgJsonAllSetsUpdater::MtgJsonAllSetsUpdater( ClientSettings*          clientSettings,
                                              MtgJsonAllSetsFileCache* mtgJsonAllSetsFileCache,
                                              const Logging::Config&   loggingConfig,
                                              QObject*                 parent )
  : AllSetsUpdater( parent ),
    mClientSettings( clientSettings ),
    mMtgJsonAllSetsFileCache( mtgJsonAllSetsFileCache ),
    mUpdateInProgress( false ),
    mCheckProgressDialog( nullptr ),
    mCheckNetworkReply( nullptr ),
    mUpdateProgressDialog( nullptr ),
    mTmpFile( nullptr ),
    mDownloadNetworkReply( nullptr ),
    mDownloadAborted( false ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    mNetworkAccessManager = new QNetworkAccessManager( this );

    connect( &mParseFutureWatcher, SIGNAL(finished()), this, SLOT(parsingFinished()) );
}


MtgJsonAllSetsUpdater::~MtgJsonAllSetsUpdater()
{
    // Possible that progress dialog was created without a parent.
    if( mCheckProgressDialog ) mCheckProgressDialog->deleteLater();
}


void
MtgJsonAllSetsUpdater::start( bool background )
{
    if( mUpdateInProgress ) return;
    mUpdateInProgress = true;

    // If parent is a widget, progress dialog will be modal to it.  If not
    // then dialog will be constructed with a null parent.
    QWidget* parentWidget = qobject_cast<QWidget*>( parent() );

    mCheckProgressDialog = new QProgressDialog( parentWidget );
    mCheckProgressDialog->setWindowModality( Qt::WindowModal );
    mCheckProgressDialog->setWindowTitle( "MtgJson Update" );
    mCheckProgressDialog->setLabelText( "Checking for MtgJSON AllSets update..." );
    mCheckProgressDialog->setMinimumDuration( 1000 );
    mCheckProgressDialog->setValue( 0 );
    mCheckProgressDialog->setRange( 0, 0 );  // do this *after* setting minDur and
                                             // val to get undetermined duration look

    // Handle cancel if user presses cancel or closes the dialog.
    connect( mCheckProgressDialog, SIGNAL(canceled()), this, SLOT(checkCanceled()) );
    connect( mCheckProgressDialog, SIGNAL(finished(int)), this, SLOT(checkCanceled()) );

    // Set some variables that will last through this update sequence.
    mBackground = background;
    mUpdateChannel = mClientSettings->getAllSetsUpdateChannel();
    mCurrentAllSetsVersion = mMtgJsonAllSetsFileCache->getCachedFileVersion( mUpdateChannel );

    QUrl url;
    if( mUpdateChannel == AllSetsUpdateChannel::CHANNEL_STABLE )
    {
        const QString webServiceBaseUrl = mClientSettings->getWebServiceBaseUrl();
        const QString clientVersion = QString::fromStdString( gClientVersion );
        url = QUrl( WebServerInterface::getMtgJsonAllSetsUpdateApiUrl( webServiceBaseUrl, clientVersion, mCurrentAllSetsVersion ) );
    }
    else if( mUpdateChannel == AllSetsUpdateChannel::CHANNEL_MTGJSON )
    {
        url = QUrl( "http://www.mtgjson.com/json/version-full.json" );
    }
    else
    {
        mLogger->notice( "unknown channel '{}'", AllSetsUpdateChannel::channelToString( mUpdateChannel ) );
        finishAndReset();
        return;
    }
            
    QNetworkRequest request( url );
    mLogger->debug( "checking for mtgjson allsets update on channel '{}' at {}",
            AllSetsUpdateChannel::channelToString( mUpdateChannel ), url.toString() );
    mCheckNetworkReply = mNetworkAccessManager->get( request );
    connect( mCheckNetworkReply, SIGNAL(finished()), this, SLOT(checkFinished()) );
}


void
MtgJsonAllSetsUpdater::finishAndReset()
{
    mUpdateInProgress = false;

    mDownloadAborted = false;

    if( mDownloadNetworkReply )
    {
        mDownloadNetworkReply->deleteLater();
        mDownloadNetworkReply = nullptr;
    }

    if( mUpdateProgressDialog )
    {
        mUpdateProgressDialog->close();
        mUpdateProgressDialog->deleteLater();
        mUpdateProgressDialog = nullptr;
    }

    if( mTmpFile )
    {
        mTmpFile->deleteLater();
        mTmpFile = nullptr;
    }

    emit finished();
}


void
MtgJsonAllSetsUpdater::checkCanceled()
{
    mLogger->debug( "update check canceled" );
    if( mCheckNetworkReply )
    {
        mCheckNetworkReply->abort();
    }
}


void
MtgJsonAllSetsUpdater::checkFinished()
{
    bool downloadStarted = false;

    // deleteLater the reply (required by Qt) when this method exits.
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyScopedPtr( mCheckNetworkReply );

    mCheckProgressDialog->reset();
    mCheckProgressDialog->deleteLater();

    if( mCheckNetworkReply->error() == QNetworkReply::NoError )
    {
        QString data( mCheckNetworkReply->readAll() );
        mLogger->debug( "update response --> {}", data );
        UpdateInfo updateInfo;
        QString errorStr;
        bool responseOk = processCheckResponseData( data, &updateInfo, &errorStr );

        if( responseOk )
        {
            if( updateInfo.updateAvailable )
            {
                mLogger->info( "mtgjson allsets update available: version={} download_url={}", updateInfo.updateVersion, updateInfo.downloadUrl.toString() );

                QMessageBox::StandardButton answer = QMessageBox::question( 0, tr("Update Available"),
                        tr("Updated MTG JSON AllSets data version %1 is available.<p>Would you like to download it?").arg( updateInfo.updateVersion ),
                        QMessageBox::Yes | QMessageBox::No );
                if( answer == QMessageBox::Yes )
                {
                    // User wants to update.  Start the download process.
                    startDownload( updateInfo.downloadUrl );
                    mUpdateVersion = updateInfo.updateVersion;
                    downloadStarted = true;
                }
            }
            else
            {
                // No update available.
                mLogger->info( "mtgjson allsets data up to date (version {})", mCurrentAllSetsVersion );
                if( !mBackground )
                {
                    QMessageBox::information( 0, tr("Check for Update"), tr("MTG JSON AllSets data is up to date.") );
                }
            }
        }
        else
        {
            // Failed to process response.
            if( !mBackground )
            {
                QMessageBox::critical( 0, tr("Update Error"), errorStr );
            }
        }
    }
    else
    {
        mLogger->warn( "network error: {}", mCheckNetworkReply->errorString() );
        if( !mBackground )
        {
            QMessageBox::critical( 0, tr("Network Error"), mCheckNetworkReply->errorString() );
        }
    }
        
    // If we didn't start a download then this is the end.
    if( !downloadStarted )
    {
        finishAndReset();
    }
}


bool
MtgJsonAllSetsUpdater::processCheckResponseData( const QString& data, UpdateInfo* updateInfo, QString* errorStr )
{
    bool result = false;
    const QString parseErrorStr( tr("The reply from the server could not be parsed.") );

    // Parse JSON response.
    mLogger->debug( "parsing json" );
    rapidjson::Document doc;
    doc.Parse( data.toStdString().c_str() );

    if( doc.HasParseError() )
    {
        mLogger->warn( "error parsing JSON reply from server (offset {}): {}",
                doc.GetErrorOffset(), rapidjson::GetParseError_En( doc.GetParseError() ) );
        *errorStr = parseErrorStr;
    }
    else if( mUpdateChannel == AllSetsUpdateChannel::CHANNEL_STABLE )
    {
        result = processStableChannelResponse( doc, updateInfo );
        if( !result )
        {
            *errorStr = parseErrorStr;
        }
    }
    else if( mUpdateChannel == AllSetsUpdateChannel::CHANNEL_MTGJSON )
    {
        result = processMtgJsonChannelResponse( doc, updateInfo );
        if( !result )
        {
            *errorStr = parseErrorStr;
        }
    }
    else
    {
        mLogger->notice( "unknown channel '{}'", AllSetsUpdateChannel::channelToString( mUpdateChannel ) );
        *errorStr = tr("Unknown MTG JSON update channel.");
    }

    return result;
}


bool
MtgJsonAllSetsUpdater::processStableChannelResponse( const rapidjson::Document& doc, UpdateInfo* updateInfo )
{
    if( doc.HasMember( "version" ) )
    {
        const rapidjson::Value& versionVal = doc["version"];
        if( versionVal.IsNull() )
        {
            // no update required
            mLogger->info( "mtgjson allsets up to date" );
            updateInfo->updateAvailable = false;
            return true;
        }
        else if( versionVal.IsString() )
        {
            // update available
            QString version = QString::fromStdString( versionVal.GetString() );
            if( doc.HasMember( "download_url" ) )
            {
                const rapidjson::Value& downloadUrlVal = doc["download_url"];
                if( downloadUrlVal.IsString() )
                {
                    updateInfo->updateAvailable = true;
                    updateInfo->updateVersion = version;
                    updateInfo->downloadUrl = QUrl( QString::fromStdString( downloadUrlVal.GetString() ) );
                    return true;
                }
                else
                {
                    mLogger->notice( "JSON reply from server has invalid 'download_url' key" );
                }
            }
            else
            {
                mLogger->notice( "JSON reply from server missing 'download_url' key" );
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

    return false;
}


// Part of a simple version comparison scheme.
// Takes QString in form of "X.Y.Z" and returns (X<<20)|(Y<<10)|Z.
// Returns -1 on parse error.
static int getVersionMagnitude( QString version )
{
    int val = 0;
    bool ok;

    QStringList parts = version.split('.');
    if( parts.size() != 3 ) return -1;
    
    val |= parts[0].toUInt( &ok ) << 20;
    if( !ok ) return -1;
    val |= parts[1].toUInt( &ok ) << 10;
    if( !ok ) return -1;
    val |= parts[2].toUInt( &ok );
    if( !ok ) return -1;

    return val;
}


bool
MtgJsonAllSetsUpdater::processMtgJsonChannelResponse( const rapidjson::Document& doc, UpdateInfo* updateInfo )
{
    if( doc.HasMember( "version" ) )
    {
        const rapidjson::Value& versionVal = doc["version"];
        if( versionVal.IsString() )
        {
            QString versionStr = QString::fromStdString( versionVal.GetString() );

            int currVer = getVersionMagnitude( mCurrentAllSetsVersion );
            int compVer = getVersionMagnitude( versionStr );

            if( compVer >= 0 )
            {
                // Update available if new version greater than current.
                if( compVer > currVer )
                {
                    updateInfo->updateAvailable = true;
                    updateInfo->updateVersion = versionStr;
                    updateInfo->downloadUrl = QUrl( "http://mtgjson.com/json/AllSets.json" );
                }
                else
                {
                    updateInfo->updateAvailable = false;
                }
                return true;
            }
            else
            {
                mLogger->notice( "parse error in JSON version from server: {}", versionStr );
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

    return false;
}


void
MtgJsonAllSetsUpdater::startDownload( QUrl& url )
{
    mTmpFile = new QTemporaryFile( this );
    if( !mTmpFile->open() )
    {
        QMessageBox::critical( 0, tr("Download Error"),
                tr("Unable to save to file %1: %2.")
                .arg( mTmpFile->fileName() ).arg( mTmpFile->errorString() ) );
        finishAndReset();
    };

    QNetworkRequest req( url );
    mLogger->debug( "starting AllSets download: {}", req.url().toString() );
    mDownloadNetworkReply = mNetworkAccessManager->get( req );
    connect( mDownloadNetworkReply, &QNetworkReply::readyRead, this, &MtgJsonAllSetsUpdater::downloadReadyRead );
    connect( mDownloadNetworkReply, &QNetworkReply::downloadProgress, this, &MtgJsonAllSetsUpdater::downloadProgress );
    connect( mDownloadNetworkReply, &QNetworkReply::finished, this, &MtgJsonAllSetsUpdater::downloadFinished );

    // If parent is a widget, progress dialog will be modal to it.  If not
    // then dialog will be constructed with a null parent.
    QWidget* parentWidget = qobject_cast<QWidget*>( parent() );

    // Set up dialog with "busy" look.
    mUpdateProgressDialog = new QProgressDialog( parentWidget );
    mUpdateProgressDialog->setWindowModality( Qt::WindowModal );
    mUpdateProgressDialog->setWindowTitle( tr("Updating Card Data") );
    mUpdateProgressDialog->setLabelText( tr("Downloading...") );
    mUpdateProgressDialog->setAutoReset( false );
    mUpdateProgressDialog->setMinimum( 0 );
    mUpdateProgressDialog->setMaximum( 0 );
    connect( mUpdateProgressDialog, SIGNAL(canceled()), this, SLOT(downloadCanceled()) );
    mUpdateProgressDialog->show();
}


void
MtgJsonAllSetsUpdater::downloadCanceled()
{
    if( mDownloadNetworkReply )
    {
        mDownloadAborted = true;
        mDownloadNetworkReply->abort();
    }
}


void
MtgJsonAllSetsUpdater::downloadReadyRead()
{
    // Read data into the file.  Done here rather than at the
    // networkAccessFinished() to avoid big RAM usage since the file can
    // get large.
    if( mDownloadNetworkReply )
    {
        mLogger->trace( "downloadReadyRead: {} bytes ready", mDownloadNetworkReply->size() );
        if( mTmpFile )
        {
            mLogger->trace( "downloadReadyRead: writing to file", mDownloadNetworkReply->size() );
            mTmpFile->write( mDownloadNetworkReply->readAll() );
        }
    }
}


void
MtgJsonAllSetsUpdater::downloadProgress( qint64 bytesReceived, qint64 bytesTotal )
{
    mLogger->trace( "downloadProgress: {}/{}", bytesReceived, bytesTotal );

    if( mUpdateProgressDialog )
    {
        if( bytesTotal < 0 )
        {
            if( bytesReceived >= 0 )
            {
                mUpdateProgressDialog->setLabelText( tr("Downloading: %1").arg( QString::number( bytesReceived ) ) );
            }
        }
        else
        {
            if( bytesReceived > 0 )
            {
                mUpdateProgressDialog->setMaximum( 100 );
                int progress = (bytesReceived * 100) / bytesTotal;
                mUpdateProgressDialog->setValue( progress );
                mUpdateProgressDialog->setLabelText( tr("Downloading: %1 / %2")
                        .arg( QString::number( bytesReceived ) )
                        .arg( QString::number( bytesTotal ) ) );
            }
        }
    }
}


void
MtgJsonAllSetsUpdater::downloadFinished()
{
    mLogger->trace( "downloadFinished" );

    // From the Qt docs:
    // Note: After the request has finished, it is the responsibility of the
    // user to delete the QNetworkReply object at an appropriate time. Do not
    // directly delete it inside the slot connected to finished(). You can use
    // the deleteLater() function.
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyScopedPtr( mDownloadNetworkReply );

    mTmpFile->flush();
    mTmpFile->close();

    // Handle cancellation during download.
    if( mDownloadAborted )
    {
        mLogger->debug( "downloadFinished: aborted" );
        finishAndReset();
        return;
    }

    // Handle network error.
    if( mDownloadNetworkReply->error() ) {
        mLogger->debug( "downloadFinished: error: {}", mDownloadNetworkReply->errorString() );
        QMessageBox::critical( 0, tr("Error"),
                tr("Download error: %1").arg( mDownloadNetworkReply->errorString() ) );
        finishAndReset();
        return;
    }

    // Handle network redirect.
    QVariant redirectionTarget = mDownloadNetworkReply->attribute( QNetworkRequest::RedirectionTargetAttribute );
    if( !redirectionTarget.isNull() )
    {
        QUrl newUrl = mDownloadNetworkReply->url().resolved( redirectionTarget.toUrl() );
        mLogger->debug( "replyFinished: redirecting to : {}", newUrl.toString() );

        // Erase and re-open the file.
        mTmpFile->resize( 0 );
        mTmpFile->open();

        startDownload( newUrl );
        return;
    }

    // Done with reply, nullify local pointer so that a cancel doesn't access
    // a bad pointer when the actual object is deleted later by the QScopedPointer.
    mDownloadNetworkReply = nullptr;

    // Reset dialog for new stage with "busy" look.
    mUpdateProgressDialog->setLabelText( tr("Parsing...") );
    mUpdateProgressDialog->setValue( 0 );
    mUpdateProgressDialog->setMinimum( 0 );
    mUpdateProgressDialog->setMaximum( 0 );

    const std::string allSetsFilePath = mTmpFile->fileName().toStdString();
    mLogger->debug( "parsing AllSets file at {}", allSetsFilePath );
    mParseFile = fopen( allSetsFilePath.c_str(), "r" );
    if( mParseFile == NULL )
    {
        mLogger->warn( "failed to open AllSets file at {}", mTmpFile->fileName() );
        QMessageBox::warning( 0, tr("Error"),
                tr("Unable to open file %1").arg( mTmpFile->fileName() ) );
        finishAndReset();
        return;
    }

    // Kick off the parsing in another thread to keep UI responsive.
    mParseAllSetsDataPtr = new MtgJsonAllSetsData( mLoggingConfig.createChildConfig( "mtgjson" ) );
    mParseFuture = QtConcurrent::run( mParseAllSetsDataPtr, &MtgJsonAllSetsData::parse, mParseFile );
    mParseFutureWatcher.setFuture( mParseFuture );
}


void
MtgJsonAllSetsUpdater::parsingFinished()
{
    AllSetsDataSharedPtr allSetsDataSptr;

    mLogger->trace( "parsingFinished" );
    fclose( mParseFile );
    mUpdateProgressDialog->setMaximum( 1 );
    if( mParseFuture.result() )
    {
        mLogger->debug( "parsing succeeded" );
        // Set dialog to 100%.
        mUpdateProgressDialog->setValue( 1 );
        allSetsDataSptr.reset( mParseAllSetsDataPtr );
    }
    else
    {
        mLogger->debug( "parsing failed" );
        delete mParseAllSetsDataPtr;

        QMessageBox::warning( 0, tr("Error"),
                tr("Failed to parse downloaded file!") );
        finishAndReset();
        return;
    }

    // File is parsed successfully, commit to cache.
    bool cached = false;
    if( mMtgJsonAllSetsFileCache != nullptr )
    {
        cached = mMtgJsonAllSetsFileCache->commit( mUpdateChannel, mTmpFile->fileName(), mUpdateVersion );
    }

    if( cached )
    {
        QMessageBox::information( 0, tr("Success"),
                tr("Update successful.") );
    }
    else
    {
        mLogger->warn( "failed to cache AllSets file at {}", mTmpFile->fileName() );
        QMessageBox::warning( 0, tr("Card Data Not Saved"),
                tr("Unable to save card data to storage location.\n\nDownloaded card data will be used for the rest of this session only.") );
    }

    // Signal the successful update.
    emit allSetsUpdated( allSetsDataSptr );

    // This will clean up the temporary file amongst other things.
    finishAndReset();
}
