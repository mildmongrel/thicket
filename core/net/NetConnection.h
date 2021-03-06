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

    // Compression mode when sending data.
    enum HeaderMode
    {
        HEADER_MODE_AUTO,
        HEADER_MODE_BRIEF,    // 2-byte header, 16K max payload
        HEADER_MODE_EXTENDED, // 6-byte header, 4TB max payload
    };

    NetConnection( const Logging::Config& loggingConfig = Logging::Config(), QObject* parent = 0 );

    // Set a receive inactivity abort time.  If nothing has been received
    // within the inactivity time the socket connection will be aborted.
    // Set to 0 to disable.
    void setRxInactivityAbortTime( int inactivityMillis );

    // Set compression mode (for testing).  Default mode is COMPRESSION_AUTO.
    void setCompressionMode( CompressionMode compressionMode ) { mCompressionMode = compressionMode; }

    // Set header mode (for testing).  Default mode is HEADER_AUTO.
    void setHeaderMode( HeaderMode headerMode ) { mHeaderMode = headerMode; }

    // Send message.  Returns true if message was sent entirely.
    bool sendMsg( const QByteArray& byteArray );

    uint64_t getBytesSent() const { return mBytesSent; }
    uint64_t getBytesReceived() const { return mBytesReceived; }

signals:

    void msgReceived( const QByteArray& byteArray );

private slots:

    void handleReadyRead();
    void handleRxInactivityAbortTimerTimeout();

protected:

    std::shared_ptr<spdlog::logger> mLogger;

private:

    NetConnection();
    NetConnection& operator=( const NetConnection& n );

    void restartRxInactivityAbortTimer();

    QTimer* mRxInactivityAbortTimer;
    int mRxInactivityAbortTimeMillis;

    CompressionMode mCompressionMode;
    HeaderMode      mHeaderMode;

    quint16 mIncomingMsgHeader;
    quint32 mExtendedLength;

    uint64_t mBytesSent;
    uint64_t mBytesReceived;
};

#endif
