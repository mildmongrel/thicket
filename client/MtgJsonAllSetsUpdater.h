#ifndef MTGJSONALLSETSUPDATECHECKER_H
#define MTGJSONALLSETSUPDATECHECKER_H

#include "AllSetsUpdater.h"
#include <QUrl>
#include <QFuture>
#include <QFutureWatcher>
#include "Logging.h"

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QTemporaryFile;
QT_END_NAMESPACE

class ClientSettings;
class MtgJsonAllSetsFileCache;
class MtgJsonAllSetsData;

#include "clienttypes.h"
#include "AllSetsUpdateChannel.h"

#include "rapidjson/document.h"

struct AllSetsDataInfo
{
    QString version;
    QString channelName;
};

class MtgJsonAllSetsUpdater : public AllSetsUpdater
{
    Q_OBJECT

public:
    MtgJsonAllSetsUpdater( ClientSettings*          clientSettings,
                                 MtgJsonAllSetsFileCache* mtgJsonAllSetsFileCache,
                                 const Logging::Config&   loggingConfig = Logging::Config(),
                                 QObject*                 parent = 0 );
    virtual ~MtgJsonAllSetsUpdater();

    // Start a query to check for an mtgjson update in the current channel.
    // Will pop up dialogs to proceed through the update if one is found.
    // Unless the background flag is set, will also pop up a dialog if no
    // update is required or if any errors are detected.
    virtual void start( bool background = false );

signals:
    void allSetsUpdated( const AllSetsDataSharedPtr& allSetsDataSptr );
                  //const AllSetsDataInfo&      allSetsDataInfo ); // add this later
    void finished();

private slots:
    void checkCanceled();
    void checkFinished();

    void downloadCanceled();
    void downloadReadyRead();
    void downloadProgress( qint64 bytesReceived, qint64 bytesTotal );
    void downloadFinished();

    void parsingFinished();

private:

    struct UpdateInfo
    {
        bool    updateAvailable;
        QString updateVersion;
        QUrl    downloadUrl;
    };

    // Returns download URL if data OK and user chooses to update
    bool processCheckResponseData( const QString& data, UpdateInfo* updateInfo, QString* errorStr );

    bool processStableChannelResponse( const rapidjson::Document& doc, UpdateInfo* updateInfo );
    bool processMtgJsonChannelResponse( const rapidjson::Document& doc, UpdateInfo* updateInfo );

    void startDownload( QUrl& url );
    void finishAndReset();

    ClientSettings*          mClientSettings;
    MtgJsonAllSetsFileCache* mMtgJsonAllSetsFileCache;
    QNetworkAccessManager*   mNetworkAccessManager;

    bool                              mUpdateInProgress;
    bool                              mBackground;
    AllSetsUpdateChannel::ChannelType mUpdateChannel;
    QString                           mCurrentAllSetsVersion;
    QString                           mUpdateVersion;

    // Check stage variables.
    //
    QProgressDialog*  mCheckProgressDialog;
    QNetworkReply*    mCheckNetworkReply;

    QProgressDialog*  mUpdateProgressDialog;

    // Download stage variables.
    //
    QTemporaryFile*   mTmpFile;
    QNetworkReply*    mDownloadNetworkReply;
    bool              mDownloadAborted;

    // Parse stage variables.
    //
    MtgJsonAllSetsData*  mParseAllSetsDataPtr;
    FILE*                mParseFile;
    QFuture<bool>        mParseFuture;
    QFutureWatcher<bool> mParseFutureWatcher;

    // Logging variables.
    //
    Logging::Config mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif

