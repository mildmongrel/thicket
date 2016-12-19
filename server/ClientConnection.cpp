#include "ClientConnection.h"


ClientConnection::ClientConnection( const Logging::Config& loggingConfig, QObject* parent )
  : NetConnection( loggingConfig, parent )
{
    QObject::connect( this, &NetConnection::msgReceived, this, &ClientConnection::handleMsgReceived );
    setRxInactivityAbortTime( 30000 );
}


bool
ClientConnection::sendProtoMsg( const proto::ServerToClientMsg* const protoMsg )
{
    const int protoSize = protoMsg->ByteSize();

    QByteArray msgByteArray;
    msgByteArray.resize( protoSize );
    protoMsg->SerializeToArray( msgByteArray.data(), protoSize );

    return sendMsg( msgByteArray );
}


void
ClientConnection::handleMsgReceived( const QByteArray& msg )
{
    proto::ClientToServerMsg protoMsg;
    bool msgParsed = protoMsg.ParseFromArray( msg.data(), msg.size() );

    if( !msgParsed )
    {
        mLogger->warn( "Failed to parse msg!" );
        return;
    }

    if( protoMsg.has_keep_alive_ind() )
    {
        // For a keep alive, no need to process any further - the
        // abort timer will be restarted when receiving any message.
        mLogger->debug( "keep_alive_ind from connection {}", (std::size_t)this );
        return;
    }

    mLogger->trace( "emitting msgReceived signal" );
    emit protoMsgReceived( &protoMsg );
}

