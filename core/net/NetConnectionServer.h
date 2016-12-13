#ifndef NETCONNECTIONSERVER_H
#define NETCONNECTIONSERVER_H

#include <QTcpServer>
#include "Logging.h"

class NetConnection;

class NetConnectionServer : public QTcpServer
{
    Q_OBJECT

public:
    NetConnectionServer( QObject* parent = 0 );

    // A little wonky - don't really want this NetConnectionServer to be a logging parent
    // of the ClientConnections it creates, so this is a way around that.
    void setNetConnectionLoggingConfig( const Logging::Config& loggingConfig )
    {
        mNetConnectionLoggingConfig = loggingConfig;
    }

signals:
    void newNetConnection( NetConnection *netConnection );

protected:
    void incomingConnection( qintptr socketDescriptor ) Q_DECL_OVERRIDE;

private:
    Logging::Config mNetConnectionLoggingConfig;
};

#endif
