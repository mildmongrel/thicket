#include <QtNetwork>

#include "ConnectionServer.h"
#include "ClientConnection.h"


ConnectionServer::ConnectionServer(QObject *parent)
    : QTcpServer(parent)
{
}


void
ConnectionServer::incomingConnection( qintptr socketDescriptor )
{
    ClientConnection *clientConnection = new ClientConnection( mClientConnectionLoggingConfig, this );
    clientConnection->setSocketDescriptor( socketDescriptor );
    emit newClientConnection( clientConnection );
}
