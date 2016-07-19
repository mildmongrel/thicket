#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <QTcpSocket>
#include "messages.pb.h"
#include "Logging.h"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class ClientConnection : public QTcpSocket
{
    Q_OBJECT

public:

    ClientConnection( const Logging::Config& loggingConfig = Logging::Config(), QObject* parent = 0 );

    bool sendMsg( const proto::ServerToClientMsg* const protoMsg );

    uint64_t getBytesSent() const { return mBytesSent; }
    uint64_t getBytesReceived() const { return mBytesReceived; }

signals:
    void msgReceived( const proto::ClientToServerMsg* const protoMsg );

private slots:
    void handleReadyRead();
    void handleAbortTimerTimeout();

private:

    void restartAbortTimer();

    quint16 mIncomingMsgHeader;

    uint64_t mBytesSent;
    uint64_t mBytesReceived;

    QTimer* mAbortTimer;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
