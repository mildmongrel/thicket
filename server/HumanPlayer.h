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
        mCurrentPackPresent( false ),
        mLogger( loggingConfig.createLogger() )
    {
        setName( "human " + std::to_string( chairIndex ) );
    }

    // Observer callbacks.  Many are no-ops that are handled by a room-wide observer.
    virtual void notifyPackQueueSizeChanged( DraftType& draft, int packQueueSize ) override { /* no-op */ }
    virtual void notifyNewPack( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards ) override;
    virtual void notifyPublicState( DraftType& draft, uint32_t packId, const std::vector<PublicCardState>& cardStates, int activeChairIndex ) override;
    virtual void notifyNamedCardSelectionResult( DraftType& draft, uint32_t packId, bool result, const DraftCard& card ) override;
    virtual void notifyIndexedCardSelectionResult( DraftType& draft, uint32_t packId, bool result, const std::vector<int>& selectionIndices, const std::vector<DraftCard>& cards ) override;
    virtual void notifyCardAutoselection( DraftType& draft, uint32_t packId, const DraftCard& card ) override;
    virtual void notifyTimeExpired( DraftType& draft, uint32_t packId ) override;
    virtual void notifyPostRoundTimerStarted( DraftType& draft, int roundIndex, int ticksRemaining ) override { /* no-op */ }
    virtual void notifyNewRound( DraftType& draft, int roundIndex ) override;
    virtual void notifyDraftComplete( DraftType& draft ) override { /* no-op */ }
    virtual void notifyDraftError( DraftType& draft ) override { /* no-op */ }

    ClientConnection* getClientConnection() const { return mClientConnection; }
    void setClientConnection( ClientConnection* c );
    void removeClientConnection();

    void sendInventoryToClient() const { sendPlayerInventoryInd(); }
    void sendCurrentPackToClient() const { sendCurrentPackInd(); }

    QString getCockatriceHash() const { return computeCockatriceHash( mInventory ); }

signals:
    void readyUpdate( bool ready );
    void deckUpdate();

private slots:
    void handleMessageFromClient( const proto::ClientToServerMsg& msg );

private:
    void handleTimeExpiredBoosterRound( DraftType& draft, uint32_t packId );
    void handleTimeExpiredGridRound( DraftType& draft, uint32_t packId );
    void sendPlayerInventoryInd() const;
    void sendPlayerNamedCardSelectionRsp( bool result, int packId, const DraftCard& card );
    void sendPlayerIndexedCardSelectionRsp( bool result, int packId, const std::vector<int> selectionIndices, const std::vector<DraftCard>& cards );
    void sendCurrentPackInd() const;
    void sendPlayerAutoCardSelectionInd( proto::PlayerAutoCardSelectionInd::AutoType type, int packId, const DraftCard& card );

    void sendServerToClientMsg( const proto::ServerToClientMsg& msg ) const;

    ClientConnection* mClientConnection;
    DraftType*        mDraft;
    PlayerInventory   mInventory;
    bool              mTimeExpired;

    bool                       mCurrentPackPresent;
    uint32_t                   mCurrentPackId;
    std::vector<DraftCard>     mCurrentPackUnselectedCards;
    std::shared_ptr<DraftCard> mPreselectedCard;

    std::vector<PublicCardState> mPublicCardStates;

    PlayerInventory::ZoneType mNamedSelectionZone;
    PlayerInventory::ZoneType mIndexedSelectionZone;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
