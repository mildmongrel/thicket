#ifndef CONNECTIONSERVER_H
#define CONNECTIONSERVER_H

#include <QTcpServer>
#include "Logging.h"

class ClientConnection;

class ConnectionServer : public QTcpServer
{
    Q_OBJECT

public:
    ConnectionServer( QObject *parent = 0 );

    // A little wonky - don't really want this ConnectionServer to be a logging parent
    // of the ClientConnections it creates, so this is a way around that.
    void setClientConnectionLoggingConfig( const Logging::Config& loggingConfig )
    {
        mClientConnectionLoggingConfig = loggingConfig;
    }

signals:
    void newClientConnection( ClientConnection *clientConnection );

protected:
    void incomingConnection( qintptr socketDescriptor ) Q_DECL_OVERRIDE;

private:
    Logging::Config mClientConnectionLoggingConfig;
};

#endif
