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

    virtual void notifyPackQueueSizeChanged( DraftType& draft, int packQueueSize ) override { /* no-op */ }
    virtual void notifyNewPack( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards ) override;
    virtual void notifyCardSelected( DraftType& draft, uint32_t packId, const DraftCard& card, bool autoSelected ) override;
    virtual void notifyCardSelectionError( DraftType& draft, const DraftCard& card ) override;
    virtual void notifyTimeExpired( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards ) override;
    virtual void notifyNewRound( DraftType& draft, int roundIndex ) override;
    virtual void notifyDraftComplete( DraftType& draft ) override;
    virtual void notifyDraftError( DraftType& draft ) override;

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

    uint32_t                  mSelectionPackId;
    PlayerInventory::ZoneType mSelectionZone;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
