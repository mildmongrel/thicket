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

    enum ChairState
    {
        CHAIR_STATE_EMPTY,
        CHAIR_STATE_STANDBY,      // connected, not ready to draft
        CHAIR_STATE_READY,        // connected, ready to draft
        CHAIR_STATE_ACTIVE,       // connected, drafting or finished drafting
        CHAIR_STATE_DEPARTED      // departed (may return)
    };

public:

    ServerRoom( unsigned int                                        roomId,
                const std::string&                                  password,
                const proto::RoomConfig&                            roomConfig,
                const DraftCardDispenserSharedPtrVector<DraftCard>& dispensers,
                const Logging::Config&                              loggingConfig = Logging::Config(),
                QObject*                                            parent = 0 );

    virtual ~ServerRoom();

    unsigned int getRoomId() const { return mRoomId; }
    
    const std::string getName() const { return mRoomConfig.name(); }

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

    const proto::RoomConfig& getRoomConfig() const
    {
        return mRoomConfig;
    }

signals:

    void playerCountChanged( int playerCount );
    void roomExpired();
    void roomError();

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
    void sendJoinRoomFailureRsp( ClientConnection*                    clientConnection,
                                 proto::JoinRoomFailureRsp_ResultType result,
                                 int                                  roomId );
    void broadcastRoomOccupantsInfo();
    void broadcastBoosterDraftState();
    void sendPublicState( const QList<ClientConnection*> clientConnections );
    void broadcastRoomChairsDeckInfo( const HumanPlayer& human );

    int getNextAvailablePlayerIndex() const;
    HumanPlayer* getHumanPlayer( const std::string& name ) const;

    int getPostRoundTimeRemainingMillis() const;

    // --- Draft Observer BEGIN ---
    virtual void notifyPackQueueSizeChanged( DraftType& draft, int chairIndex, int packQueueSize ) override;
    virtual void notifyNewPack( DraftType& draft, int chairIndex, uint32_t packId, const std::vector<DraftCard>& unselectedCards ) override {}
    virtual void notifyPublicState( DraftType& draft, uint32_t packId, const std::vector<PublicCardState>& cardStates, int activeChairIndex) override;
    virtual void notifyNamedCardSelectionResult( DraftType& draft, int chairIndex, uint32_t packId, bool result, const DraftCard& card ) override {}
    virtual void notifyIndexedCardSelectionResult( DraftType& draft, int chairIndex, uint32_t packId, bool result, const std::vector<int>& selectionIndices, const std::vector<DraftCard>& cards ) override {}
    virtual void notifyCardAutoselection( DraftType& draft, int chairIndex, uint32_t packId, const DraftCard& card ) override {}
    virtual void notifyTimeExpired( DraftType& draft,int chairIndex, const uint32_t packId ) override {}
    virtual void notifyPostRoundTimerStarted( DraftType& draft, int roundIndex, int ticksRemaining ) override;
    virtual void notifyNewRound( DraftType& draft, int roundIndex ) override;
    virtual void notifyDraftComplete( DraftType& draft ) override;
    virtual void notifyDraftError( DraftType& draft ) override;
    // --- Draft Observer END ---

private:  // Data

    const unsigned int       mRoomId;
    const std::string        mPassword;
    const proto::RoomConfig  mRoomConfig;
    const DraftCardDispenserSharedPtrVector<DraftCard> mDispensers;
    const unsigned int       mChairCount;
    const unsigned int       mBotPlayerCount;

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

    // Public state information.
    bool                         mPublicStatePresent;
    uint32_t                     mPublicPackId;
    std::vector<PublicCardState> mPublicCardStates;
    int                          mPublicActiveChairIndex;

    bool mPostRoundTimerActive;
    int  mPostRoundTimerTicksRemaining;

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
