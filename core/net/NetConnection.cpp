#include "NetConnection.h"
#include <QDataStream>
#include <QTimer>
#include "qtutils_core.h"


NetConnection::NetConnection( const Logging::Config &loggingConfig, QObject* parent )
    : QTcpSocket( parent ),
      mLogger( loggingConfig.createLogger() ),
      mRxInactivityAbortTimeMillis( 0 ),
      mCompressionMode( COMPRESSION_MODE_AUTO ),
      mHeaderMode( HEADER_MODE_AUTO ),
      mIncomingMsgHeader( 0 ),
      mExtendedLength( 0 ),
      mBytesSent( 0 ),
      mBytesReceived( 0 )
{
    QObject::connect(this, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));

    // Init and start abort connection timer.
    mRxInactivityAbortTimer = new QTimer( this );
    connect( mRxInactivityAbortTimer, &QTimer::timeout,
             this,                    &NetConnection::handleRxInactivityAbortTimerTimeout );
    mRxInactivityAbortTimer->setSingleShot( true );
    restartRxInactivityAbortTimer();
}


void
NetConnection::setRxInactivityAbortTime( int inactivityMillis )
{
    mRxInactivityAbortTimeMillis = inactivityMillis;
    restartRxInactivityAbortTimer();
}


bool
NetConnection::sendMsg( const QByteArray& byteArray )
{
    mLogger->trace( "sendmsg: [{}] {}", byteArray.size(), hexStringify( byteArray, 10 ) );

    // 16-bit header: 1 bit compression flag, 1 bit extended flag, 14 bits size.
    quint16 header = 0x0000;
    const QByteArray* payloadMsgByteArrayPtr;

    QByteArray compressedMsgByteArray;
    if( mCompressionMode == COMPRESSION_MODE_UNCOMPRESSED )
    {
        payloadMsgByteArrayPtr = &byteArray;
    }
    else 
    {
        const int COMPRESSION_MAX = 9;
        compressedMsgByteArray = qCompress( byteArray, COMPRESSION_MAX );
        mLogger->debug( "compressed {} bytes to {} bytes",
                byteArray.size(), compressedMsgByteArray.size() );

        if( mCompressionMode == COMPRESSION_MODE_AUTO )
        {
            if( compressedMsgByteArray.size() < byteArray.size() )
            {
                // The compression resulted in a smaller payload.
                header |= 0x8000;
                payloadMsgByteArrayPtr = &compressedMsgByteArray;
            }
            else
            {
                mLogger->debug( "inefficient compression, sending uncompressed" );
                payloadMsgByteArrayPtr = &byteArray;
            }
        }
        else
        {
            header |= 0x8000;
            payloadMsgByteArrayPtr = &compressedMsgByteArray;
        }
    }

    const int payloadSize = payloadMsgByteArrayPtr->size();
    if( (mHeaderMode == HEADER_MODE_BRIEF) && (payloadSize > 0x3FFF) )
    {
        mLogger->error( "payload too large ({} bytes) to send!", payloadSize );
        return false;
    }

    bool extended = (mHeaderMode == HEADER_MODE_EXTENDED) ||
                    ((mHeaderMode == HEADER_MODE_AUTO) && (payloadSize > 0x3FFF));
    if( extended )
    {
        // Set extended flag.
        header |= 0x4000;
    }
    else
    {
        // Payload size in lower 14 bits.
        header |= payloadSize;
    }

    QByteArray block;
    block.resize( sizeof( header ) + payloadSize );
    QDataStream out( &block, QIODevice::WriteOnly );
    out.setVersion( QDataStream::Qt_4_0 );
    out << (quint16) header;
    if( extended ) out << (quint32) payloadSize;
    out.writeRawData( payloadMsgByteArrayPtr->data(), payloadSize );
    mLogger->trace( "sendmsg: wrote [{}] {}", payloadMsgByteArrayPtr->size(),
            hexStringify( *payloadMsgByteArrayPtr, 10 ) );

    bool writeOk = write( block );

    if( writeOk ) mBytesSent += block.size();

    return writeOk;
}


void
NetConnection::handleReadyRead()
{
    QDataStream in( this );
    in.setVersion( QDataStream::Qt_4_0 );

    mLogger->trace( "handleReadyRead(): sock {}: bytesAvail={}",
            (std::size_t)this, bytesAvailable() );

    while( bytesAvailable() )
    {
        if( mIncomingMsgHeader == 0 ) {
            if( bytesAvailable() < (int)sizeof( mIncomingMsgHeader ) )
                return;

            in >> mIncomingMsgHeader;
            mBytesReceived += sizeof( mIncomingMsgHeader );
        }

        const bool msgExtendedHeader = mIncomingMsgHeader & 0x4000;

        if( msgExtendedHeader && (mExtendedLength == 0) ) {
            if( bytesAvailable() < (int)sizeof( mExtendedLength ) )
                return;

            in >> mExtendedLength;
            mBytesReceived += sizeof( mExtendedLength );
        }

        const bool msgCompressed = mIncomingMsgHeader & 0x8000;

        const quint32 msgSize = msgExtendedHeader ? mExtendedLength
                                                  : mIncomingMsgHeader & 0x3FFF;

        if( bytesAvailable() < msgSize )
            return;

        QByteArray msgByteArray;
        msgByteArray.resize( msgSize );
        mLogger->debug( "reading {} bytes into message buffer ({} available)",
                msgSize, bytesAvailable() );
        in.readRawData( msgByteArray.data(), msgSize );
        mBytesReceived += msgSize;

        mIncomingMsgHeader = 0;
        mExtendedLength = 0;

        if( msgCompressed )
        {
            msgByteArray = qUncompress( msgByteArray );
            mLogger->debug( "deserialized {} bytes, uncompressed to {} bytes",
                    msgSize, msgByteArray.size() );
        }
        else
        {
            mLogger->debug( "deserialized {} bytes, not compressed", msgSize );
        }

        mLogger->trace( "emit: [{}] {}", msgByteArray.size(),
                hexStringify( msgByteArray, 10 ) );
        emit msgReceived( msgByteArray );
    }

    // The other side is alive - reset monitoring timer.
    restartRxInactivityAbortTimer();
}


void
NetConnection::handleRxInactivityAbortTimerTimeout()
{
    mLogger->debug( "rx inactivity abort timer expired for sock {}", (std::size_t)this );
    mRxInactivityAbortTimer->stop();
    abort();
}


void
NetConnection::restartRxInactivityAbortTimer()
{
    if( mRxInactivityAbortTimeMillis > 0 )
    {
        mRxInactivityAbortTimer->start( mRxInactivityAbortTimeMillis );
    }
    else
    {
        mRxInactivityAbortTimer->stop();
    }
}

