#ifndef SERVER_H
#define SERVER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QNetworkSession;
class QTimer;
QT_END_NAMESPACE

#include <QAbstractSocket>
#include <QList>
#include <QMap>
#include <memory>

#include "messages.pb.h"
#include "AllSetsData.h"
#include "Draft.h"

#include "Logging.h"
#include "DraftTypes.h"

class ConnectionServer;
class ClientConnection;
class ServerRoom;
class ServerSettings;
class ClientNotices;

class Server : public QObject
{
    Q_OBJECT

public:
    Server( unsigned int                              port,
            const std::shared_ptr<ServerSettings>&    settings,
            const std::shared_ptr<const AllSetsData>& allSetsData,
            const std::shared_ptr<ClientNotices>&     clientNotices,
            const Logging::Config&                    loggingConfig = Logging::Config(),
            QObject*                                  parent = 0 );

    virtual ~Server();

public slots:

    void start();

signals:

    void finished();

private slots:

    void sessionOpened();
    void handleNewClientConnection( ClientConnection* clientConnection );
    void handleMessageFromClient( const thicket::ClientToServerMsg* const msg );
    void handleClientError(QAbstractSocket::SocketError);
    void handleClientDestroyed(QObject*);
    void handleClientDisconnected();

    void handleAnnouncementsUpdate( const QString& text );
    void handleAlertUpdate( const QString& text );

    void handleRoomPlayerCountChanged( int playerCount );
    void handleRoomExpired();

    void handleRoomsInfoDiffBroadcastTimerTimeout();

private:  // Methods

    void sendGreetingInd( ClientConnection* clientConnection );
    void sendAnnouncementsInd( ClientConnection* clientConnection, const std::string& text );
    void sendAlertsInd( ClientConnection* clientConnection, const std::string& text );
    void sendRoomCapabilitiesInd( ClientConnection* clientConnection );
    void sendLoginRsp( ClientConnection* clientConnection,
                       const thicket::LoginRsp::ResultType& result );
    void sendCreateRoomFailureRsp( ClientConnection* clientConnection,
                                   thicket::CreateRoomFailureRsp_ResultType result );
    void sendJoinRoomFailureRsp( ClientConnection* clientConnection,
                                 thicket::JoinRoomFailureRsp_ResultType result, int roomId );

    // Send a baseline rooms information message to a client.
    void sendBaselineRoomsInfo( ClientConnection* clientConnection );

    // Broadcast rooms information differences to all clients.
    void broadcastRoomsInfoDiffs();

    // Arms the timer to send out a rooms info diff broadcast, unless
    // it was already armed.
    void armRoomsInfoDiffBroadcastTimer();

    // Send a baseline users information message to a client.
    void sendBaselineUsersInfo( ClientConnection* clientConnection );

    // Broadcast users information differences to all clients.
    void broadcastUsersInfoDiffs();

private:  // Data

    const quint16                      mPort;
    std::shared_ptr<ServerSettings>    mSettings;
    std::shared_ptr<const AllSetsData> mAllSetsData;
    std::shared_ptr<ClientNotices>     mClientNotices;

    QNetworkSession*                    mNetworkSession;
    ConnectionServer*                   mConnectionServer;
    QMap<ClientConnection*,std::string> mClientConnectionLoginMap;
    unsigned int                        mNextRoomId;
    QMap<unsigned int,ServerRoom*>      mRoomMap;

    QList<int>       mRoomsInfoDiffAddedRoomIds;
    QList<int>       mRoomsInfoDiffRemovedRoomIds;
    QMap<int,int>    mRoomsInfoDiffPlayerCountsMap;
    QTimer*          mRoomsInfoDiffBroadcastTimer;

    QList<std::string> mUsersInfoDiffAddedNames;
    QList<std::string> mUsersInfoDiffRemovedNames;

    uint64_t mTotalDisconnectedClientBytesSent;
    uint64_t mTotalDisconnectedClientBytesReceived;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
