#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <NetConnection.h>
#include "messages.pb.h"
#include "Logging.h"

class ClientConnection : public NetConnection
{
    Q_OBJECT

public:

    ClientConnection( const Logging::Config& loggingConfig = Logging::Config(), QObject* parent = 0 );

    bool sendProtoMsg( const proto::ServerToClientMsg& protoMsg );

signals:
    void protoMsgReceived( const proto::ClientToServerMsg& protoMsg );

private slots:
    void handleMsgReceived( const QByteArray& msg );
};

#endif
