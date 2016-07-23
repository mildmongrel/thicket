#include <algorithm>

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
    mRoundTicksRemaining( 0 ),
    mNextPackId( 0 ),
    mProcessingMessageQueue( false ),
    mLogger( loggingConfig.createLogger() )
{
    // Make sure card dispensers size matches config.
    if( mDraftConfig.dispensers_size() != mCardDispensers.size() )
    {
        mLogger->error( "card dispenser size mismatch! (config {} != actual {})",
                mDraftConfig.dispensers_size(), mCardDispensers.size() );
        mState = STATE_ERROR;
    }

    // Create chairs.
    for( int i = 0; i < mDraftConfig.chair_count(); ++i )
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
Draft<C>::makeCardSelection( int chairIndex, const C& cardDescriptor )
{
    // Check that chair parameter is valid
    if( (chairIndex < 0) || (chairIndex > getChairCount()) )
    {
        mLogger->error( "invalid chair {}", chairIndex );
        return false;
    }

    MessageSharedPtr msg( new CardSelectionMessage( chairIndex, cardDescriptor ) );
    mMessageQueue.push( msg );
    processMessageQueue();

    return true;
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
        else if( msg->messageType == MESSAGE_CARD_SELECTION )
        {
            CardSelectionMessage* cardSelMsg = static_cast<CardSelectionMessage*>( msg.get() );
            processCardSelection( cardSelMsg->chairIndex, cardSelMsg->cardDescriptor );
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
Draft<C>::processCardSelection( int chairIndex, const C& cardDescriptor )
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

    Chair *chair = mChairs[chairIndex];

    // Get top pack for chair
    PackSharedPtr pack = chair->getTopPack();
    if( pack == nullptr )
    {
        mLogger->warn( "no pack available for chair {}", chairIndex );
        for( auto obs : mObservers ) 
        {
            obs->notifyCardSelectionError( *this, chairIndex, cardDescriptor );
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
            obs->notifyCardSelectionError( *this, chairIndex, cardDescriptor );
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
        obs->notifyPackQueueSizeChanged( *this, chairIndex,
                chair->getPackQueueSize() );
        obs->notifyCardSelected( *this, chairIndex, pack->getPackId(),
                card->getCardDescriptor(), false );
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
            obs->notifyCardSelected( *this, nextChairIndex, pack->getPackId(),
                    lastUnselectedCard->getCardDescriptor(), true );
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
                obs->notifyCardSelected( *this, chairIndex, pack->getPackId(),
                        lastUnselectedCard->getCardDescriptor(), true );
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
        if( isRoundComplete() )
        {
            doRoundTransition();
        }
    }
}


template<typename C>
void
Draft<C>::processTick()
{
    mLogger->debug( "tick" );

    // Don't tick unless draft is running.
    if( mState != STATE_RUNNING ) return;

    // Tick chair timers, but only if the round is configured for timeouts
    if( mDraftConfigAdapter.getBoosterRoundSelectionTime( mCurrentRound, 0 ) > 0 )
    {
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
                        obs->notifyTimeExpired( *this, chair->getIndex(),
                                pack->getPackId(), pack->getUnselectedCardDescriptors() );
                    }
                }
            }
        }
    }

    // Tick the round timer, but only once all selections are complete.
    if( isSelectionComplete() )
    {
        mRoundTicksRemaining--;

        // See if the round has ended due to timer.
        if( isRoundComplete() )
        {
            doRoundTransition();
        }
    }
}


template<typename C>
void
Draft<C>::startNewRound()
{
    // Notify all observers of new round.  Note that no packs or cards are
    // in place yet.
    for( auto obs : mObservers ) 
    {
        obs->notifyNewRound( *this, mCurrentRound );
    }

    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( mCurrentRound );
    if( roundConfig.has_booster_round() &&
        roundConfig.booster_round().dispensations_size() > 0 )
    {
        // Create packs for each chair.
        for( int i = 0; i < getChairCount(); ++i )
        {
            PackSharedPtr pack; // start with nullptr

            // Go through each dispensation looking for stuff for this chair.
            for( int dispIdx = 0; dispIdx < roundConfig.booster_round().dispensations_size(); ++dispIdx )
            {
                // If the dispensation contains the chair index, add cards to pack.
                const proto::DraftConfig::CardDispensation& disp = roundConfig.booster_round().dispensations( dispIdx );
                if( std::find( disp.chair_indices().begin(), disp.chair_indices().end(), i ) !=
                        disp.chair_indices().end() )
                {
                    const int cardDispenserIndex = disp.dispenser_index();

                    // Check that the index is legal.
                    if( cardDispenserIndex >= mCardDispensers.size() )
                    {
                        mLogger->error( "invalid card dispenser index!" );
                        enterDraftErrorState();
                        return;
                    }

                    // We are going to be adding cards to a pack, so create
                    // the pack if we haven't already.
                    if( !pack ) pack = std::make_shared<Pack>( mNextPackId++ );

                    // Dispense and add cards to the pack.
                    const std::vector<C> cardDescs = mCardDispensers[cardDispenserIndex]->dispense();
                    for( auto& cardDesc : cardDescs )
                    {
                        CardSharedPtr c = std::make_shared<Card>( cardDesc );
                        pack->addCard( c );
                    }
                }
            }

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

        // TODO this code shares a LOT in common with the booster portion
        // try to commonize part that generates dispensations for each chair

        // Create packs for each chair.
        for( int i = 0; i < getChairCount(); ++i )
        {
            PackSharedPtr pack; // start with nullptr

            // Go through each dispensation looking for stuff for this chair.
            for( int dispIdx = 0; dispIdx < roundConfig.sealed_round().dispensations_size(); ++dispIdx )
            {
                // If the dispensation contains the chair index, add cards to pack.
                const proto::DraftConfig::CardDispensation& disp = roundConfig.sealed_round().dispensations( dispIdx );
                if( std::find( disp.chair_indices().begin(), disp.chair_indices().end(), i ) !=
                        disp.chair_indices().end() )
                {
                    const int cardDispenserIndex = disp.dispenser_index();

                    // Check that the index is legal.
                    if( cardDispenserIndex >= mCardDispensers.size() )
                    {
                        mLogger->error( "invalid card dispenser index!" );
                        enterDraftErrorState();
                        return;
                    }

                    // We are going to be adding cards to a pack, so create
                    // the pack if we haven't already.
                    if( !pack ) pack = std::make_shared<Pack>( mNextPackId++ );

                    // Dispense cards to the pack.
                    const std::vector<C> cardDescs = mCardDispensers[cardDispenserIndex]->dispense();

                    for( auto& cardDesc : cardDescs )
                    {
                        // Auto-select cards to chair and notify.
                        CardSharedPtr c = std::make_shared<Card>( cardDesc );
                        pack->addCard( c );
                        auto chair = mChairs[i];
                        c->setSelected( chair, mCurrentRound, 0 );
                        chair->addSelectedCard( c );
                        for( auto obs : mObservers ) 
                        {
                            obs->notifyCardSelected( *this, i, pack->getPackId(), c->getCardDescriptor(), true );
                        }
                    }
                }
            }
        }
    }
    else
    {
        mLogger->error( "unhandled round configuration!" );
        enterDraftErrorState();
        return;
    }

    mRoundTicksRemaining = roundConfig.has_timer() ? roundConfig.timer() : 0;

    // It's possible that the round is over already (very possible for sealed).
    if( isRoundComplete() )
    {
        doRoundTransition();
    }
}


template<typename C>
bool
Draft<C>::isSelectionComplete()
{
    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( mCurrentRound );
    if( roundConfig.has_booster_round() )
    {
        // Booster rounds are incomplete if there's a pack on any queue.
        for( auto chair : mChairs )
        {
            if( chair->getTopPack() != nullptr ) return false;
        }
    }
    return true;
}


template<typename C>
bool
Draft<C>::isRoundComplete()
{
    // Round is over when selections are done and the round timer has expired.
    return isSelectionComplete() && (mRoundTicksRemaining <= 0);
}


template<typename C>
void
Draft<C>::doRoundTransition()
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
int
Draft<C>::Pack::getUnselectedCardCount() const
{
    return std::count_if(
            mCards.begin(),
            mCards.end(),
            [] (const CardSharedPtr& card) { return !card->isSelected(); } );
}

template<typename C>
int
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

