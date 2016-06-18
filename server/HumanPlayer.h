#ifndef HUMANPLAYER_H
#define HUMANPLAYER_H

#include <QObject>
#include "Draft.h"
#include "DraftTypes.h"
#include "Player.h"
#include "ClientConnection.h"
#include "PlayerInventory.h"
#include "DeckHashing.h"

// send messages via clientconnection, and register for clientconnection newmsg signal, filtering on what's important
class HumanPlayer : public QObject, public Player {
    Q_OBJECT
public:
    HumanPlayer( int chairIndex, DraftType* draft, Logging::Config loggingConfig = Logging::Config(), QObject* parent = 0 )
      : QObject( parent ),
        Player( chairIndex ),
        mClientConnection( 0 ),
        mDraft( draft ),
        mTimeExpired( false ),
        mLogger( loggingConfig.createLogger() )
    {
        setName( "human " + std::to_string( chairIndex ) );
    }

    virtual void notifyPackQueueSizeChanged( DraftType& draft, int packQueueSize ) { /* no-op */ }
    virtual void notifyNewPack( DraftType& draft, const DraftPackId& packId, const std::vector<DraftCard>& unselectedCards );
    virtual void notifyCardSelected( DraftType& draft, const DraftPackId& packId, const DraftCard& card, bool autoSelected );
    virtual void notifyCardSelectionError( DraftType& draft, const DraftCard& card );
    virtual void notifyTimeExpired( DraftType& draft, const DraftPackId& packId, const std::vector<DraftCard>& unselectedCards );
    virtual void notifyNewRound( DraftType& draft, int roundIndex, const DraftRoundInfo& round );
    virtual void notifyDraftComplete( DraftType& draft );

    ClientConnection* getClientConnection() const { return mClientConnection; }
    void setClientConnection( ClientConnection* c );
    void removeClientConnection();

    void sendInventoryToClient() { sendPlayerInventoryInd(); }

    QString getCockatriceHash() const { return computeCockatriceHash( mInventory ); }

signals:
    void readyUpdate( bool ready );
    void deckUpdate();

private slots:
    void handleMessageFromClient( const thicket::ClientToServerMsg* const msg );

private:
    void sendPlayerInventoryInd();
    void sendPlayerCardSelectionRsp( bool result, int packId, const DraftCard& card );
    void sendPlayerAutoCardSelectionInd( thicket::PlayerAutoCardSelectionInd::AutoType type, int packId, const DraftCard& card );

    void sendServerToClientMsg( const thicket::ServerToClientMsg& msg );

    ClientConnection* mClientConnection;
    DraftType*        mDraft;
    PlayerInventory   mInventory;
    bool              mTimeExpired;

    DraftPackId               mSelectionPackId;
    PlayerInventory::ZoneType mSelectionZone;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
