#include "HumanPlayer.h"
#include "SimpleRandGen.h"
#include "SimpleCardData.h"
#include "ProtoHelper.h"


void
HumanPlayer::notifyNewRound( DraftType& draft, int roundIndex )
{
    // Reset round-based flags.
    mCurrentPackPresent = false;
}

void
HumanPlayer::notifyNewPack( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards )
{
    // Save the state data for future rejoin or autoselection.
    mCurrentPackPresent = true;
    mCurrentPackId = packId;
    mCurrentPackUnselectedCards = unselectedCards;

    // Reset preselection for this pack.
    mPreselectedCard = nullptr;

    // Send user the public state indication.
    sendCurrentPackInd();
}


void
HumanPlayer::notifyPublicState( DraftType& draft, uint32_t packId, const std::vector<PublicCardState>& cardStates, int activeChairIndex )
{
    mPublicCardStates = cardStates;
}


void
HumanPlayer::notifyNamedCardSelectionResult( DraftType& draft, uint32_t packId, bool result, const DraftCard& card )
{
    mLogger->debug( "notifyNamedCardSelectionResult" );

    // Create card to be added to inventory.
    auto cardData = std::make_shared<SimpleCardData>( card.name, card.setCode );

    if( result )
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
            sendPlayerNamedCardSelectionRsp( true, packId, card );
            mInventory.add( cardData, mNamedSelectionZone );
        }
    }
    else
    {
        if( !mTimeExpired )
        {
            // If time hasn't run out this must have been a player selection
            // and we can send an error response.
            sendPlayerNamedCardSelectionRsp( false, packId, card );
        }
        else
        {
            // Autoselect failed for some reason!
            mLogger->error( "error auto-selecting card!" );
        }
    }
}


void
HumanPlayer::notifyIndexedCardSelectionResult( DraftType& draft, uint32_t packId, bool result, const std::vector<int>& selectionIndices, const std::vector<DraftCard>& cards )
{
    mLogger->debug( "notifyIndexedCardSelectionResult" );

    if( result )
    {
        if( mTimeExpired )
        {
            mTimeExpired = false;

            // Send autoselect "time expired" indications.
            for( const auto& card : cards )
            {
                sendPlayerAutoCardSelectionInd(
                        proto::PlayerAutoCardSelectionInd::AUTO_TIMED_OUT, packId, card );
                auto cardData = std::make_shared<SimpleCardData>( card.name, card.setCode );
                mInventory.add( cardData, PlayerInventory::ZONE_AUTO );
            }
        }
        else
        {
            // Send affirmative response to request.
            sendPlayerIndexedCardSelectionRsp( true, packId, selectionIndices, cards );
            for( const auto& card : cards )
            {
                auto cardData = std::make_shared<SimpleCardData>( card.name, card.setCode );
                mInventory.add( cardData, mIndexedSelectionZone );
            }
        }
    }
    else
    {
        if( !mTimeExpired )
        {
            // If time hasn't run out this must have been a player selection
            // and we can send an error response.
            sendPlayerIndexedCardSelectionRsp( false, packId, selectionIndices, cards );
        }
        else
        {
            // Autoselect failed for some reason!
            mLogger->error( "error auto-selecting indexed cards!" );
        }
    }
}


void
HumanPlayer::notifyCardAutoselection( DraftType& draft, uint32_t packId, const DraftCard& card )
{
    // Create card to be added to inventory.
    auto cardData = std::make_shared<SimpleCardData>( card.name, card.setCode );

    // Send autoselect indication.
    sendPlayerAutoCardSelectionInd( proto::PlayerAutoCardSelectionInd::AUTO_LAST_CARD, packId, card );
    mInventory.add( cardData, PlayerInventory::ZONE_AUTO );
}


void
HumanPlayer::notifyTimeExpired( DraftType& draft, uint32_t packId )
{
    // The timer expired.  Select a preselected card if the client has
    // indicated one, otherwise do the stupid thing and select a random card.
    mTimeExpired = true;

    if( draft.isBoosterRound() )
    {
        handleTimeExpiredBoosterRound( draft, packId );
    }
    else if( draft.isGridRound() )
    {
        handleTimeExpiredGridRound( draft, packId );
    }
    else
    {
        mLogger->warn( "time expired for unhandled round type" );
    }
}


void
HumanPlayer::handleTimeExpiredBoosterRound( DraftType& draft, uint32_t packId )
{
    // If the client preselected a card make sure it's valid.
    if( mPreselectedCard )
    {
        bool preselectionValid = false;
        for( const DraftCard& dc : mCurrentPackUnselectedCards )
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
        result = mDraft->makeNamedCardSelection( getChairIndex(), packId, *mPreselectedCard );
    }
    else
    {
        SimpleRandGen rng;
        const int index = rng.generateInRange( 0, mCurrentPackUnselectedCards.size() - 1 );
        DraftCard stupidCardToSelect = mCurrentPackUnselectedCards[index];
        mLogger->info( "HumanPlayer at chair {} auto-selecting card {}", getChairIndex(), stupidCardToSelect );
        result = mDraft->makeNamedCardSelection( getChairIndex(), packId, stupidCardToSelect );
    }

    if( !result )
    {
        mLogger->error( "error selecting card {}", *mPreselectedCard );
    }
}


void
HumanPlayer::handleTimeExpiredGridRound( DraftType& draft, uint32_t packId )
{
    GridHelper gh;
    if( mPublicCardStates.size() < gh.getIndexCount() )
    {
        mLogger->warn( "less than {} cards ({}) in grid pack!", gh.getIndexCount(), mPublicCardStates.size() );
        return;
    }

    // Build set of unavailable indices.
    GridHelper::IndexSet unavailableIndices;
    for( std::size_t i = 0; i < mPublicCardStates.size(); ++i )
    {
        if( mPublicCardStates[i].getSelectedChairIndex() != -1 ) unavailableIndices.insert( i );
    }

    // Get available selections.
    GridHelper gridHelper;
    auto availableSelectionsMap = gridHelper.getAvailableSelectionsMap( unavailableIndices );
    if( availableSelectionsMap.empty() )
    {
        mLogger->error( "no available selections in grid pack!" );
        return;
    }

    // Randomly pick a selection.
    SimpleRandGen rng;
    const int adv = rng.generateInRange( 0, availableSelectionsMap.size() - 1 );
    auto iter = availableSelectionsMap.begin();
    std::advance( iter, adv );

    mLogger->debug( "selecting slice {}", iter->second );
    std::vector<int> indices( iter->first.begin(), iter->first.end() );
    bool result = draft.makeIndexedCardSelection( getChairIndex(), packId, indices );

    if( !result )
    {
        mLogger->warn( "error selecting public cards" );
    }
}


void
HumanPlayer::handleMessageFromClient( const proto::ClientToServerMsg& msg )
{
    if( msg.has_player_named_card_preselection_ind() )
    {
        const proto::PlayerNamedCardPreselectionInd& ind = msg.player_named_card_preselection_ind();
        mPreselectedCard = std::make_shared<DraftCard>( ind.card().name(), ind.card().set_code() );
        mLogger->debug( "client indicated preselection card={}", *mPreselectedCard );
    }
    else if( msg.has_player_named_card_selection_req() )
    {
        const proto::PlayerNamedCardSelectionReq& req = msg.player_named_card_selection_req();
        DraftCard card( req.card().name(), req.card().set_code() );
        mLogger->debug( "client requested named selection pack_id={},card={}", req.pack_id(), card );
        mNamedSelectionZone = convertZone( req.zone() );
        bool result = mDraft->makeNamedCardSelection( getChairIndex(), req.pack_id(), card );
        if( !result )
        {
            // Notify of error (currently always saying invalid card)
            sendPlayerNamedCardSelectionRsp( false, req.pack_id(), card );
        }
    }
    else if( msg.has_player_indexed_card_selection_req() )
    {
        const proto::PlayerIndexedCardSelectionReq& req = msg.player_indexed_card_selection_req();
        std::vector<int> indices;
        for( auto index : req.indices() )
        {
            indices.push_back( index );
        }
        mLogger->debug( "client requested indexed selection pack_id={},indices={}", req.pack_id(), StringUtil::stringify( indices ) );
        mIndexedSelectionZone = convertZone( req.zone() );
        bool result = mDraft->makeIndexedCardSelection( getChairIndex(), req.pack_id(), indices );
        if( !result )
        {
            // Notify of error (currently always saying invalid card)
            sendPlayerIndexedCardSelectionRsp( false, req.pack_id(), indices, std::vector<DraftCard>() );
        }
    }
    else if( msg.has_player_ready_ind() )
    {
        const proto::PlayerReadyInd& ind = msg.player_ready_ind();
        emit readyUpdate( ind.ready() );
    }
    else if( msg.has_player_inventory_update_ind() )
    {
        const proto::PlayerInventoryUpdateInd& ind = msg.player_inventory_update_ind();
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
        mLogger->debug( "unhandled message from client: {}", msg.msg_case() );
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
        connect( mClientConnection, &ClientConnection::protoMsgReceived,
                 this,              &HumanPlayer::handleMessageFromClient );
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
HumanPlayer::sendPlayerInventoryInd() const
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
HumanPlayer::sendCurrentPackInd() const
{
    if( !mCurrentPackPresent )
    {
        mLogger->info( "current pack not present, not sending PlayerCurrentPackInd" );
        return;
    }

    // Build the outgoing new pack message from unselected cards in the pack.
    proto::ServerToClientMsg msg;
    proto::PlayerCurrentPackInd* packInd = msg.mutable_player_current_pack_ind();
    packInd->set_pack_id( mCurrentPackId );
    for( auto packCard : mCurrentPackUnselectedCards )
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
HumanPlayer::sendPlayerNamedCardSelectionRsp( bool             result,
                                              int              packId,
                                              const DraftCard& draftCard )
{
    proto::ServerToClientMsg msg;
    proto::PlayerNamedCardSelectionRsp* cardSelRsp = msg.mutable_player_named_card_selection_rsp();
    cardSelRsp->set_result( result );
    cardSelRsp->set_pack_id( packId );
    proto::Card* card = cardSelRsp->mutable_card();
    card->set_name( draftCard.name );
    card->set_set_code( draftCard.setCode );

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending playerNamedCardSelectionRsp, size={}", protoSize );
    mLogger->debug( "  isInit={}", cardSelRsp->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::sendPlayerIndexedCardSelectionRsp( bool result, int packId, const std::vector<int> selectionIndices, const std::vector<DraftCard>& cards )
{
    if( selectionIndices.size() != cards.size() )
    {
        mLogger->warn( "sendPlayerIndexedCardSelectionRsp size mismatch!  {} != {}", selectionIndices.size(), cards.size() );
        return;
    }

    proto::ServerToClientMsg msg;
    proto::PlayerIndexedCardSelectionRsp* cardSelRsp = msg.mutable_player_indexed_card_selection_rsp();
    cardSelRsp->set_result( result );
    cardSelRsp->set_pack_id( packId );

    for( std::size_t i = 0; i < selectionIndices.size(); ++i )
    {
        cardSelRsp->add_indices( selectionIndices[i] );
        proto::Card* card = cardSelRsp->add_cards();
        card->set_name( cards[i].name );
        card->set_set_code( cards[i].setCode );
    }

    int protoSize = msg.ByteSize();
    mLogger->debug( "sending playerIndexedCardSelectionRsp, size={}", protoSize );
    mLogger->debug( "  isInit={}", cardSelRsp->IsInitialized() );

    sendServerToClientMsg( msg );
}


void
HumanPlayer::sendPlayerAutoCardSelectionInd( proto::PlayerAutoCardSelectionInd::AutoType type, int packId, const DraftCard& draftCard )
{
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
HumanPlayer::sendServerToClientMsg( const proto::ServerToClientMsg& msg ) const
{
    if( mClientConnection != 0 )
    {
        mClientConnection->sendProtoMsg( msg );
    }
    else
    {
        mLogger->debug( "dropping message - no client connection" );
    }
}
