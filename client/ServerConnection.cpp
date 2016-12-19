#include "ServerConnection.h"


ServerConnection::ServerConnection( const Logging::Config& loggingConfig, QObject* parent )
  : NetConnection( loggingConfig, parent )
{
    QObject::connect( this, &NetConnection::msgReceived, this, &ServerConnection::handleMsgReceived );
}


bool
ServerConnection::sendProtoMsg( const proto::ClientToServerMsg& protoMsg )
{
    const int protoSize = protoMsg.ByteSize();

    QByteArray msgByteArray;
    msgByteArray.resize( protoSize );
    protoMsg.SerializeToArray( msgByteArray.data(), protoSize );

    return sendMsg( msgByteArray );
}


void
ServerConnection::handleMsgReceived( const QByteArray& msg )
{
    proto::ServerToClientMsg protoMsg;
    bool msgParsed = protoMsg.ParseFromArray( msg.data(), msg.size() );

    if( !msgParsed )
    {
        mLogger->warn( "Failed to parse msg!" );
        return;
    }

    mLogger->trace( "emitting msgReceived signal" );
    emit protoMsgReceived( protoMsg );
}

