#include "HumanPlayer.h"
#include "SimpleRandGen.h"
#include "SimpleCardData.h"
#include "ProtoHelper.h"

void
HumanPlayer::notifyNewRound( DraftType& draft, int roundIndex )
{
    // Send user a room stage update indication.
    proto::ServerToClientMsg msg;
    proto::RoomStageInd* roomStageInd = msg.mutable_room_stage_ind();
    roomStageInd->set_stage( proto::RoomStageInd::STAGE_RUNNING );
    proto::RoomStageInd::RoundInfo* roundInfo = roomStageInd->mutable_round_info();
    roundInfo->set_round( roundIndex );
    roundInfo->set_round_timed( false ); // not currently used, always false

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending RoomStageInd (STAGE_RUNNING), size={} round={}",
            protoSize, roomStageInd->round_info().round() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::notifyDraftComplete( DraftType& draft )
{
    // Send user a room stage update indication.
    proto::ServerToClientMsg msg;
    proto::RoomStageInd* roomStageInd = msg.mutable_room_stage_ind();
    roomStageInd->set_stage( proto::RoomStageInd::STAGE_COMPLETE );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending RoomStageInd (STAGE_COMPLETE), size={}", protoSize );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::notifyDraftError( DraftType& draft )
{
    // Send user a room error indication.
    proto::ServerToClientMsg msg;
    (void*) msg.mutable_room_error_ind();
    int protoSize = msg.ByteSize();
    mLogger->debug( "sending RoomErrorInd", protoSize );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::notifyNewPack( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards )
{
    // Reset preselection for this pack.
    mPreselectedCard = nullptr;

    // Build the outgoing new pack message from unselected cards in the pack.
    proto::ServerToClientMsg msg;
    proto::PlayerCurrentPackInd* packInd = msg.mutable_player_current_pack_ind();
    packInd->set_pack_id( packId );
    for( auto packCard : unselectedCards )
    {
        proto::Card* card = packInd->add_cards();
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
HumanPlayer::notifyCardSelected( DraftType& draft, uint32_t packId, const DraftCard& card, bool autoSelected )
{
    mLogger->debug( "notifyCardSelected, auto={}", autoSelected );

    // Create card to be added to inventory.
    auto cardData = std::make_shared<SimpleCardData>( card.name, card.setCode );

    if( autoSelected )
    {
        // Send autoselect indication.
        sendPlayerAutoCardSelectionInd(
                proto::PlayerAutoCardSelectionInd::AUTO_LAST_CARD, packId, card );
        mInventory.add( cardData, PlayerInventory::ZONE_AUTO );
    }
    else
    {
        if( mTimeExpired )
        {
            mTimeExpired = false;

            // Send autoselect "time expired" indication.
            sendPlayerAutoCardSelectionInd(
                    proto::PlayerAutoCardSelectionInd::AUTO_TIMED_OUT, packId, card );
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
    if( !mTimeExpired )
    {
        // If time hasn't run out this must have been a player selection
        // and we can send an error response.
        sendPlayerCardSelectionRsp( false, mSelectionPackId, card );
    }
    else
    {
        // Autoselect failed for some reason so do the safest thing and
        // pick the first card in the pack.
        mLogger->error( "error auto-selecting card {}", *mPreselectedCard );
    }
}


void
HumanPlayer::notifyTimeExpired( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards )
{
    // The timer expired.  Select a preselected card if one the client has
    // indicated one, otherwise  do the stupid thing and select a random card.
    mTimeExpired = true;

    // If the client preselected a card make sure it's valid.
    if( mPreselectedCard )
    {
        bool preselectionValid = false;
        for( const DraftCard& dc : unselectedCards )
        {
            if( *mPreselectedCard == dc )
            {
                preselectionValid = true;
                break;
            }
        }
        // Reset the preselected card if it wasn't valid.
        if( !preselectionValid )
        {
            mLogger->warn( "preselected card {} was invalid", *mPreselectedCard );
            mPreselectedCard = nullptr;
        }
    }

    // If a card was preselected, select it.  Otherwise choose at random.
    bool result;
    if( mPreselectedCard )
    {
        mLogger->info( "HumanPlayer at chair {} selecting preselected card {}", getChairIndex(), *mPreselectedCard );
        result = mDraft->makeCardSelection( getChairIndex(), *mPreselectedCard );
    }
    else
    {
        SimpleRandGen rng;
        const int index = rng.generateInRange( 0, unselectedCards.size() - 1 );
        DraftCard stupidCardToSelect = unselectedCards[index];
        mLogger->info( "HumanPlayer at chair {} auto-selecting card {}", getChairIndex(), stupidCardToSelect );
        result = mDraft->makeCardSelection( getChairIndex(), stupidCardToSelect );
    }

    if( !result )
    {
        mLogger->error( "error selecting card {}", *mPreselectedCard );
    }
}


void
HumanPlayer::handleMessageFromClient( const proto::ClientToServerMsg* const msg )
{
    if( msg->has_player_card_preselection_ind() )
    {
        const proto::PlayerCardPreselectionInd& ind = msg->player_card_preselection_ind();
        mPreselectedCard = std::make_shared<DraftCard>( ind.card().name(), ind.card().set_code() );
        mLogger->debug( "client indicated preselection card={}", *mPreselectedCard );
    }
    else if( msg->has_player_card_selection_req() )
    {
        const proto::PlayerCardSelectionReq& req = msg->player_card_selection_req();
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
    else if( msg->has_player_ready_ind() )
    {
        const proto::PlayerReadyInd& ind = msg->player_ready_ind();
        emit readyUpdate( ind.ready() );
    }
    else if( msg->has_player_inventory_update_ind() )
    {
        const proto::PlayerInventoryUpdateInd& ind = msg->player_inventory_update_ind();
        mLogger->debug( "playerInventoryUpdate" );

        bool inSync = true;

        // Handle drafted card moves.
        for( int i = 0; (i < ind.drafted_card_moves_size()) && inSync; ++i )
        {
            const proto::PlayerInventoryUpdateInd::DraftedCardMove& move =
                    ind.drafted_card_moves( i );
            const proto::Card& card = move.card();
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
            const proto::PlayerInventoryUpdateInd::BasicLandAdjustment& adj =
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
        connect( mClientConnection, SIGNAL(msgReceived(const proto::ClientToServerMsg* const)),
                 this, SLOT(handleMessageFromClient(const proto::ClientToServerMsg* const)));
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
    proto::ServerToClientMsg msg;
    proto::PlayerInventoryInd* playerInventoryInd = msg.mutable_player_inventory_ind();

    for( PlayerInventory::ZoneType zone : PlayerInventory::gZoneTypeArray )
    {
        proto::Zone protoZone = convertZone( zone );

        std::vector< std::shared_ptr<CardData> > cardList = mInventory.getCards( zone );
        if( !cardList.empty() )
        {
            mLogger->debug( "player inventory cards ({}): ", stringify( zone ) );
            for( auto c : cardList )
            {
                mLogger->debug( "  {}", c->getName() );
                proto::PlayerInventoryInd::DraftedCard* draftedCard =
                        playerInventoryInd->add_drafted_cards();
                proto::Card* card = draftedCard->mutable_card();
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
                proto::PlayerInventoryInd::BasicLandQuantity* basicLandQty =
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
    proto::ServerToClientMsg msg;
    proto::PlayerCardSelectionRsp* cardSelRsp = msg.mutable_player_card_selection_rsp();
    cardSelRsp->set_result( result );
    cardSelRsp->set_pack_id( packId );
    proto::Card* card = cardSelRsp->mutable_card();
    card->set_name( draftCard.name );
    card->set_set_code( draftCard.setCode );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending playerCardSelectionRsp, size={}", protoSize );
    mLogger->debug( "  isInit={}", cardSelRsp->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::sendPlayerAutoCardSelectionInd( proto::PlayerAutoCardSelectionInd::AutoType type, int packId, const DraftCard& draftCard )
{
    // Build the outgoing new pack message from unselected cards in the pack.
    proto::ServerToClientMsg msg;
    proto::PlayerAutoCardSelectionInd* autoSelInd = msg.mutable_player_auto_card_selection_ind();
    autoSelInd->set_type( type );
    autoSelInd->set_pack_id( packId );
    proto::Card* card = autoSelInd->mutable_card();
    card->set_name( draftCard.name );
    card->set_set_code( draftCard.setCode );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending playerCardAutoSelectionInd, size={}", protoSize );
    mLogger->debug( "  isInit={}", autoSelInd->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::sendServerToClientMsg( const proto::ServerToClientMsg& msg )
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
