#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H

#include <NetConnection.h>
#include "messages.pb.h"
#include "Logging.h"

class ServerConnection : public NetConnection
{
    Q_OBJECT

public:

    ServerConnection( const Logging::Config& loggingConfig = Logging::Config(), QObject* parent = 0 );

    bool sendProtoMsg( const proto::ClientToServerMsg& protoMsg );

signals:
    void protoMsgReceived( const proto::ServerToClientMsg& protoMsg );

private slots:
    void handleMsgReceived( const QByteArray& msg );
};

#endif
