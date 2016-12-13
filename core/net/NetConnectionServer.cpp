#include <QtNetwork>

#include "NetConnectionServer.h"
#include "NetConnection.h"


NetConnectionServer::NetConnectionServer( QObject* parent )
    : QTcpServer(parent)
{
}


void
NetConnectionServer::incomingConnection( qintptr socketDescriptor )
{
    NetConnection* netConnection = new NetConnection( mNetConnectionLoggingConfig, this );
    netConnection->setSocketDescriptor( socketDescriptor );
    emit newNetConnection( netConnection );
}
