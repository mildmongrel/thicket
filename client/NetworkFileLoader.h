#ifndef NETWORKIMAGELOADER_H
#define NETWORKIMAGELOADER_H

#include <QObject>

#include <QMap>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QUrl;
QT_END_NAMESPACE

#include "Logging.h"

class NetworkFileLoader : public QObject
{
    Q_OBJECT

public:
    NetworkFileLoader( Logging::Config loggingConfig = Logging::Config(),
                       QObject*        parent = 0 );

    void loadFile( const QUrl& url, const QVariant& token );

signals:
    void fileLoaded( const QVariant& token, const QByteArray& bytes );

private slots:
    void networkAccessFinished( QNetworkReply *reply );

private:

    QNetworkAccessManager*        mNetworkAccessManager;
    QMap<QNetworkReply*,QVariant> mReplyToTokenMap;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // NETWORKIMAGELOADER_H
