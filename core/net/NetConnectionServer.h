#ifndef NETCONNECTIONSERVER_H
#define NETCONNECTIONSERVER_H

#include <QTcpServer>

class NetConnectionServer : public QTcpServer
{
    Q_OBJECT

public:
    NetConnectionServer( QObject* parent = 0 );

signals:
    void incomingConnectionSocket( qintptr socketDescriptor );

protected:
    void incomingConnection( qintptr socketDescriptor ) Q_DECL_OVERRIDE;
};

#endif
