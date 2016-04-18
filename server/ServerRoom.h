#ifndef SERVERROOM_H
#define SERVERROOM_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

#include <QList>
#include <QMap>
#include <memory>

#include "messages.pb.h"
#include "Draft.h"

#include "RoomConfigPrototype.h"

#include "Logging.h"
#include "DraftTypes.h"

class ConnectionServer;
class ClientConnection;
class Player;
class BotPlayer;
class HumanPlayer;

class ServerRoom : public QObject, DraftObserverType
{
    Q_OBJECT

public:

    using RoomConfigPrototypeSharedPtr = std::shared_ptr<RoomConfigPrototype>;

    enum ChairState
    {
        CHAIR_STATE_EMPTY,
        CHAIR_STATE_STANDBY,      // connected, not ready to draft
        CHAIR_STATE_READY,        // connected, ready to draft
        CHAIR_STATE_ACTIVE,       // connected, drafting or finished drafting
        CHAIR_STATE_DEPARTED      // departed (may return)
    };

public:

    ServerRoom( unsigned int                        roomId,
                const RoomConfigPrototypeSharedPtr& roomConfigPrototype,
                const Logging::Config&              loggingConfig = Logging::Config(),
                QObject*                            parent = 0 );

    virtual ~ServerRoom();

    unsigned int getRoomId() const { return mRoomId; }
    
    const std::string getName() const { return mRoomConfigPrototype->getRoomName(); }

    unsigned int getChairCount() const { return mChairCount; }

    // Count of players currently in room.
    unsigned int getPlayerCount() const { return mBotList.size() + mHumanList.size(); }

    bool containsConnection( ClientConnection* clientConnection ) const { return mClientConnectionMap.contains( clientConnection ); }
    bool containsHumanPlayer( const std::string& name ) const { return getHumanPlayer( name ) != nullptr; }
    QList<ClientConnection*> getClientConnections() const { return mClientConnectionMap.keys(); }

    // Join a user connection to the room.
    bool join( ClientConnection* clientConnection, const std::string& name, const std::string& password, int& chairIndex );

    // Process a client disconnect from the room.
    void leave( ClientConnection* clientConnection );

    // Rejoin a previously-disconnected user to the room.
    bool rejoin( ClientConnection* clientConnection, const std::string& name );

    bool getChairInfo( unsigned int  chairIndex,
                       std::string&  name,           // output
                       bool&         isBot,          // output
                       ChairState&   state,          // output
                       unsigned int& packsQueued,    // output
                       unsigned int& ticksRemaining  /* output */ ) const;

    std::shared_ptr<RoomConfigPrototype> getRoomConfigPrototype() const
    {
        return mRoomConfigPrototype;
    }

signals:

    void playerCountChanged( int playerCount );
    void roomExpired();

private slots:

    void initialize();
    void handleDraftTimerTick();
    void handleHumanReadyUpdate( bool ready );
    void handleHumanDeckUpdate();

private:  // Methods

    void sendJoinRoomSuccessRspInd( ClientConnection* clientConnection,
                                    int               roomId,
                                    bool              rejoin,
                                    int               chairIndex );
    void sendJoinRoomFailureRsp( ClientConnection*                      clientConnection,
                                 thicket::JoinRoomFailureRsp_ResultType result,
                                 int                                    roomId );
    void broadcastRoomOccupantsInfo();
    void broadcastRoomChairsInfo();
    void broadcastRoomChairsDeckInfo( const HumanPlayer& human );

    int getNextAvailablePlayerIndex() const;
    HumanPlayer* getHumanPlayer( const std::string& name ) const;

    // --- Draft Observer BEGIN ---
    virtual void notifyPackQueueSizeChanged( DraftType& draft, int chairIndex, int packQueueSize );
    virtual void notifyNewPack( DraftType& draft, int chairIndex, const DraftPackId& pack, const std::vector<DraftCard>& unselectedCards ) {}
    virtual void notifyCardSelected( DraftType& draft, int chairIndex, const DraftPackId& pack, const DraftCard& card, bool autoSelected ) {}
    virtual void notifyCardSelectionError( DraftType& draft, int chairIndex, const DraftCard& card ) {}
    virtual void notifyTimeExpired( DraftType& draft,int chairIndex, const DraftPackId& pack, const std::vector<DraftCard>& unselectedCards ) {}
    virtual void notifyNewRound( DraftType& draft, int roundIndex, const DraftRoundInfo& round ) {}
    virtual void notifyDraftComplete( DraftType& draft );
    // --- Draft Observer END ---

private:  // Data

    const unsigned int                 mRoomId;
    const RoomConfigPrototypeSharedPtr mRoomConfigPrototype;
    const unsigned int                 mChairCount;
    const unsigned int                 mBotPlayerCount;

    DraftType* mDraftPtr;
    bool       mDraftComplete;

    QTimer *mRoomExpirationTimer;
    QTimer *mDraftTimer;

    // Lists of specific occupant types.
    QList<BotPlayer*>   mBotList;
    QList<HumanPlayer*> mHumanList;

    // This list is specially preallocated with 0's and filled in with
    // players as the room is filled.
    QList<Player*> mPlayerList;

    // This list mirrors the mPlayerList and contains state entries for
    // every chair index.
    QList<ChairState> mChairStateList;

    // Map of connections to player objects.  Used when connections come
    // and go; a disconnection doesn't imply that the player object is
    // removed - it is allowed to make bot-like decisions until a
    // reconnection can take place.
    QMap<ClientConnection*,HumanPlayer*> mClientConnectionMap;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};


inline std::string stringify( const ServerRoom::ChairState& state )
{
    switch( state )
    {
        case ServerRoom::CHAIR_STATE_EMPTY:     return "EMPTY";
        case ServerRoom::CHAIR_STATE_STANDBY:   return "STANDBY";
        case ServerRoom::CHAIR_STATE_READY:     return "READY";
        case ServerRoom::CHAIR_STATE_ACTIVE:    return "ACTIVE";
        case ServerRoom::CHAIR_STATE_DEPARTED:  return "DEPARTED";
        default:                                return std::string();
    }
}

#endif
