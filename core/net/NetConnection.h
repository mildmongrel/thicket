#ifndef NETCONNECTION_H
#define NETCONNECTION_H

#include <QTcpSocket>
#include "Logging.h"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class NetConnection : public QTcpSocket
{
    Q_OBJECT

public:

    // Compression mode when sending data.
    enum CompressionMode
    {
        COMPRESSION_MODE_AUTO,
        COMPRESSION_MODE_COMPRESSED,
        COMPRESSION_MODE_UNCOMPRESSED,
    };

    NetConnection( const Logging::Config& loggingConfig = Logging::Config(), QObject* parent = 0 );

    // Set a receive inactivity abort time.  If nothing has been received
    // within the inactivity time the socket connection will be aborted.
    // Set to 0 to disable.
    void setRxInactivityAbortTime( int inactivityMillis );

    // Set compression mode (for testing).  Default mode is COMPRESSION_AUTO.
    void setCompressionMode( CompressionMode compressionMode ) { mCompressionMode = compressionMode; }

    // Send message.  Returns true if message was sent entirely.
    bool sendMsg( const QByteArray& byteArray );

    uint64_t getBytesSent() const { return mBytesSent; }
    uint64_t getBytesReceived() const { return mBytesReceived; }

signals:

    void msgReceived( const QByteArray& byteArray );

private slots:

    void handleReadyRead();
    void handleRxInactivityAbortTimerTimeout();

private:

    NetConnection();
    NetConnection& operator=( const NetConnection& n );

    void restartRxInactivityAbortTimer();

    QTimer* mRxInactivityAbortTimer;
    int mRxInactivityAbortTimeMillis;

    CompressionMode mCompressionMode;

    quint16 mIncomingMsgHeader;

    uint64_t mBytesSent;
    uint64_t mBytesReceived;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
