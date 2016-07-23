#include "Server.h"

#include <QtNetwork>

#include <stdlib.h>
#include <memory>
#include <sstream>
#include <iomanip>

#include "version.h"

#include "qtutils_core.h"

#include "ServerSettings.h"
#include "ClientNotices.h"
#include "ConnectionServer.h"
#include "ClientConnection.h"
#include "ServerRoom.h"
#include "RoomConfigValidator.h"
#include "CardDispenserFactory.h"

Server::Server( unsigned int                              port,
                const std::shared_ptr<ServerSettings>&    settings,
                const std::shared_ptr<const AllSetsData>& allSetsData,
                const std::shared_ptr<ClientNotices>&     clientNotices,
                const Logging::Config&                    loggingConfig,
                QObject*                                  parent )
:   QObject( parent ),
    mPort( port ),
    mSettings( settings ),
    mAllSetsData( allSetsData ),
    mClientNotices( clientNotices ),
    mNetworkSession( 0 ),
    mConnectionServer( 0 ),
    mRoomConfigValidator( allSetsData, loggingConfig.createChildConfig( "roomconfigvalidator" ) ),
    mNextRoomId( 0 ),
    mTotalDisconnectedClientBytesSent( 0 ),
    mTotalDisconnectedClientBytesReceived( 0 ),
    mLoggingConfig( loggingConfig ),
    mLogger( mLoggingConfig.createLogger() )
{
    connect( mClientNotices.get(), &ClientNotices::announcementsUpdate, this, &Server::handleAnnouncementsUpdate );
    connect( mClientNotices.get(), &ClientNotices::alertUpdate, this, &Server::handleAlertUpdate );

    mRoomsInfoDiffBroadcastTimer = new QTimer( this );
    mRoomsInfoDiffBroadcastTimer->setSingleShot( true );
    connect( mRoomsInfoDiffBroadcastTimer, &QTimer::timeout, this, &Server::handleRoomsInfoDiffBroadcastTimerTimeout );
}


Server::~Server()
{
    mLogger->trace( "~Server" );

    // Delete rooms.
    for( auto iter = mRoomMap.begin(); iter != mRoomMap.end(); ++iter )
    {
        ServerRoom* room = iter.value();
        room->deleteLater();
    }
    mRoomMap.clear();
}


void
Server::start()
{
    // Check if a network session is required.
    QNetworkConfigurationManager manager;
    if( manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired )
    {
        // If network session required, use the system default.
        QNetworkConfiguration config = manager.defaultConfiguration();

        mNetworkSession = new QNetworkSession(config, this);

        // Forward the session-opened signal to our own event signal.
        connect( mNetworkSession, SIGNAL(opened()), this, SIGNAL(sessionOpened()) );
        mNetworkSession->open();
    }
    else
    {
        sessionOpened();
    }
}


void
Server::sessionOpened()
{
    // Save the used configuration
    if( mNetworkSession ) {
        QNetworkConfiguration config = mNetworkSession->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = mNetworkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();

        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }

    // Set up main connection server.

    mConnectionServer = new ConnectionServer(this);
    mConnectionServer->setClientConnectionLoggingConfig( mLoggingConfig.createChildConfig( "clientconnection" ) );
    if( !mConnectionServer->listen( QHostAddress::Any, mPort ) ) {
        mLogger->critical( "Unable to start the server: {}", mConnectionServer->errorString() );
        emit finished();
        return;
    }

    connect( mConnectionServer, SIGNAL(newClientConnection(ClientConnection*)),
             this, SLOT(handleNewClientConnection(ClientConnection*)) );

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    mLogger->notice( "The server is running on\n\nIP: {}\nport: {}\n\n",
            ipAddress, mConnectionServer->serverPort() );
}


void
Server::handleNewClientConnection( ClientConnection* clientConnection )
{
    connect( clientConnection, SIGNAL(msgReceived(const proto::ClientToServerMsg* const)),
             this, SLOT(handleMessageFromClient(const proto::ClientToServerMsg* const)) );
    connect( clientConnection, SIGNAL(disconnected()),
             this, SLOT(handleClientDisconnected()) );
    connect( clientConnection, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(handleClientError(QAbstractSocket::SocketError)) );
    connect( clientConnection, SIGNAL(destroyed(QObject*)),
             this, SLOT(handleClientDestroyed(QObject*)) );

    const quint16 localPort = clientConnection->localPort();
    mLogger->info( "client connection: localPort={}, peerAddr:Port={}:{}",
            localPort, clientConnection->peerAddress().toString(), clientConnection->peerPort() );

    // Send a greeting to the client, who should make a request after receiving it.
    sendGreetingInd( clientConnection );
}


void
Server::sendGreetingInd( ClientConnection* clientConnection )
{
    mLogger->trace( "sendGreetingInd" );
    proto::ServerToClientMsg msg;
    proto::GreetingInd* greetingInd = msg.mutable_greeting_ind();
    greetingInd->set_protocol_version_major( proto::PROTOCOL_VERSION_MAJOR );
    greetingInd->set_protocol_version_minor( proto::PROTOCOL_VERSION_MINOR );
    greetingInd->set_server_name( mSettings->getServerName().toStdString() );
    greetingInd->set_server_version( gServerVersion );
    clientConnection->sendMsg( &msg );
}


void
Server::sendAnnouncementsInd( ClientConnection* clientConnection, const std::string& text )
{
    mLogger->trace( "sendAnnouncementsInd" );
    proto::ServerToClientMsg msg;
    proto::AnnouncementsInd* announcementsInd = msg.mutable_announcements_ind();
    announcementsInd->set_text( text );
    clientConnection->sendMsg( &msg );
}


void
Server::sendAlertsInd( ClientConnection* clientConnection, const std::string& text )
{
    mLogger->trace( "sendAlertsInd" );
    proto::ServerToClientMsg msg;
    proto::AlertsInd* alertsInd = msg.mutable_alerts_ind();
    alertsInd->set_text( text );
    clientConnection->sendMsg( &msg );
}


void
Server::sendRoomCapabilitiesInd( ClientConnection* clientConnection )
{
    mLogger->trace( "sendRoomCapabilitiesInd" );
    proto::ServerToClientMsg msg;
    proto::RoomCapabilitiesInd* capsInd = msg.mutable_room_capabilities_ind();
    const std::vector<std::string> allSetCodes = mAllSetsData->getSetCodes();
    for( const std::string& code : allSetCodes )
    {
        proto::RoomCapabilitiesInd::SetCapability* addedSet = capsInd->add_sets();
        addedSet->set_code( code );
        addedSet->set_name( mAllSetsData->getSetName( code ) );
        addedSet->set_booster_generation( mAllSetsData->hasBoosterSlots( code ) );
    }
    clientConnection->sendMsg( &msg );
}


void
Server::sendLoginRsp( ClientConnection* clientConnection, const proto::LoginRsp::ResultType& result )
{
    mLogger->trace( "sendLoginRsp" );
    proto::ServerToClientMsg msg;
    proto::LoginRsp* rsp = msg.mutable_login_rsp();
    rsp->set_result( result );
    clientConnection->sendMsg( &msg );
}


void
Server::sendLoginRspIncompatibleProtoVer( ClientConnection* clientConnection )
{
    mLogger->trace( "sendLoginRspIncompatibleProtoVer" );
    proto::ServerToClientMsg msg;
    proto::LoginRsp* rsp = msg.mutable_login_rsp();
    rsp->set_result( proto::LoginRsp::RESULT_FAILURE_INCOMPATIBLE_PROTO_VER );
    proto::LoginRsp::ClientDownloadInfo* dlInfo = rsp->mutable_client_download_info();
    dlInfo->set_description(
            "Please visit the project release area to download a compatible client." );
    dlInfo->set_url( "http://github.com/mildmongrel/thicket/releases" );
    clientConnection->sendMsg( &msg );
}


void
Server::sendCreateRoomFailureRsp( ClientConnection* clientConnection, proto::CreateRoomFailureRsp_ResultType result )
{
    mLogger->trace( "sendCreateRoomFailureRsp, result={}", result );
    proto::ServerToClientMsg msg;
    proto::CreateRoomFailureRsp* createRoomFailureRsp = msg.mutable_create_room_failure_rsp();
    createRoomFailureRsp->set_result( result );
    clientConnection->sendMsg( &msg );
}


void
Server::sendJoinRoomFailureRsp( ClientConnection* clientConnection, proto::JoinRoomFailureRsp_ResultType result, int roomId )
{
    mLogger->trace( "sendJoinRoomFailureRsp, result={}, roomId={}", result, roomId );
    proto::ServerToClientMsg msg;
    proto::JoinRoomFailureRsp* joinRoomFailureRsp = msg.mutable_join_room_failure_rsp();
    joinRoomFailureRsp->set_result( result );
    joinRoomFailureRsp->set_room_id( roomId );
    clientConnection->sendMsg( &msg );
}


void
Server::sendBaselineRoomsInfo( ClientConnection* clientConnection )
{
    mLogger->trace( "sendBaselineRoomsInfo" );

    // Assemble the message.
    proto::ServerToClientMsg msg;
    proto::RoomsInfoInd* roomsInfoInd = msg.mutable_rooms_info_ind();

    auto iter = mRoomMap.constBegin();
    while( iter != mRoomMap.constEnd() )
    {
        const ServerRoom * const room = iter.value();

        proto::RoomsInfoInd::RoomInfo* addedRoom = roomsInfoInd->add_added_rooms();
        addedRoom->set_room_id( iter.key() );

        // Assemble room configuration.
        proto::RoomConfig* roomConfig = addedRoom->mutable_room_config();
        *roomConfig = room->getRoomConfig();

        if( room->getPlayerCount() > 0 )
        {
            proto::RoomsInfoInd::PlayerCount* playerCount = roomsInfoInd->add_player_counts();
            playerCount->set_room_id( iter.key() );
            playerCount->set_player_count( room->getPlayerCount() );
        }

        ++iter;
    }

    mLogger->debug( "sending RoomsInfoInd, size={} to client {}",
            msg.ByteSize(), (std::size_t)clientConnection );
    clientConnection->sendMsg( &msg );
}


void
Server::broadcastRoomsInfoDiffs()
{
    mLogger->trace( "broadcastRoomsInfoDiffs" );

    //
    // Go through the added, removed, and player updates and
    // assemble a diff message
    //

    // First, make sure there is something to send.
    if( mRoomsInfoDiffAddedRoomIds.empty() &&
        mRoomsInfoDiffRemovedRoomIds.empty() &&
        mRoomsInfoDiffPlayerCountsMap.empty() )
    {
        mLogger->debug( "broadcastRoomsInfoDiffs: nothing to send" );
        return;
    }

    proto::ServerToClientMsg msg;
    proto::RoomsInfoInd* roomsInfoInd = msg.mutable_rooms_info_ind();

    for( int roomId : mRoomsInfoDiffAddedRoomIds )
    {
        proto::RoomsInfoInd::RoomInfo* addedRoom = roomsInfoInd->add_added_rooms();
        const ServerRoom* const room = mRoomMap[roomId];

        addedRoom->set_room_id( roomId );

        // Assemble room configuration.
        proto::RoomConfig* roomConfig = addedRoom->mutable_room_config();
        *roomConfig = room->getRoomConfig();
    }

    for( int roomId : mRoomsInfoDiffRemovedRoomIds )
    {
        roomsInfoInd->add_removed_rooms( roomId );
    }

    auto iter = mRoomsInfoDiffPlayerCountsMap.constBegin();
    while( iter != mRoomsInfoDiffPlayerCountsMap.constEnd() )
    {
        proto::RoomsInfoInd::PlayerCount* playerCount = roomsInfoInd->add_player_counts();
        playerCount->set_room_id( iter.key() );
        playerCount->set_player_count( iter.value() );
        ++iter;
    }

    // Clear all differences.
    mRoomsInfoDiffAddedRoomIds.clear();
    mRoomsInfoDiffRemovedRoomIds.clear();
    mRoomsInfoDiffPlayerCountsMap.clear();

    // Send the message to each client connection.
    for( auto clientConn : mClientConnectionLoginMap.keys() )
    {
        mLogger->debug( "sending RoomsInfoInd, size={} to client {}",
                msg.ByteSize(), (std::size_t)clientConn );
        clientConn->sendMsg( &msg );
    }
}


void
Server::armRoomsInfoDiffBroadcastTimer()
{
    if( !mRoomsInfoDiffBroadcastTimer->isActive() )
    {
        mLogger->debug( "starting room info diffs broadcast timer" );
        mRoomsInfoDiffBroadcastTimer->start( 1000 );
    }
}


void
Server::sendBaselineUsersInfo( ClientConnection* clientConnection )
{
    mLogger->trace( "sendBaselineUsersInfo" );

    // Assemble the message.
    proto::ServerToClientMsg msg;
    proto::UsersInfoInd* usersInfoInd = msg.mutable_users_info_ind();

    auto iter = mClientConnectionLoginMap.constBegin();
    while( iter != mClientConnectionLoginMap.constEnd() )
    {
        proto::UsersInfoInd::UserInfo* addedUser = usersInfoInd->add_added_users();
        addedUser->set_name( iter.value() );

        ++iter;
    }

    mLogger->debug( "sending UsersInfoInd, size={} to client {}",
            msg.ByteSize(), (std::size_t)clientConnection );
    clientConnection->sendMsg( &msg );
}


void
Server::broadcastUsersInfoDiffs()
{
    mLogger->trace( "broadcastUsersInfoDiffs" );

    //
    // Go through the added, removed, and player updates and
    // assemble a diff message
    //

    // First, make sure there is something to send.
    if( mUsersInfoDiffAddedNames.empty() &&
        mUsersInfoDiffRemovedNames.empty() )
    {
        mLogger->debug( "broadcastUsersInfoDiffs: nothing to send" );
        return;
    }

    proto::ServerToClientMsg msg;
    proto::UsersInfoInd* usersInfoInd = msg.mutable_users_info_ind();

    for( const std::string& name : mUsersInfoDiffAddedNames )
    {
        proto::UsersInfoInd::UserInfo* addedUser = usersInfoInd->add_added_users();
        addedUser->set_name( name );
    }

    for( const std::string& name : mUsersInfoDiffRemovedNames )
    {
        usersInfoInd->add_removed_users( name );
    }

    // Clear all differences.
    mUsersInfoDiffAddedNames.clear();
    mUsersInfoDiffRemovedNames.clear();

    // Send the message to each client connection.
    for( auto clientConn : mClientConnectionLoginMap.keys() )
    {
        mLogger->debug( "sending UsersInfoInd, size={} to client {}",
                msg.ByteSize(), (std::size_t)clientConn );
        clientConn->sendMsg( &msg );
    }
}


void
Server::handleMessageFromClient( const proto::ClientToServerMsg* const msg )
{
    // This is where non-draft messaging can be handled for connections:
    //   - login/auth messages before HumanPlayer is set up
    //   - login/auth to re-establish HumanPlayer setup
    //   - chat
    //   - graceful exit

    ClientConnection *clientConnection = qobject_cast<ClientConnection *>(QObject::sender());

    const bool loggedIn = mClientConnectionLoginMap.contains( clientConnection );

    if( msg->has_login_req() )
    {
        const proto::LoginReq& req = msg->login_req();
        const std::string name = req.name();

        // Currently nothing special to authenticate, just need a unique
        // connection and a unique name.
        if( loggedIn )
        {
            sendLoginRsp( clientConnection, proto::LoginRsp::RESULT_FAILURE_ALREADY_LOGGED_IN );
        }
        else if( proto::PROTOCOL_VERSION_MAJOR > req.protocol_version_major() )
        {
            // The client is too old, we don't support it.
            sendLoginRspIncompatibleProtoVer( clientConnection );
        }
        else if( name.empty() || mClientConnectionLoginMap.values().contains( name ) )
        {
            sendLoginRsp( clientConnection, proto::LoginRsp::RESULT_FAILURE_NAME_IN_USE );
        }
        else
        {
            // The user will be logged in.  Just before updating the login
            // list, broadcast any current rooms information differences to
            // all other logged in users.  At this point all other users will
            // be in sync with the new user once he gets his baseline rooms
            // information.
            broadcastRoomsInfoDiffs();

            // Similarly to rooms, add the user to the users list and
            // broadcast it out so that all users are in sync when the new
            // user gets his baseline.
            mUsersInfoDiffAddedNames.push_back( name );
            broadcastUsersInfoDiffs();

            mLogger->info( "client logged in: name={}", name );
            mClientConnectionLoginMap.insert( clientConnection, name );
            sendLoginRsp( clientConnection, proto::LoginRsp::RESULT_SUCCESS );

            // OPTIMIZATION: Always send room capabilities at login time
            // for now, but to save bandwidth this could be sent on request
            // from client when it's time to create a room.
            sendRoomCapabilitiesInd( clientConnection );

            // Send all current room and user information to client.
            sendBaselineRoomsInfo( clientConnection );
            sendBaselineUsersInfo( clientConnection );

            // If there are any announcements, send them now.
            QString announcements = mClientNotices->getAnnouncements();
            if( !announcements.isEmpty() )
            {
                sendAnnouncementsInd( clientConnection, announcements.toStdString() );
            }

            // If there is an alert, send it now.
            QString alert = mClientNotices->getAlert();
            if( !alert.isEmpty() )
            {
                sendAlertsInd( clientConnection, alert.toStdString() );
            }

            // User has logged in.  Check to see if they should be rejoined to a room
            // they had disconnected from.
            auto end = mRoomMap.cend();
            for( auto iter = mRoomMap.cbegin(); iter != end; ++iter )
            {
                ServerRoom* room = iter.value();

                // If the user has logged in with a unique name, the only
                // way a room could contain that name is if the user had
                // been previously disconnected.
                if( room->containsHumanPlayer( name ) )
                {
                    bool result = room->rejoin( clientConnection, name );
                    if( !result )
                    {
                        // This should never happen but it's not critical,
                        // it just means the user isn't in the room.
                        mLogger->warn( "{} failed to rejoin room!", name );
                    }
                    break;
                }
            }
        }
    }
    else if( msg->has_chat_message_ind() && loggedIn )
    {
        const proto::ChatMessageInd& ind = msg->chat_message_ind();
        const std::string loginName = mClientConnectionLoginMap.value( clientConnection );
        mLogger->debug( "got chat message from {}, scope={}", loginName, ind.scope() );

        // Deliver message to all logged-in users.
        proto::ServerToClientMsg msg;
        proto::ChatMessageDeliveryInd* deliveryInd = msg.mutable_chat_message_delivery_ind();
        deliveryInd->set_sender( loginName );
        deliveryInd->set_scope( ind.scope() );
        deliveryInd->set_text( ind.text() );

        QList<ClientConnection*> destClientConnections;

        if( ind.scope() == proto::CHAT_SCOPE_ALL )
        {
            // Send the message to each client connection.
            destClientConnections = mClientConnectionLoginMap.keys();
        }
        else if( ind.scope() == proto::CHAT_SCOPE_ROOM )
        {
            auto end = mRoomMap.cend();
            for( auto iter = mRoomMap.cbegin(); iter != end; ++iter )
            {
                ServerRoom* room = iter.value();
                if( room->containsHumanPlayer( loginName ) )
                {
                    // Found the room.  Get all other connections and
                    // send them the chat message.
                    destClientConnections = room->getClientConnections();
                }
            }
        }
        else
        {
            mLogger->warn( "chat scope {} not currently supported", ind.scope() );
        }

        // Send the message to each destination client connection.
        for( auto clientConn : destClientConnections )
        {
            mLogger->debug( "sending ChatMessageDeliveryInd, size={} to client {}",
                    msg.ByteSize(), (std::size_t)clientConn );
            clientConn->sendMsg( &msg );
        }
    }
    else if( msg->has_create_room_req() && loggedIn )
    {
        const proto::CreateRoomReq& req = msg->create_room_req();
        const proto::RoomConfig& roomConfig = req.room_config();

        // Make sure the name is unique.
        const std::string& name = roomConfig.name();
        for( auto iter = mRoomMap.cbegin(); iter != mRoomMap.cend(); ++iter )
        {
            ServerRoom* room = iter.value();
            if( name == room->getName() )
            {
                sendCreateRoomFailureRsp( clientConnection, proto::CreateRoomFailureRsp::RESULT_NAME_IN_USE );
                return;
            }
        }

        // Make sure the configuration is valid.
        proto::CreateRoomFailureRsp_ResultType failureResult;
        bool valid = mRoomConfigValidator.validate( roomConfig, failureResult );
        if( !valid )
        {
            mLogger->notice( "invalid configuration from client" );
            sendCreateRoomFailureRsp( clientConnection, failureResult );
            return;
        }

        // Create dispensers.
        CardDispenserFactory factory( mAllSetsData );
        DraftCardDispenserSharedPtrVector<DraftCard> dispensers =
                factory.createCardDispensers( roomConfig.draft_config() );
        if( dispensers.empty() )
        {
            mLogger->warn( "error creating configurations" );
            sendCreateRoomFailureRsp( clientConnection, proto::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_CONFIG );
            return;
        }

        // Create room.
        const int roomId = mNextRoomId++;
        const std::string& password = req.has_password() ? req.password() : std::string();
        const QString loggingConfigName = "serverroom-" + QString::number( roomId );
        ServerRoom* room = new ServerRoom( roomId, password, roomConfig, dispensers,
                mLoggingConfig.createChildConfig( loggingConfigName.toStdString() ), this );
        mRoomMap[roomId] = room;
        connect( room, &ServerRoom::playerCountChanged, this, &Server::handleRoomPlayerCountChanged );
        connect( room, &ServerRoom::roomExpired, this, &Server::handleRoomExpired );
        connect( room, &ServerRoom::roomError, this, &Server::handleRoomError );

        // Add the room to the room information differences list.
        mRoomsInfoDiffAddedRoomIds.push_back( roomId );
        armRoomsInfoDiffBroadcastTimer();

        // Send response to client.
        mLogger->debug( "sendCreateRoomSuccessRsp: roomId={}", roomId );
        proto::ServerToClientMsg msg;
        proto::CreateRoomSuccessRsp* createRoomSuccessRsp = msg.mutable_create_room_success_rsp();
        createRoomSuccessRsp->set_room_id( roomId );
        clientConnection->sendMsg( &msg );
    }
    else if( msg->has_join_room_req() && loggedIn )
    {
        const proto::JoinRoomReq& req = msg->join_room_req();
        const unsigned int roomId = req.room_id();
        const std::string& password = req.has_password() ? req.password() : std::string();
        const std::string loginName = mClientConnectionLoginMap.value( clientConnection );

        ServerRoom* room = mRoomMap.value( roomId, nullptr );
        if( room != nullptr )
        {
            int chairIndex = -1;
            bool result = room->join( clientConnection, loginName, password, chairIndex );
            if( !result )
            {
                // This is normal, e.g. room full or bad password.
                mLogger->info( "{} failed to join room", loginName );
            }
        }
        else
        {
            sendJoinRoomFailureRsp( clientConnection,
                    proto::JoinRoomFailureRsp::RESULT_INVALID_ROOM, roomId );
        }
    }
    else if( msg->has_depart_room_ind() && loggedIn )
    {
        // If the client is in a room, remove the client.
        auto end = mRoomMap.cend();
        for( auto iter = mRoomMap.cbegin(); iter != end; ++iter )
        {
            ServerRoom* room = iter.value();
            if( room->containsConnection( clientConnection ) )
            {
                room->leave( clientConnection );
                break;
            }
        }
    }
    else
    {
        // This may not be the only handler, so it's not an error to pass on
        // a message.
        mLogger->debug( "unhandled message from client: {}", msg->msg_case() );
    }
}


void
Server::handleClientError( QAbstractSocket::SocketError socketError )
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(QObject::sender());
    mLogger->debug( "client {} error {}", (std::size_t)clientSocket, socketError );
}


void
Server::handleClientDestroyed( QObject* obj )
{
    mLogger->debug( "client obj {} (sender {}) destroyed", (std::size_t)obj, (std::size_t)QObject::sender() );
}


void
Server::handleClientDisconnected()
{
    ClientConnection *clientConnection = qobject_cast<ClientConnection *>(QObject::sender());
    const int localPort = clientConnection->localPort();
    mLogger->debug( "client {} disconnected, peerAddr={}, localPort={}",
            (std::size_t)clientConnection, clientConnection->peerAddress().toString(), localPort );

    // If the client is in a room, remove it.
    auto end = mRoomMap.cend();
    for( auto iter = mRoomMap.cbegin(); iter != end; ++iter )
    {
        ServerRoom* room = iter.value();
        if( room->containsConnection( clientConnection ) )
        {
            room->leave( clientConnection );
            break;
        }
    }

    // If the client was logged in, broadcast that the user is gone.
    auto iter = mClientConnectionLoginMap.find( clientConnection );
    if( iter != mClientConnectionLoginMap.end() )
    {
        mUsersInfoDiffRemovedNames.push_back( iter.value() );
        broadcastUsersInfoDiffs();
    }

    // Remove client from login map.
    mClientConnectionLoginMap.remove( clientConnection );

    // Log network activity for diagnostics.
    mTotalDisconnectedClientBytesSent += clientConnection->getBytesSent();
    mTotalDisconnectedClientBytesReceived += clientConnection->getBytesReceived();
    mLogger->debug( "disconnected clients: bytesSent={}, bytesReceived={}, total={}",
            mTotalDisconnectedClientBytesSent, mTotalDisconnectedClientBytesReceived,
            mTotalDisconnectedClientBytesSent + mTotalDisconnectedClientBytesReceived );

    clientConnection->deleteLater();

}


void
Server::handleAnnouncementsUpdate( const QString& text )
{
    mLogger->trace( "handleAnnouncementsUpdate" );

    // Broadcast the new announcements to all clients.
    const std::string str = text.toStdString();
    for( auto clientConn : mClientConnectionLoginMap.keys() )
    {
        sendAnnouncementsInd( clientConn, str );
    }
}


void
Server::handleAlertUpdate( const QString& text )
{
    mLogger->trace( "handleAlertUpdate" );

    // Broadcast the new alert to all clients.
    const std::string str = text.toStdString();
    for( auto clientConn : mClientConnectionLoginMap.keys() )
    {
        sendAlertsInd( clientConn, str );
    }
}


void
Server::handleRoomPlayerCountChanged( int playerCount )
{
    ServerRoom *room = qobject_cast<ServerRoom*>( QObject::sender() );
    const int roomId = room->getRoomId();

    mLogger->debug( "player count changed: roomId={}, playerCount={}", roomId, playerCount );

    mRoomsInfoDiffPlayerCountsMap.insert( roomId, playerCount );
    armRoomsInfoDiffBroadcastTimer();
}


void
Server::handleRoomExpired()
{
    ServerRoom* room = qobject_cast<ServerRoom*>( QObject::sender() );
    mLogger->info( "room expired: roomId={}", room->getRoomId() );
    teardownRoom( room );
}


void
Server::handleRoomError()
{
    ServerRoom* room = qobject_cast<ServerRoom*>( QObject::sender() );
    mLogger->error( "room error! roomId={}", room->getRoomId() );
    teardownRoom( room );
}


void
Server::teardownRoom( ServerRoom* room )
{
    const int roomId = room->getRoomId();

    // Remove room from room map.
    mRoomMap.remove( roomId );

    // Update room differences and ready an update.
    if( mRoomsInfoDiffAddedRoomIds.contains( roomId ) )
    {
        // It's possible the room was very recently added.  If so, just
        // remove it from the list like it never existed.
        mRoomsInfoDiffAddedRoomIds.removeAll( roomId );
    }
    else
    {
        // Typical case of room being removed.
        mRoomsInfoDiffRemovedRoomIds.push_back( roomId );
    }
    armRoomsInfoDiffBroadcastTimer();

    // Destroy the room.
    room->deleteLater();
}


void
Server::handleRoomsInfoDiffBroadcastTimerTimeout()
{
    mLogger->trace( "handleRoomsInfoDiffBroadcastTimerTimeout" );

    // Broadcast the room update to all clients.
    broadcastRoomsInfoDiffs();
}
