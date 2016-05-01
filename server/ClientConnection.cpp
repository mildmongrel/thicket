#include "ClientConnection.h"
#include <QDataStream>
#include <QTimer>

static const int ABORT_CONNECTION_SECS = 30;

ClientConnection::ClientConnection( const Logging::Config &loggingConfig, QObject* parent )
    : QTcpSocket( parent ),
      mIncomingMsgHeader( 0 ),
      mBytesSent( 0 ),
      mBytesReceived( 0 ),
      mLogger( loggingConfig.createLogger() )
{
    QObject::connect(this, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));

    // Init and start abort connection timer.
    mAbortTimer = new QTimer( this );
    connect( mAbortTimer, &QTimer::timeout, this, &ClientConnection::handleAbortTimerTimeout );
    restartAbortTimer();
}


bool
ClientConnection::sendMsg( const thicket::ServerToClientMsg* const protoMsg )
{
    const int protoSize = protoMsg->ByteSize();

    QByteArray msgByteArray;
    msgByteArray.resize( protoSize );
    protoMsg->SerializeToArray( msgByteArray.data(), protoSize );

    // 16-bit header: 1 bit compression flag, 15 bits size.
    quint16 header = 0x0000;
    QByteArray* payloadMsgByteArrayPtr;

    const int COMPRESSION_MAX = 9;
    QByteArray compressedMsgByteArray = qCompress( msgByteArray, COMPRESSION_MAX );
    mLogger->debug( "serialized {} bytes, compressed to {} bytes", protoSize, compressedMsgByteArray.size() );
    if( compressedMsgByteArray.size() < protoSize )
    {
        // The compression resulted in a smaller payload.
        header |= 0x8000;
        payloadMsgByteArrayPtr = &compressedMsgByteArray;
    }
    else
    {
        mLogger->debug( "inefficient compression, sending uncompressed" );
        payloadMsgByteArrayPtr = &msgByteArray;
    }

    const int payloadSize = payloadMsgByteArrayPtr->size();
    if( payloadSize > 0x7FFF )
    {
        mLogger->error( "payload too large ({} bytes) to send!", payloadSize );
        return false;
    }
    header |= payloadSize;

    QByteArray block;
    block.resize( sizeof( header ) + payloadSize );
    QDataStream out( &block, QIODevice::WriteOnly );
    out.setVersion( QDataStream::Qt_4_0 );
    out << (quint16) header;
    out.writeRawData( payloadMsgByteArrayPtr->data(), payloadSize );

    bool writeOk = write( block );
    if( writeOk ) mBytesSent += block.size();
    return writeOk;
}


void
ClientConnection::handleReadyRead()
{
    QDataStream in( this );
    in.setVersion( QDataStream::Qt_4_0 );

    mLogger->trace( "handleReadyRead(): client {}: bytesAvail={}", (std::size_t)this, bytesAvailable() );

    while( bytesAvailable() )
    {
        if( mIncomingMsgHeader == 0 ) {
            if( bytesAvailable() < (int)sizeof( mIncomingMsgHeader ) )
                return;

            in >> mIncomingMsgHeader;
            mBytesReceived += sizeof( mIncomingMsgHeader );
        }

        const bool msgCompressed = mIncomingMsgHeader & 0x8000;
        const quint16 msgSize = mIncomingMsgHeader & 0x7FFF;

        if( bytesAvailable() < msgSize )
            return;

        QByteArray msgByteArray;
        msgByteArray.resize( msgSize );
        in.readRawData( msgByteArray.data(), msgSize );
        mBytesReceived += msgSize;

        if( msgCompressed )
        {
            msgByteArray = qUncompress( msgByteArray );
            mLogger->debug( "read {} bytes, uncompressed to {} bytes",
                    msgSize, msgByteArray.size() );
        }

        thicket::ClientToServerMsg msg;
        bool msgParsed = msg.ParseFromArray( msgByteArray.data(), msgByteArray.size() );
        mIncomingMsgHeader = 0;

        if( !msgParsed )
        {
            mLogger->warn( "Failed to parse msg!" );
            continue;
        }

        if( msg.has_keep_alive_ind() )
        {
            // For a keep alive, no need to process any further - the
            // abort timer will be restarted when receiving any message.
            mLogger->debug( "keep_alive_ind from connection {}", (std::size_t)this );
            continue;
        }

        mLogger->trace( "emitting msgReceived signal" );
        emit msgReceived( &msg );
    }

    // The client is alive - reset monitoring timer.
    restartAbortTimer();
}


void
ClientConnection::handleAbortTimerTimeout()
{
    mLogger->debug( "abort timer expired for connection {}", (std::size_t)this );
    mAbortTimer->stop();
    abort();
}


void
ClientConnection::restartAbortTimer()
{
    mAbortTimer->start( ABORT_CONNECTION_SECS * 1000 );
}

