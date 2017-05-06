#include <algorithm>
#include "GridHelper.h"
#include "StringUtil.h"

//------------------------------------------------------------------------
// Draft template implementation
//------------------------------------------------------------------------

template<typename C>
Draft<C>::Draft( const proto::DraftConfig&                   draftConfig,
                 const DraftCardDispenserSharedPtrVector<C>& cardDispensers,
                 const Logging::Config&                      loggingConfig )
  : mDraftConfig( draftConfig ),
    mDraftConfigAdapter( draftConfig ),
    mCardDispensers( cardDispensers ),
    mState( STATE_NEW ),
    mCurrentRound( -1 ),
    mPostRoundTimerStarted( false ),
    mPostRoundTimerTicksRemaining( 0 ),
    mPublicActiveChair( nullptr ),
    mNextPackId( 0 ),
    mProcessingMessageQueue( false ),
    mLogger( loggingConfig.createLogger() )
{
    // Make sure card dispensers size matches config.
    if( mDraftConfig.dispensers_size() != static_cast<int>( mCardDispensers.size() ) )
    {
        mLogger->error( "card dispenser size mismatch! (config {} != actual {})",
                mDraftConfig.dispensers_size(), mCardDispensers.size() );
        mState = STATE_ERROR;
    }

    // Create chairs.
    for( uint32_t i = 0; i < mDraftConfig.chair_count(); ++i )
    {
        mChairs.push_back( new Chair(i) );
        mLogger->debug( "mChairs[{}]={},id={}", i, (std::size_t)mChairs[i], mChairs[i]->getIndex() );
    }
}


template<typename C>
Draft<C>::~Draft()
{
    mLogger->debug( "~Draft" );
    for( Chair *chairPtr : mChairs )
    {
        delete chairPtr;
    }
}


template<typename C>
void
Draft<C>::removeObserver( Observer* observer )
{
    mObservers.erase( std::remove( mObservers.begin(), mObservers.end(), observer ) );
}


template<typename C>
void
Draft<C>::start()
{
    MessageSharedPtr msg( new StartMessage() );
    mMessageQueue.push( msg );
    processMessageQueue();
}


template<typename C>
bool
Draft<C>::makeNamedCardSelection( int chairIndex, uint32_t packId, const C& cardDescriptor )
{
    // Check that chair parameter is valid
    if( (chairIndex < 0) || (chairIndex > getChairCount()) )
    {
        mLogger->error( "invalid chair {}", chairIndex );
        return false;
    }

    MessageSharedPtr msg( new NamedCardSelectionMessage( chairIndex, packId, cardDescriptor ) );
    mMessageQueue.push( msg );
    processMessageQueue();

    return true;
}


template<typename C>
bool
Draft<C>::makeIndexedCardSelection( int chairIndex, uint32_t packId, const std::vector<int>& selectionIndices )
{
    // Check that chair parameter is valid
    if( (chairIndex < 0) || (chairIndex > getChairCount()) )
    {
        mLogger->error( "invalid chair {}", chairIndex );
        return false;
    }

    MessageSharedPtr msg( new IndexedCardSelectionMessage( chairIndex, packId, selectionIndices ) );
    mMessageQueue.push( msg );
    processMessageQueue();

    return true;
}


template<typename C>
bool
Draft<C>::isBoosterRound() const
{
    if( (mCurrentRound < 0) || (mCurrentRound >= mDraftConfig.rounds_size()) ) return false;

    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( mCurrentRound );
    return roundConfig.has_booster_round();
}


template<typename C>
bool
Draft<C>::isSealedRound() const
{
    if( (mCurrentRound < 0) || (mCurrentRound >= mDraftConfig.rounds_size()) ) return false;

    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( mCurrentRound );
    return roundConfig.has_sealed_round();
}


template<typename C>
bool
Draft<C>::isGridRound() const
{
    if( (mCurrentRound < 0) || (mCurrentRound >= mDraftConfig.rounds_size()) ) return false;

    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( mCurrentRound );
    return roundConfig.has_grid_round();
}


template<typename C>
void
Draft<C>::tick()
{
    MessageSharedPtr msg( new TickMessage() );
    mMessageQueue.push( msg );
    processMessageQueue();
}


template<typename C>
int
Draft<C>::getTicksRemaining( int chairIndex ) const
{
    return ((chairIndex >= 0) && (chairIndex < getChairCount())) ? mChairs[chairIndex]->getTicksRemaining() : -1;
}


template<typename C>
int
Draft<C>::getPackQueueSize( int chairIndex ) const
{
    return ((chairIndex >= 0) && (chairIndex < getChairCount())) ? mChairs[chairIndex]->getPackQueueSize() : -1;
}


template<typename C>
std::vector<C>
Draft<C>::getSelectedCards( int chairIndex ) const
{
    std::vector<C> cards;
    if( (chairIndex >= 0) && (chairIndex < getChairCount()) )
    {
        for( auto card : mChairs[chairIndex]->getSelectedCards() )
        {
            cards.push_back( card->getCardDescriptor() );
        }
    }
    return cards;
}


template<typename C>
uint32_t
Draft<C>::getTopPackId( int chairIndex ) const
{
    return (getPackQueueSize( chairIndex ) > 0) ?
            mChairs[chairIndex]->getTopPack()->getPackId() : 0;
}


template<typename C>
std::vector<C>
Draft<C>::getTopPackUnselectedCards( int chairIndex ) const
{
    return (getPackQueueSize( chairIndex ) > 0) ?
        mChairs[chairIndex]->getTopPack()->getUnselectedCardDescriptors() : std::vector<C>();
}


template<typename C>
void
Draft<C>::processMessageQueue()
{
    if( mProcessingMessageQueue ) return;
    mProcessingMessageQueue = true;

    while( !mMessageQueue.empty() && (mState != STATE_ERROR) )
    {
        MessageSharedPtr msg = mMessageQueue.front();
        mMessageQueue.pop();
        if( msg->messageType == MESSAGE_START )
        {
            processStart();
        }
        else if( msg->messageType == MESSAGE_NAMED_CARD_SELECTION )
        {
            NamedCardSelectionMessage* cardSelMsg = static_cast<NamedCardSelectionMessage*>( msg.get() );
            processNamedCardSelection( cardSelMsg->chairIndex, cardSelMsg->packId, cardSelMsg->cardDescriptor );
        }
        else if( msg->messageType == MESSAGE_INDEXED_CARD_SELECTION )
        {
            IndexedCardSelectionMessage* cardSelMsg = static_cast<IndexedCardSelectionMessage*>( msg.get() );
            processIndexedCardSelection( cardSelMsg->chairIndex, cardSelMsg->packId, cardSelMsg->selectionIndices );
        }
        else if( msg->messageType == MESSAGE_TICK )
        {
            processTick();
        }
        else
        {
            // Unhandled message.  Put the draft into an error state.
            mLogger->error( "unknown internal message type {}", msg->messageType );
            enterDraftErrorState();
        }
    }

    mProcessingMessageQueue = false;
}


template<typename C>
void
Draft<C>::processStart()
{
    if( mState != STATE_NEW )
    {
        mLogger->warn( "Cannot start draft in state {}", mState );
        return;
    }

    mState = STATE_RUNNING;
    mCurrentRound = 0;
    startNewRound();
}


template<typename C>
void
Draft<C>::processNamedCardSelection( int chairIndex, uint32_t packId, const C& cardDescriptor )
{
    mLogger->debug( "got chair {} selection: {}", chairIndex, cardDescriptor );

    // Check that chair is valid
    if( (chairIndex < 0) || (chairIndex > getChairCount()) )
    {
        // This should already have been checked!
        mLogger->error( "invalid chair {}", chairIndex );
        enterDraftErrorState();
        return;
    }

    // This message is only valid for booster rounds
    if( !isBoosterRound() )
    {
        mLogger->warn( "invalid round type" );
        for( auto obs : mObservers ) 
        {
            obs->notifyNamedCardSelectionResult( *this, chairIndex, packId, false, cardDescriptor );
        }
        return;
    }

    Chair *chair = mChairs[chairIndex];

    // Get top pack for chair
    PackSharedPtr pack = chair->getTopPack();
    if( pack == nullptr )
    {
        mLogger->warn( "no pack available for chair {}", chairIndex );
        for( auto obs : mObservers ) 
        {
            obs->notifyNamedCardSelectionResult( *this, chairIndex, packId, false, cardDescriptor );
        }
        return;
    }

    if( packId != pack->getPackId() )
    {
        mLogger->warn( "invalid packId ({}) available for chair {} (should be {})", packId, chairIndex, pack->getPackId() );
        for( auto obs : mObservers ) 
        {
            obs->notifyNamedCardSelectionResult( *this, chairIndex, packId, false, cardDescriptor );
        }
        return;
    }

    // Check that card is in current pack for chair and available for selection
    CardSharedPtr card = pack->getFirstUnselectedCard( cardDescriptor );
    if( card == nullptr )
    {
        mLogger->warn( "no unselected card {} in pack {}", cardDescriptor, pack->getPackId() );
        for( auto obs : mObservers ) 
        {
            obs->notifyNamedCardSelectionResult( *this, chairIndex, packId, false, cardDescriptor );
        }
        return;
    }

    // OK, the selection will be valid.  A few things:
    //  - Pop the pack from the chair
    //  - Mark the card selected
    //  - Record the assignment to the chair
    chair->popTopPack();
    int selectionIndexInRound = pack->getSelectedCardCount();
    card->setSelected( chair, mCurrentRound, selectionIndexInRound );
    chair->addSelectedCard( card );

    for( auto obs : mObservers ) 
    {
        obs->notifyPackQueueSizeChanged( *this, chairIndex, chair->getPackQueueSize() );
        obs->notifyNamedCardSelectionResult( *this, chairIndex, packId, true, cardDescriptor );
    }

    //
    // Deal with where the pack goes...
    //

    // Enqueue pack to next chair if there are still unselected cards in the pack.
    // Autopick the last card in the pack to the next player.
    int nextChairIndex = getNextChairIndex( chairIndex );
    Chair *nextChair = mChairs[nextChairIndex];
    bool nextChairWaiting = (nextChair->getPackQueueSize() == 0);

    if( (pack->getUnselectedCardCount() > 1) && nextChairWaiting )
    {
        // If there is more than 1 card in the pack and the next chair is
        // waiting on a pack: enqueue the pack, start the next chair's
        // timer, and inform the chair.
        nextChair->enqueuePack( pack );
        int ticksRemaining = mDraftConfigAdapter.getBoosterRoundSelectionTime( mCurrentRound, 0 );
        nextChair->setTicksRemaining( ticksRemaining );
        for( auto obs : mObservers ) 
        {
            obs->notifyPackQueueSizeChanged( *this, nextChairIndex,
                    nextChair->getPackQueueSize() );
            obs->notifyNewPack( *this, nextChairIndex, pack->getPackId(),
                    pack->getUnselectedCardDescriptors() );
        }
    }
    else if( (pack->getUnselectedCardCount() == 1) && nextChairWaiting )
    {
        // If there is only 1 card left and the next chair is waiting, it
        // can be autopicked to that player.
        CardSharedPtr lastUnselectedCard = pack->getUnselectedCards()[0];
        int selectionIndexInRound = pack->getSelectedCardCount();
        lastUnselectedCard->setSelected( nextChair, mCurrentRound, selectionIndexInRound );
        nextChair->addSelectedCard( lastUnselectedCard );
        for( auto obs : mObservers ) 
        {
            obs->notifyCardAutoselection( *this, nextChairIndex, pack->getPackId(),
                    lastUnselectedCard->getCardDescriptor() );
        }
    }
    else
    {
        // Either there is more than 1 card in the pack or if the next
        // chair is still working on another pack: enqueue the pack to the
        // next chair.
        nextChair->enqueuePack( pack );
        for( auto obs : mObservers ) 
        {
            obs->notifyPackQueueSizeChanged( *this, nextChair->getIndex(), nextChair->getPackQueueSize() );
        }
    }

    //
    // Deal with our next pack...
    //

    if( chair->getPackQueueSize() > 0 )
    {
        PackSharedPtr pack = chair->getTopPack();
        if( pack->getUnselectedCardCount() > 1 )
        {
            int ticksRemaining = mDraftConfigAdapter.getBoosterRoundSelectionTime( mCurrentRound, 0 );
            chair->setTicksRemaining( ticksRemaining );
            for( auto obs : mObservers ) 
            {
                obs->notifyNewPack( *this, chair->getIndex(), pack->getPackId(),
                        pack->getUnselectedCardDescriptors() );
            }
        }
        else if( pack->getUnselectedCardCount() == 1 )
        {
            // Auto-pick the last card from the pack.
            chair->popTopPack();
            CardSharedPtr lastUnselectedCard = pack->getUnselectedCards()[0];
            int selectionIndexInRound = pack->getSelectedCardCount();
            lastUnselectedCard->setSelected( chair, mCurrentRound, selectionIndexInRound );
            chair->addSelectedCard( lastUnselectedCard );
            for( auto obs : mObservers ) 
            {
                obs->notifyPackQueueSizeChanged( *this, chair->getIndex(),
                        chair->getPackQueueSize() );
                obs->notifyCardAutoselection( *this, chairIndex, pack->getPackId(),
                        lastUnselectedCard->getCardDescriptor() );
            }
        }
        else
        {
            // Should never have an empty pack here.
            mLogger->error( "unexpected empty pack {} for chair {}", pack->getPackId(), chairIndex );
            enterDraftErrorState();
        }
    }

    // Pack queue could have been empty or just became empty thanks to an
    // auto-pick.
    if( chair->getPackQueueSize() == 0 )
    {
        // This could be the end of the round, so check and take action if so.
        checkRoundTransition();
    }
}


template<typename C>
void
Draft<C>::processIndexedCardSelection( int chairIndex, uint32_t packId, const std::vector<int>& selectionIndices )
{
    mLogger->debug( "got chair {} public selection {}", chairIndex, StringUtil::stringify( selectionIndices ) );

    // Check that chair is valid
    if( (chairIndex < 0) || (chairIndex > getChairCount()) )
    {
        // This should already have been checked!
        mLogger->error( "invalid chair {}", chairIndex );
        enterDraftErrorState();
        return;
    }

    // Currently this message is only valid for grid rounds
    if( !isGridRound() )
    {
        mLogger->warn( "invalid round type" );
        for( auto obs : mObservers ) 
        {
            obs->notifyIndexedCardSelectionResult( *this, chairIndex, packId, false, selectionIndices, std::vector<C>() );
        }
        return;
    }

    // Check that chair is the active public chair
    if( mPublicActiveChair != mChairs[chairIndex] )
    {
        mLogger->warn( "chair {} is not the active chair", chairIndex );
        for( auto obs : mObservers ) 
        {
            obs->notifyIndexedCardSelectionResult( *this, chairIndex, packId, false, selectionIndices, std::vector<C>() );
        }
        return;
    }

    // Convert selection indices vector to set.
    GridHelper::IndexSet selectedIndicesSet( selectionIndices.begin(), selectionIndices.end() );

    // Build set of unavailable indices and vector of selected cards.
    GridHelper::IndexSet unavailableIndices;
    std::vector<CardSharedPtr> selectedCards;
    for( std::size_t i = 0; i < mPublicPack->getCardCount(); ++i )
    {
        CardSharedPtr card = mPublicPack->getCard( i );
        if( card == nullptr )
        {
            mLogger->error( "nullptr at card {}!", i );
            enterDraftErrorState();
            return;
        }

        if( card->isSelected() ) unavailableIndices.insert( i );

        if( selectedIndicesSet.count( i ) > 0 ) selectedCards.push_back( card );
    }

    GridHelper gridHelper;
    auto availableSelectionsMap = gridHelper.getAvailableSelectionsMap( unavailableIndices );
    auto iter = availableSelectionsMap.find( selectedIndicesSet );
    if( iter == availableSelectionsMap.end() )
    {
        for( auto obs : mObservers ) 
        {
            mLogger->warn( "grid selection invalid" );
            obs->notifyIndexedCardSelectionResult( *this, chairIndex, packId, false, selectionIndices, std::vector<C>() );
        }
        return;
    }

    mLogger->info( "valid grid selection: slice {}", iter->second );

    // Mark the cards as selected in the pack and create desc vector
    std::vector<C> selectedCardDescs;
    for( auto card : selectedCards )
    {
        card->setSelected( mPublicActiveChair, mCurrentRound, mPublicPack->getSelectedCardCount() );
        selectedCardDescs.push_back( card->getCardDescriptor() );
    }

    // Send out confirmations
    for( auto obs : mObservers ) 
    {
        obs->notifyIndexedCardSelectionResult( *this, chairIndex, packId, true, selectionIndices, selectedCardDescs );
    }

    // Clear timer for active chair
    mPublicActiveChair->setTicksRemaining( 0 );

    // Set active chair to next, allowing for end of selections
    int activeChairIndex;
    if( !isSelectionComplete() )
    {
        activeChairIndex = getNextChairIndex( mPublicActiveChair->getIndex() );
        mPublicActiveChair = mChairs[ activeChairIndex ];

        // Reset timer for active chair
        int ticksRemaining = mDraftConfigAdapter.getGridRoundSelectionTime( mCurrentRound, 0 );
        mPublicActiveChair->setTicksRemaining( ticksRemaining );
    }
    else
    {
        activeChairIndex = -1;
        mPublicActiveChair = nullptr;
    }
  
    // Create current public pack state
    std::vector<typename Observer::PublicCardState> cardStates;
    for( std::size_t i = 0; i < mPublicPack->getCardCount(); ++i )
    {
        CardSharedPtr card = mPublicPack->getCard( i );
        int selectedChairIndex = (card->getSelectedChair() != nullptr) ? card->getSelectedChair()->getIndex() : -1;
        typename Observer::PublicCardState state( card->getCardDescriptor(), selectedChairIndex,
                card->getSelectedIndexInRound() );
        cardStates.push_back( state );
    }

    // Notify players of public state.
    for( auto obs : mObservers ) 
    {
        obs->notifyPublicState( *this, mPublicPack->getPackId(), cardStates, activeChairIndex );
    }

    // This could be the end of the round, so check and take action if so.
    checkRoundTransition();
}


template<typename C>
void
Draft<C>::processTick()
{
    mLogger->debug( "tick" );

    // Don't tick unless draft is running.
    if( mState != STATE_RUNNING ) return;

    // Tick down the post-round timer if it's been started.
    if( mPostRoundTimerStarted )
    {
        mPostRoundTimerTicksRemaining--;
    }
    else if( mDraftConfigAdapter.getBoosterRoundSelectionTime( mCurrentRound, 0 ) > 0 )
    {
        // Booster round: tick chair timers, but only if configured for timeouts
        for( auto chair : mChairs )
        {
            // Don't tick unless there are packs on a chair's queue.
            if( chair->getPackQueueSize() > 0 )
            {
                chair->setTicksRemaining( chair->getTicksRemaining() - 1);
                if( chair->getTicksRemaining() <= 0 )
                {
                    for( auto obs : mObservers ) 
                    {
                        PackSharedPtr pack = chair->getTopPack();
                        obs->notifyTimeExpired( *this, chair->getIndex(), pack->getPackId() );
                    }
                }
            }
        }
    }
    else if( mDraftConfigAdapter.getGridRoundSelectionTime( mCurrentRound, 0 ) > 0 )
    {
        // Grid round: tick active chair's timer, but only if configured for timeouts
        mPublicActiveChair->setTicksRemaining( mPublicActiveChair->getTicksRemaining() - 1 );
        if( mPublicActiveChair->getTicksRemaining() <= 0 )
        {
            for( auto obs : mObservers ) 
            {
                obs->notifyTimeExpired( *this, mPublicActiveChair->getIndex(), mPublicPack->getPackId() );
            }
        }
    }

    // Check if a round transition is necessary.
    checkRoundTransition();
}


template<typename C>
void
Draft<C>::startNewRound()
{
    // Check that round is valid.
    if( (mCurrentRound < 0) || (mCurrentRound >= mDraftConfig.rounds_size()) )
    {
        enterDraftErrorState();
        return;
    }

    // Notify all observers of new round.  Note that no packs or cards are
    // in place yet.
    for( auto obs : mObservers ) 
    {
        obs->notifyNewRound( *this, mCurrentRound );
    }

    mPublicActiveChair = nullptr;

    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( mCurrentRound );
    if( roundConfig.has_booster_round() &&
        roundConfig.booster_round().dispensations_size() > 0 )
    {
        // Create packs for each chair.
        for( int i = 0; i < getChairCount(); ++i )
        {
            PackSharedPtr pack = createPackFromDispensations( i, roundConfig.booster_round().dispensations() );

            // Enqueue the pack and notify if it was created.
            if( pack )
            {
                mChairs[i]->enqueuePack( pack );
                for( auto obs : mObservers ) 
                {
                    obs->notifyPackQueueSizeChanged( *this, i, mChairs[i]->getPackQueueSize() );
                }
            }
        }

        // Reset timers and notify players to start making decisions.
        for( auto chair : mChairs )
        {
            int ticksRemaining = mDraftConfigAdapter.getBoosterRoundSelectionTime( mCurrentRound, 0 );
            chair->setTicksRemaining( ticksRemaining );

            for( auto obs : mObservers ) 
            {
                // Notify any chairs that have packs queued.
                if( chair->getPackQueueSize() > 0 )
                {
                    PackSharedPtr pack = chair->getTopPack();
                    obs->notifyNewPack( *this, chair->getIndex(), pack->getPackId(),
                            pack->getUnselectedCardDescriptors() );
                }
            }
        }

    }
    else if( roundConfig.has_sealed_round() &&
        roundConfig.sealed_round().dispensations_size() > 0 )
    {
        // Create packs for each chair.
        for( int i = 0; i < getChairCount(); ++i )
        {
            PackSharedPtr pack = createPackFromDispensations( i, roundConfig.sealed_round().dispensations() );

            if( pack )
            {
                auto cards = pack->getUnselectedCards();  // same as all cards in the new pack
                for( auto c : cards )
                {
                    // Auto-select cards to chair and notify.
                    auto chair = mChairs[i];
                    c->setSelected( chair, mCurrentRound, 0 );
                    chair->addSelectedCard( c );
                    for( auto obs : mObservers ) 
                    {
                        obs->notifyCardAutoselection( *this, i, pack->getPackId(), c->getCardDescriptor() );
                    }
                }
            }
        }
    }
    else if( roundConfig.has_grid_round() &&
        roundConfig.grid_round().dispenser_index() < mCardDispensers.size() )
    {
        mPublicPack = createGridPackFromDispenser( roundConfig.grid_round().dispenser_index() );

        // Set the active chair
        if( roundConfig.grid_round().initial_chair() >= mChairs.size() )
        {
            mLogger->error( "invalid initial chair index!" );
            enterDraftErrorState();
            return;
        }
        mPublicActiveChair = mChairs[ roundConfig.grid_round().initial_chair() ];

        // Reset timer for active chair
        int ticksRemaining = mDraftConfigAdapter.getGridRoundSelectionTime( mCurrentRound, 0 );
        mPublicActiveChair->setTicksRemaining( ticksRemaining );

        // Create public pack state
        std::vector<typename Observer::PublicCardState> cardStates;
        for( std::size_t i = 0; i < mPublicPack->getCardCount(); ++i )
        {
            CardSharedPtr card = mPublicPack->getCard( i );
            int selectedChairIndex = (card->getSelectedChair() != nullptr) ? card->getSelectedChair()->getIndex() : -1;
            typename Observer::PublicCardState state( card->getCardDescriptor(), selectedChairIndex,
                    card->getSelectedIndexInRound() );
            cardStates.push_back( state );
        }

        // Notify players of public state.
        for( auto obs : mObservers ) 
        {
            obs->notifyPublicState( *this, mPublicPack->getPackId(), cardStates, mPublicActiveChair->getIndex() );
        }
    }
    else
    {
        mLogger->error( "unhandled round configuration!" );
        enterDraftErrorState();
        return;
    }

    mPostRoundTimerStarted = false;
    mPostRoundTimerTicksRemaining = roundConfig.has_post_round_timer() ? roundConfig.post_round_timer() : 0;

    // It's possible that the round is over already (very possible for sealed).
    checkRoundTransition();
}


// Create a pack based on dispensations.  The returned pack may be empty
// (nullptr) due to:
//   - no dispensation for the chair index, or
//   - errors in dispensation configuration (i.e. invalid dispenser indices or quantities)
template<typename C>
typename Draft<C>::PackSharedPtr
Draft<C>::createPackFromDispensations( int                                     chairIndex,
                                       const CardDispensationRepeatedPtrField& dispensations )
{
    PackSharedPtr pack;

    // Go through each dispensation looking for stuff for this chair.
    for( auto iter = dispensations.begin(); iter != dispensations.end(); ++iter )
    {
        // If the dispensation contains the chair index, add cards to pack.
        // If the dispensation has no indices at all the chair is implicitly specified.
        if( (iter->chair_indices_size() == 0) ||
            (std::find( iter->chair_indices().begin(), iter->chair_indices().end(), chairIndex ) != iter->chair_indices().end()) )
        {
            const uint32_t cardDispenserIndex = iter->dispenser_index();
            const bool dispenseAll = iter->dispense_all();
            const uint32_t quantity = iter->quantity();

            // Check that the index is legal.
            if( cardDispenserIndex >= mCardDispensers.size() )
            {
                mLogger->error( "invalid card dispenser index! ({})", cardDispenserIndex );
                enterDraftErrorState();
                continue;
            }

            // Must dispense at least one card.
            if( !dispenseAll && (quantity < 1)  )
            {
                mLogger->error( "invalid card dispensation quantity! ({})", quantity );
                enterDraftErrorState();
                continue;
            }

            // We are going to be adding cards to a pack, so create the pack if we haven't already.
            if( !pack ) pack = std::make_shared<Pack>( mNextPackId++ );
            const std::vector<C> cardDescs = dispenseAll ?
                                             mCardDispensers[cardDispenserIndex]->dispenseAll() :
                                             mCardDispensers[cardDispenserIndex]->dispense( quantity );
            for( auto& cardDesc : cardDescs )
            {
                CardSharedPtr c = std::make_shared<Card>( cardDesc );
                pack->addCard( c );
            }
        }
    }

    return pack;
}


template<typename C>
typename Draft<C>::PackSharedPtr
Draft<C>::createGridPackFromDispenser( uint32_t cardDispenserIndex )
{
    const uint32_t GRID_CARD_COUNT = 9;

    PackSharedPtr pack = std::make_shared<Pack>( mNextPackId++ );

    // Check that the index is legal.
    if( cardDispenserIndex >= mCardDispensers.size() )
    {
        mLogger->error( "invalid card dispenser index! ({})", cardDispenserIndex );
        enterDraftErrorState();
        return pack;
    }

    // Generate cards for the grid.
    const std::vector<C> cardDescs = mCardDispensers[cardDispenserIndex]->dispense( GRID_CARD_COUNT );
    for( auto& cardDesc : cardDescs )
    {
        CardSharedPtr c = std::make_shared<Card>( cardDesc );
        pack->addCard( c );
    }

    return pack;
}


template<typename C>
bool
Draft<C>::isSelectionComplete()
{
    if( isBoosterRound() )
    {
        // Booster rounds are incomplete if there's a pack on any queue.
        for( auto chair : mChairs )
        {
            if( chair->getTopPack() != nullptr ) return false;
        }
    }

    if( isGridRound() )
    {
        // Grid rounds are incomplete if any chair has yet to select a card.
        std::set<const Chair*> allChairs( mChairs.begin(), mChairs.end() );
        std::set<const Chair*> selectedChairs;

        // Go through every card and add its owner to a set.
        for( std::size_t i = 0; i < mPublicPack->getCardCount(); ++i )
        {
            CardSharedPtr c = mPublicPack->getCard( i );
            if( c != nullptr )
            {
                const Chair* selectedChair = c->getSelectedChair();
                if( selectedChair != nullptr )
                {
                    selectedChairs.insert( selectedChair );
                }
            }
        }

        if( allChairs != selectedChairs ) return false;
    }

    return true;
}


template<typename C>
bool
Draft<C>::checkRoundTransition()
{
    // A round transition can only happen when selections are complete.
    if( !isSelectionComplete() ) return false;

    // Round is over when the post-round timer has expired.
    if( mPostRoundTimerTicksRemaining == 0 ) 
    {
        mLogger->debug( "round complete" );

        // See if we're moving to another round or if the draft is over.
        mCurrentRound++;
        if( mCurrentRound < getRoundCount() )
        {
            startNewRound();
        }
        else
        {
            mLogger->debug( "draft complete!" );
            mState = STATE_COMPLETE;
            mCurrentRound = -1;
            for( auto obs : mObservers ) 
            {
                obs->notifyDraftComplete( *this );
            }
        }
        return true;
    }
    else
    {
        // Post-round timer has some ticks left.  Notify all observers if
        // the post-round timer just started.
        if( !mPostRoundTimerStarted )
        {
            for( auto obs : mObservers ) 
            {
                obs->notifyPostRoundTimerStarted( *this, mCurrentRound, mPostRoundTimerTicksRemaining );
            }
            mPostRoundTimerStarted = true;
        }
        return false;
    }
}


template<typename C>
int
Draft<C>::getNextChairIndex( int thisChairIndex )
{
    // Compute adjustment: +ve for even rounds, -ve for odd.
    proto::DraftConfig::Direction passDirection =
            mDraftConfigAdapter.getBoosterRoundPassDirection( mCurrentRound, proto::DraftConfig::DIRECTION_CLOCKWISE );
    int adj = (passDirection == proto::DraftConfig::DIRECTION_CLOCKWISE) ? 1 : -1;
    return (getChairCount() + thisChairIndex + adj) % getChairCount();
}


template<typename C>
void
Draft<C>::enterDraftErrorState()
{
    mState = STATE_ERROR;
    for( auto obs : mObservers ) 
    {
        obs->notifyDraftError( *this );
    }
}


//------------------------------------------------------------------------
// Draft::Chair template implementation
//------------------------------------------------------------------------


template<typename C>
Draft<C>::Chair::Chair( int index )
  : mIndex( index ), mTicksRemaining( 0 )
{}

template<typename C>
void
Draft<C>::Chair::enqueuePack( PackSharedPtr pack )
{
    mPackQueue.push( pack );
}

// Return top pack or null if empty.
template<typename C>
typename::Draft<C>::PackSharedPtr
Draft<C>::Chair::getTopPack()
{
    return !mPackQueue.empty() ? mPackQueue.front() : PackSharedPtr(nullptr);
}

template<typename C>
void
Draft<C>::Chair::popTopPack()
{
    if( !mPackQueue.empty() ) mPackQueue.pop();
}

template<typename C>
void
Draft<C>::Chair::addSelectedCard( CardSharedPtr card )
{
    mSelectedCards.push_back( card );
}


//------------------------------------------------------------------------
// Draft::Pack template implementation
//------------------------------------------------------------------------


template<typename C>
void
Draft<C>::Pack::addCard( CardSharedPtr spCard )
{
    mCards.push_back( spCard );
}

template<typename C>
typename Draft<C>::CardSharedPtr
Draft<C>::Pack::getCard( std::size_t index ) const
{
    return (index < mCards.size()) ? mCards[index] : nullptr;
}

template<typename C>
std::size_t
Draft<C>::Pack::getUnselectedCardCount() const
{
    return std::count_if(
            mCards.begin(),
            mCards.end(),
            [] (const CardSharedPtr& card) { return !card->isSelected(); } );
}

template<typename C>
std::size_t
Draft<C>::Pack::getSelectedCardCount() const
{
    return getCardCount() - getUnselectedCardCount();
}

template<typename C>
std::vector<C>
Draft<C>::Pack::getCardDescriptors() const
{
    std::vector<C> cardDescriptors;
    for( auto c : mCards )
    {
        cardDescriptors.push_back( c->getCardDescriptor() );
    }
    return cardDescriptors;
}

template<typename C>
std::vector<typename Draft<C>::CardSharedPtr>
Draft<C>::Pack::getUnselectedCards() const
{
    std::vector<CardSharedPtr> cards;
    std::copy_if(
            mCards.begin(),
            mCards.end(),
            std::back_inserter(cards),
            [] (const CardSharedPtr& card) { return !card->isSelected(); } );
    return cards;
}

template<typename C>
std::vector<C>
Draft<C>::Pack::getUnselectedCardDescriptors() const
{
    std::vector<CardSharedPtr> cards = getUnselectedCards();
    std::vector<C> cardDescriptors;
    for( auto c : cards )
    {
        cardDescriptors.push_back( c->getCardDescriptor() );
    }
    return cardDescriptors;
}

template<typename C>
typename Draft<C>::CardSharedPtr
Draft<C>::Pack::getFirstUnselectedCard( const C& cardDescriptor )
{
    for( auto card : mCards )
    {
        if( (card->getCardDescriptor() == cardDescriptor) && !card->isSelected() ) return card;
    }
    return CardSharedPtr();
}


//------------------------------------------------------------------------
// Draft::Card template implementation
//------------------------------------------------------------------------


template<typename C>
void
Draft<C>::Card::setSelected( const Chair* const chairPtr, int round, int indexInRound )
{
    mSelectedChairPtr = chairPtr;
    mSelectedRound = round;
    mSelectedIndexInRound = indexInRound;
}

