#ifndef ADMINSHELL_H
#define ADMINSHELL_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

class ClientNotices;

#include "Logging.h"

class AdminShell : public QObject
{
    Q_OBJECT

public:
    AdminShell( const std::shared_ptr<ClientNotices>& clientNotices,
                const Logging::Config&                loggingConfig = Logging::Config(),
                QObject*                              parent = 0 );

public slots:

    void start();

signals:

    void finished();

private slots:

    void handleNewConnection();
    void handleSocketReadyRead();
    void handleSocketDisconnected();

private:

    bool writeToSocket( QString str );
    bool writePrompt();
    void processCommand( const QString& tokens );

    const std::shared_ptr<ClientNotices> mClientNotices;
    QLocalServer*                        mServer;
    QLocalSocket*                        mConnection;

    const Logging::Config                mLoggingConfig;
    std::shared_ptr<spdlog::logger>      mLogger;
};

#endif
