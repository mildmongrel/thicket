#include "MtgJsonAllSetsUpdateDialog.h"

#include <QLabel>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QTemporaryFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QMessageBox>
#include <QtConcurrent>
#include "qtutils_core.h"

#include "MtgJsonAllSetsData.h"
#include "MtgJsonAllSetsFileCache.h"

MtgJsonAllSetsUpdateDialog::MtgJsonAllSetsUpdateDialog( const QString&           defaultUrl,
                                                        MtgJsonAllSetsFileCache* cache,
                                                        const Logging::Config&   loggingConfig,
                                                        QWidget*                 parent )
  : mCache( cache ),
    mTmpFile( nullptr ),
    mNetworkReply( nullptr ),
    mProgressDialog( nullptr ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    setWindowTitle( tr("Update Card Data") );

    QLabel* infoLabel = new QLabel( tr("Thicket card data is provided by the 'AllSets.json' file from the MTG JSON project.\n") );

    mUrlLineEdit = new QLineEdit;
    mUrlLineEdit->setText( defaultUrl );
    QLabel* urlLabel = new QLabel( tr("&URL:") );
    urlLabel->setBuddy( mUrlLineEdit );

    mStatusLabel = new QLabel();

    mUpdateButton = new QPushButton( tr("Update") );
    mCancelButton = new QPushButton( tr("Cancel") );

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton( mUpdateButton, QDialogButtonBox::ActionRole );
    buttonBox->addButton( mCancelButton, QDialogButtonBox::RejectRole );
    mUpdateButton->setDefault( true );
    mUpdateButton->setFocus();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget( infoLabel,    0, 0, 1, 2 );
    mainLayout->addWidget( urlLabel,     1, 0 );
    mainLayout->addWidget( mUrlLineEdit, 1, 1 );
    mainLayout->addWidget( mStatusLabel, 2, 0, 1, 2 );
    mainLayout->addWidget( buttonBox,    3, 0, 1, 2 );
    mainLayout->setColumnMinimumWidth( 1, 250 );
    setLayout(mainLayout);

    connect( mUpdateButton, SIGNAL(clicked()), this, SLOT(update()) );
    connect( mCancelButton, SIGNAL(clicked()), this, SLOT(cancel()) );

    // This addresses a Qt oddity where a dialog forgets default buttons
    // after it finishes.  Since this dialog may be reused, reset the
    // default button.
    connect( this, &QDialog::finished,
             this, [this](int result) { mUpdateButton->setDefault( true ); } );

    mNetworkAccessManager = new QNetworkAccessManager(this);

    connect( &mParseFutureWatcher, SIGNAL(finished()), this, SLOT(parsingFinished()) );
}


void
MtgJsonAllSetsUpdateDialog::update()
{
    mStatusLabel->setText( tr("Downloading...") );
        
    mUpdateButton->setEnabled( false );

    mTmpFile = new QTemporaryFile( this );
    if( !mTmpFile->open() )
    {
        QMessageBox::warning( this, tr("Error"),
                tr("Unable to save to file %1: %2.")
                .arg( mTmpFile->fileName() ).arg( mTmpFile->errorString() ) );
        delete mTmpFile;
        mTmpFile = nullptr;
        reject();
    };

    mUrl = mUrlLineEdit->text();
    startDownload();
}


void
MtgJsonAllSetsUpdateDialog::cancel()
{
    reject();
    mStatusLabel->setText( QString() );
}


void
MtgJsonAllSetsUpdateDialog::downloadCanceled()
{
    if( mNetworkReply )
    {
        mNetworkReplyAborted = true;
        mNetworkReply->abort();
    }
}


void
MtgJsonAllSetsUpdateDialog::startDownload()
{
    QNetworkRequest req( mUrl );
    mLogger->debug( "starting AllSets download: {}", req.url().toString() );
    mNetworkReply = mNetworkAccessManager->get( req );
    mNetworkReplyAborted = false,
    connect( mNetworkReply, &QNetworkReply::readyRead, this, &MtgJsonAllSetsUpdateDialog::replyReadyRead );
    connect( mNetworkReply, &QNetworkReply::downloadProgress, this, &MtgJsonAllSetsUpdateDialog::replyDownloadProgress );
    connect( mNetworkReply, &QNetworkReply::finished, this, &MtgJsonAllSetsUpdateDialog::replyFinished );

    mProgressDialog = new QProgressDialog( this );
    mProgressDialog->setWindowModality( Qt::WindowModal );
    mProgressDialog->setWindowTitle( tr("Downloading Card Data") );
    mProgressDialog->setMinimum( 0 );
    // Set maximum to 0 for now to get "busy" look
    mProgressDialog->setMaximum( 0 );
    connect( mProgressDialog, SIGNAL(canceled()), this, SLOT(downloadCanceled()) );
    mProgressDialog->show();
}


void
MtgJsonAllSetsUpdateDialog::resetState()
{
    if( mTmpFile )
    {
        delete mTmpFile;
        mTmpFile = nullptr;
    }

    // Null out network reply pointer but do not delete; it must be
    // deleteLater()'d during networkFinished.
    mNetworkReply = nullptr;

    mUpdateButton->setEnabled( true );

    if( mProgressDialog )
    {
        mProgressDialog->close();
        delete mProgressDialog;
        mProgressDialog = nullptr;
    }
}


void
MtgJsonAllSetsUpdateDialog::replyReadyRead()
{
    // Read data into the file.  Done here rather than at the
    // networkAccessFinished() to avoid big RAM usage since the file can
    // get large.
    if( mNetworkReply )
    {
        mLogger->trace( "replyReadyRead: {} bytes ready", mNetworkReply->size() );
        if( mTmpFile )
        {
            mLogger->trace( "replyReadyRead: writing to file", mNetworkReply->size() );
            mTmpFile->write( mNetworkReply->readAll() );
        }
    }
}


void
MtgJsonAllSetsUpdateDialog::replyDownloadProgress( qint64 bytesReceived, qint64 bytesTotal )
{
    mLogger->trace( "replyDownloadProgress: {}/{}", bytesReceived, bytesTotal );

    if( mProgressDialog )
    {
        if( bytesTotal >= 0 )
        {
            mProgressDialog->setMaximum( bytesTotal );
        }
        if( bytesReceived >= 0 )
        {
            mProgressDialog->setValue( bytesReceived );
            mProgressDialog->setLabelText( tr("Downloading: %1").arg( QString::number( bytesReceived ) ) );
        }
    }
}


void
MtgJsonAllSetsUpdateDialog::replyFinished()
{
    mLogger->trace( "replyFinished" );

    // From the Qt docs:
    // Note: After the request has finished, it is the responsibility of the
    // user to delete the QNetworkReply object at an appropriate time. Do not
    // directly delete it inside the slot connected to finished(). You can use
    // the deleteLater() function.
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyScopedPtr( mNetworkReply );

    mTmpFile->flush();
    mTmpFile->close();

    // Handle cancellation during download.
    if( mNetworkReplyAborted )
    {
        mLogger->debug( "replyFinished: aborted" );
        mStatusLabel->setText( "Aborted!" );
        resetState();
        return;
    }

    // Handle network error.
    if( mNetworkReply->error() ) {
        mLogger->debug( "replyFinished: error: {}", mNetworkReply->errorString() );
        QMessageBox::warning( this, tr("Error"),
                tr("Download error: %1").arg( mNetworkReply->errorString() ) );
        mStatusLabel->setText( "Network Error!" );
        resetState();
        return;
    }

    // Handle network redirect.
    QVariant redirectionTarget = mNetworkReply->attribute( QNetworkRequest::RedirectionTargetAttribute );
    if( !redirectionTarget.isNull() )
    {
        QUrl newUrl = mUrl.resolved( redirectionTarget.toUrl() );
        mLogger->debug( "replyFinished: redirecting to : {}", newUrl.toString() );
        mUrl = newUrl;

        // Erase and re-open the file.
        mTmpFile->resize( 0 );
        mTmpFile->open();

        startDownload();
        return;
    }

    // Close the download progress dialog.
    if( mProgressDialog )
    {
        mProgressDialog->close();
    }

    // Done with reply, nullify local pointer.  Actual object will be
    // deleted later by the QScopedPointer.
    mNetworkReply = nullptr;
    mStatusLabel->setText( "Parsing..." );
    const std::string allSetsFilePath = mTmpFile->fileName().toStdString();
    mLogger->debug( "parsing AllSets file at {}", allSetsFilePath );
    mParseFile = fopen( allSetsFilePath.c_str(), "r" );
    if( mParseFile == NULL )
    {
        mLogger->warn( "failed to open AllSets file at {}", mTmpFile->fileName() );
        QMessageBox::warning( this, tr("Error"),
                tr("Unable to open file %1").arg( mTmpFile->fileName() ) );
        resetState();
        return;
    }

    // Kick off the parsing in another thread to keep UI responsive.
    mParseAllSetsDataPtr = new MtgJsonAllSetsData( mLoggingConfig.createChildConfig( "mtgjson" ) );
    mParseFuture = QtConcurrent::run( mParseAllSetsDataPtr, &MtgJsonAllSetsData::parse, mParseFile );
    mParseFutureWatcher.setFuture( mParseFuture );
}


void
MtgJsonAllSetsUpdateDialog::parsingFinished()
{
    mLogger->trace( "parsingFinished" );
    fclose( mParseFile );
    if( mParseFuture.result() )
    {
        mLogger->debug( "parsing succeeded" );
        mAllSetsDataSptr.reset( mParseAllSetsDataPtr );
    }
    else
    {
        mLogger->debug( "parsing failed" );
        delete mParseAllSetsDataPtr;

        QMessageBox::warning( this, tr("Error"),
                tr("Failed to parse downloaded file!") );
        mStatusLabel->setText( "Parsing Error!" );
        resetState();
        return;
    }

    // File is parsed successfully, commit to cache.
    bool cached = false;
    if( mCache != nullptr )
    {
        cached = mCache->commit( mTmpFile->fileName() );
    }
    if( !cached ) mLogger->warn( "failed to cache AllSets file at {}", mTmpFile->fileName() );

    // Done with the temporary file, make sure it's cleaned up.
    delete mTmpFile;
    mTmpFile = nullptr;

    if( cached )
    {
        QMessageBox::information( this, tr("Success"),
                tr("Update successful.") );
    }
    else
    {
        QMessageBox::warning( this, tr("Card Data Not Saved"),
                tr("Unable to save card data to storage location.\n\nDownloaded card data will be used for the rest of this session only.") );
    }

    mStatusLabel->setText( QString() );
    resetState();
    accept();
}
