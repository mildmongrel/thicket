#include <QtNetwork>

#include "NetConnectionServer.h"


NetConnectionServer::NetConnectionServer( QObject* parent )
    : QTcpServer(parent)
{
}


void
NetConnectionServer::incomingConnection( qintptr socketDescriptor )
{
    emit incomingConnectionSocket( socketDescriptor );
}
