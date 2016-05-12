#ifndef MTGJSONALLSETSUPDATEDIALOG_H
#define MTGJSONALLSETSUPDATEDIALOG_H

#include "AllSetsUpdateDialog.h"

#include <QUrl>
#include <QFuture>
#include <QFutureWatcher>
#include "Logging.h"
#include "clienttypes.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QTemporaryFile;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
QT_END_NAMESPACE

class MtgJsonAllSetsData;
class MtgJsonAllSetsFileCache;

class MtgJsonAllSetsUpdateDialog : public AllSetsUpdateDialog
{
    Q_OBJECT

public:
    MtgJsonAllSetsUpdateDialog( const QString&           defaultUrl,
                                MtgJsonAllSetsFileCache* cache,
                                const Logging::Config&   loggingConfig = Logging::Config(),
                                QWidget*                 parent = 0 );
    ~MtgJsonAllSetsUpdateDialog() {}
    AllSetsDataSharedPtr getAllSetsData() const { return mAllSetsDataSptr; }

private slots:
    void update();
    void cancel();
    void replyReadyRead();
    void replyDownloadProgress( qint64 bytesReceived, qint64 bytesTotal );
    void downloadCanceled();
    void replyFinished();
    void parsingFinished();

private:
    void startDownload();
    void resetState();

    MtgJsonAllSetsFileCache* mCache;

    QLineEdit*    mUrlLineEdit;
    QLabel*       mStatusLabel;
    QPushButton*  mUpdateButton;
    QPushButton*  mCancelButton;

    QUrl            mUrl;
    QTemporaryFile* mTmpFile;

    QNetworkAccessManager* mNetworkAccessManager;
    QNetworkReply*         mNetworkReply;
    bool                   mNetworkReplyAborted;

    QProgressDialog* mProgressDialog;

    MtgJsonAllSetsData*  mParseAllSetsDataPtr;
    FILE*                mParseFile;
    QFuture<bool>        mParseFuture;
    QFutureWatcher<bool> mParseFutureWatcher;

    AllSetsDataSharedPtr mAllSetsDataSptr;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // MTGJSONALLSETSUPDATEDIALOG_H
