#ifndef CLIENTUPDATECHECKER_H
#define CLIENTUPDATECHECKER_H

#include <QObject>
#include "Logging.h"

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
QT_END_NAMESPACE

class ClientUpdateChecker : public QObject
{
    Q_OBJECT

public:
    ClientUpdateChecker( const Logging::Config& loggingConfig = Logging::Config(),
                         QObject*               parent = 0 );
    ~ClientUpdateChecker();

    // Start a query to check for a client update.  Will pop up a dialog
    // with an update link if an update is found.  Unless the background
    // flag is set, will also pop up a dialog if no update is required or if
    // any errors are detected.
    // After completion this object will delete itself.
    void check( const QString& clientUpdateApiUrl,
                const QString& currentClientVersion,
                bool           background = false );

signals:
    void finished();   

private slots:
    void cancel();
    void queryFinished( QNetworkReply* reply );

private:
    void processResponseData( const QString& data );

    QNetworkAccessManager* mNetworkAccessManager;
    bool                   mQueryStarted;
    bool                   mBackground;
    QProgressDialog*       mProgressDialog;
    QNetworkReply*         mNetworkReply;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif

