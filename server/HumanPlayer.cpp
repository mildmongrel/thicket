#include "HumanPlayer.h"
#include "SimpleRandGen.h"
#include "SimpleCardData.h"
#include "ProtoHelper.h"

void
HumanPlayer::notifyNewRound( DraftType& draft, int roundIndex, const DraftRoundInfo& round )
{
    thicket::ServerToClientMsg msg;
    thicket::RoomStageInd* roomStageInd = msg.mutable_room_stage_ind();
    roomStageInd->set_round( roundIndex );
    roomStageInd->set_complete( false );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending RoomStageInd, size={}", protoSize );
    mLogger->debug( "  round={}", roomStageInd->round() );
    mLogger->debug( "  complete={}", roomStageInd->complete() );
    mLogger->debug( "  isInit={}", roomStageInd->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::notifyDraftComplete( DraftType& draft )
{
    thicket::ServerToClientMsg msg;
    thicket::RoomStageInd* roomStageInd = msg.mutable_room_stage_ind();
    roomStageInd->set_round( -1 );
    roomStageInd->set_complete( true );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending RoomStateInd, size={}", protoSize );
    mLogger->debug( "  round={}", roomStageInd->round() );
    mLogger->debug( "  complete={}", roomStageInd->complete() );
    mLogger->debug( "  isInit={}", roomStageInd->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::notifyNewPack( DraftType& draft, const DraftPackId& packId, const std::vector<DraftCard>& unselectedCards )
{
    // Build the outgoing new pack message from unselected cards in the pack.
    thicket::ServerToClientMsg msg;
    thicket::PlayerCurrentPackInd* packInd = msg.mutable_player_current_pack_ind();
    packInd->set_pack_id( packId );
    for( auto packCard : unselectedCards )
    {
        thicket::Card* card = packInd->add_cards();
        card->set_name( packCard.name );
        card->set_set_code( packCard.setCode );
    }

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending playerCurrentPackInd, size={}", protoSize );
    mLogger->debug( "  cardsSize={}", packInd->cards_size() );
    mLogger->debug( "  isInit={}", packInd->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::notifyCardSelected( DraftType& draft, const DraftPackId& packId, const DraftCard& card, bool autoSelected )
{
    mLogger->debug( "notifyCardSelected, auto={}", autoSelected );

    // Create card to be added to inventory.
    auto cardData = std::make_shared<SimpleCardData>( card.name, card.setCode );

    if( autoSelected )
    {
        // Send autoselect indication.
        sendPlayerAutoCardSelectionInd(
                thicket::PlayerAutoCardSelectionInd::AUTO_LAST_CARD, packId, card );
        mInventory.add( cardData, PlayerInventory::ZONE_AUTO );
    }
    else
    {
        if( mTimeExpired )
        {
            mTimeExpired = false;

            // Send autoselect "time expired" indication.
            sendPlayerAutoCardSelectionInd(
                    thicket::PlayerAutoCardSelectionInd::AUTO_TIMED_OUT, packId, card );
            mInventory.add( cardData, PlayerInventory::ZONE_AUTO );
        }
        else
        {
            // Send affirmative response to request.
            sendPlayerCardSelectionRsp( true, packId, card );
            mInventory.add( cardData, mSelectionZone );
        }
    }
}


void
HumanPlayer::notifyCardSelectionError( DraftType& draft, const DraftCard& card )
{
    sendPlayerCardSelectionRsp( false, mSelectionPackId, card );
}


void
HumanPlayer::notifyTimeExpired( DraftType& draft, const DraftPackId& packId, const std::vector<DraftCard>& unselectedCards )
{
    // The timer expired.  Here's where we would kick in a preference for auto-picking a card.
    // For now do the stupid thing and select a random card.
    mTimeExpired = true;
    SimpleRandGen rng;
    const int index = rng.generateInRange( 0, unselectedCards.size() - 1 );
    DraftCard stupidCardToSelect = unselectedCards[index];
    mLogger->info( "HumanPlayer at chair {} selecting card {}", getChairIndex(), stupidCardToSelect );
    bool result = mDraft->makeCardSelection( getChairIndex(), stupidCardToSelect );
    if( !result )
    {
        mLogger->warn( "error selecting card {}", stupidCardToSelect );
    }
}


void
HumanPlayer::handleMessageFromClient( const thicket::ClientToServerMsg* const msg )
{
    if( msg->has_player_card_selection_req() )
    {
        const thicket::PlayerCardSelectionReq& req = msg->player_card_selection_req();
        DraftCard card( req.card().name(), req.card().set_code() );
        mLogger->debug( "client requested selection pack_id={},card={}", req.pack_id(), card );
        mSelectionPackId = req.pack_id();
        mSelectionZone = convertZone( req.zone() );
        bool result = mDraft->makeCardSelection( getChairIndex(), card );
        if( !result )
        {
            // Notify of error (currently always saying invalid card)
            sendPlayerCardSelectionRsp( false, req.pack_id(), card );
        }
    }
    if( msg->has_player_ready_ind() )
    {
        const thicket::PlayerReadyInd& ind = msg->player_ready_ind();
        emit readyUpdate( ind.ready() );
    }
    if( msg->has_player_inventory_update_ind() )
    {
        const thicket::PlayerInventoryUpdateInd& ind = msg->player_inventory_update_ind();
        mLogger->debug( "playerInventoryUpdate" );

        bool inSync = true;

        // Handle drafted card moves.
        for( int i = 0; (i < ind.drafted_card_moves_size()) && inSync; ++i )
        {
            const thicket::PlayerInventoryUpdateInd::DraftedCardMove& move =
                    ind.drafted_card_moves( i );
            const thicket::Card& card = move.card();
            mLogger->debug( "  {}: {} -> {}", card.name(), move.zone_from(), move.zone_to() );
            auto cardData = std::make_shared<SimpleCardData>( card.name(), card.set_code() );

            bool moveOk = mInventory.move( cardData,
                    convertZone( move.zone_from() ), convertZone( move.zone_to() ) );
            if( !moveOk )
            {
                // Error moving a card.  This should never happen, but if
                // it somehow does, set a flag to resync the client.
                mLogger->warn( "error moving card - client out of sync!" );
                inSync = false;
            }
        }

        // Handle basic land adjustments.
        for( int i = 0; (i < ind.basic_land_adjustments_size()) && inSync; ++i )
        {
            const thicket::PlayerInventoryUpdateInd::BasicLandAdjustment& adj =
                    ind.basic_land_adjustments( i );
            mLogger->debug( "  {}: {} -> {}", stringify( adj.basic_land() ),
                    stringify( adj.zone() ), adj.adjustment() );

            bool adjOk = mInventory.adjustBasicLand( convertBasicLand( adj.basic_land() ),
                    convertZone( adj.zone() ), adj.adjustment() );
            if( !adjOk )
            {
                // Error adjusting a basic land.  This should never happen,
                // but if it somehow does, set a flag to resync the client.
                mLogger->warn( "error adjusting basic land - client out of sync!" );
                inSync = false;
            }
        }

        if( !inSync )
        {
            mLogger->notice( "resyncing client with new inventory" );
            sendPlayerInventoryInd();
        }

        emit deckUpdate();
    }
    else
    {
        // This may not be the only handler, so it's not an error to pass on
        // a message.
        mLogger->debug( "unhandled message from client: {}", msg->msg_case() );
    }
}


void
HumanPlayer::setClientConnection( ClientConnection* c )
{
    if( mClientConnection != 0 )
    {
        removeClientConnection();
    }

    mClientConnection = c;

    if( mClientConnection != 0 )
    {
        connect( mClientConnection, SIGNAL(msgReceived(const thicket::ClientToServerMsg* const)),
                 this, SLOT(handleMessageFromClient(const thicket::ClientToServerMsg* const)));
    }
}


void
HumanPlayer::removeClientConnection()
{
    if( mClientConnection != 0 )
    {
        disconnect( mClientConnection, 0, this, 0 );
        mClientConnection = 0;
    }
}


void
HumanPlayer::sendPlayerInventoryInd()
{
    mLogger->trace( "sendPlayerInventoryInd" );
    thicket::ServerToClientMsg msg;
    thicket::PlayerInventoryInd* playerInventoryInd = msg.mutable_player_inventory_ind();

    for( PlayerInventory::ZoneType zone : PlayerInventory::gZoneTypeArray )
    {
        thicket::Zone protoZone = convertZone( zone );

        std::vector< std::shared_ptr<CardData> > cardList = mInventory.getCards( zone );
        if( !cardList.empty() )
        {
            mLogger->debug( "player inventory cards ({}): ", stringify( zone ) );
            for( auto c : cardList )
            {
                mLogger->debug( "  {}", c->getName() );
                thicket::PlayerInventoryInd::DraftedCard* draftedCard =
                        playerInventoryInd->add_drafted_cards();
                thicket::Card* card = draftedCard->mutable_card();
                card->set_name( c->getName() );
                card->set_set_code( c->getSetCode() );
                draftedCard->set_zone( protoZone );
            }
        }

        BasicLandQuantities basicLandQtys = mInventory.getBasicLandQuantities( zone );
        for( auto basic : gBasicLandTypeArray )
        {
            int qty = basicLandQtys.getQuantity( basic );
            if( qty > 0 )
            {
                mLogger->debug( "basic land ({}) ({}): {}", stringify( basic ),
                        stringify( zone ), qty );
                thicket::PlayerInventoryInd::BasicLandQuantity* basicLandQty =
                        playerInventoryInd->add_basic_land_qtys();
                basicLandQty->set_basic_land( convertBasicLand( basic ) );
                basicLandQty->set_zone( protoZone );
                basicLandQty->set_quantity( qty );
            }
        }

    }

    sendServerToClientMsg( msg );
}


void
HumanPlayer::sendPlayerCardSelectionRsp( bool result, int packId, const DraftCard& draftCard )
{
    // Build the outgoing new pack message from unselected cards in the pack.
    thicket::ServerToClientMsg msg;
    thicket::PlayerCardSelectionRsp* cardSelRsp = msg.mutable_player_card_selection_rsp();
    cardSelRsp->set_result( result );
    cardSelRsp->set_pack_id( packId );
    thicket::Card* card = cardSelRsp->mutable_card();
    card->set_name( draftCard.name );
    card->set_set_code( draftCard.setCode );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending playerCardSelectionRsp, size={}", protoSize );
    mLogger->debug( "  isInit={}", cardSelRsp->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::sendPlayerAutoCardSelectionInd( thicket::PlayerAutoCardSelectionInd::AutoType type, int packId, const DraftCard& draftCard )
{
    // Build the outgoing new pack message from unselected cards in the pack.
    thicket::ServerToClientMsg msg;
    thicket::PlayerAutoCardSelectionInd* autoSelInd = msg.mutable_player_auto_card_selection_ind();
    autoSelInd->set_type( type );
    autoSelInd->set_pack_id( packId );
    thicket::Card* card = autoSelInd->mutable_card();
    card->set_name( draftCard.name );
    card->set_set_code( draftCard.setCode );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending playerCardAutoSelectionInd, size={}", protoSize );
    mLogger->debug( "  isInit={}", autoSelInd->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::sendServerToClientMsg( const thicket::ServerToClientMsg& msg )
{
    if( mClientConnection != 0 )
    {
        mClientConnection->sendMsg( &msg );
    }
    else
    {
        mLogger->debug( "dropping message - no client connection" );
    }
}
