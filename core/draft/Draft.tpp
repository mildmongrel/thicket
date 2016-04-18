#include <algorithm>

//------------------------------------------------------------------------
// Draft template implementation
//------------------------------------------------------------------------

template<typename R,typename P,typename C>
Draft<R,P,C>::Draft( int chairCount, const std::vector<RoundConfiguration>& roundConfigs, const Logging::Config &loggingConfig )
  : mRoundConfigs( roundConfigs ),
    mState( STATE_NEW ),
    mCurrentRound( -1 ),
    mScopeCount( 0 ),
    mLogger( loggingConfig.createLogger() )
{
    // Create chairs.
    for( int i = 0; i < chairCount; ++i )
    {
        mChairs.push_back( new Chair(i) );
        mLogger->debug( "mChairs[{}]={},id={}", i, (std::size_t)mChairs[i], mChairs[i]->getIndex() );
    }
}


template<typename R,typename P,typename C>
Draft<R,P,C>::~Draft()
{
    mLogger->debug( "~Draft" );
    for( Chair *chairPtr : mChairs )
    {
        delete chairPtr;
    }
}


template<typename R,typename P,typename C>
void
Draft<R,P,C>::removeObserver( Observer* observer )
{
    mObservers.erase( std::remove( mObservers.begin(), mObservers.end(), observer ) );
}


template<typename R,typename P,typename C>
void
Draft<R,P,C>::go()
{
    mState = STATE_RUNNING;
    mCurrentRound = 0;
    startNewRound();
}


template<typename R,typename P,typename C>
R
Draft<R,P,C>::getCurrentRoundDescriptor() const
{
    return (mState == STATE_RUNNING) ? mRoundConfigs[mCurrentRound].getRoundDescriptor() : R();
}


template<typename R,typename P,typename C>
void
Draft<R,P,C>::tick()
{
    ScopeCounter sc( mScopeCount );

    mLogger->debug( "tick" );

    // Don't tick unless draft is running.
    if( mState != STATE_RUNNING ) return;

    // Don't tick if the round is configured for no timeouts
    const RoundConfiguration& roundConfig = mRoundConfigs[mCurrentRound];
    if( roundConfig.getTimeoutTicks() == 0 ) return;

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
                    obs->notifyTimeExpired( *this, chair->getIndex(), pack->getPackDescriptor(), pack->getUnselectedCardDescriptors() );
                }
            }
        }
    }

    processCardSelections();
}


template<typename R,typename P,typename C>
bool
Draft<R,P,C>::makeCardSelection( int chairIndex, const C& cardDescriptor )
{
    ScopeCounter sc( mScopeCount );

    // Check that chair is valid
    if( (chairIndex < 0) || (chairIndex > getChairCount()) )
    {
        mLogger->error( "invalid chair {}", chairIndex );
        return false;
    }

    mCardSelectionQueue.push( std::make_pair( chairIndex, cardDescriptor ) );

    // This ensures that the card selection queue isn't processed in the context
    // of an observer callback, which can screw up internal data structures.
    //
    if( !sc.inNestedScope() )
    {
        processCardSelections();
    }

    return true;
}


template<typename R,typename P,typename C>
int
Draft<R,P,C>::getTicksRemaining( int chairIndex ) const
{
    return ((chairIndex >= 0) && (chairIndex < getChairCount())) ? mChairs[chairIndex]->getTicksRemaining() : -1;
}


template<typename R,typename P,typename C>
int
Draft<R,P,C>::getPackQueueSize( int chairIndex ) const
{
    return ((chairIndex >= 0) && (chairIndex < getChairCount())) ? mChairs[chairIndex]->getPackQueueSize() : -1;
}


template<typename R,typename P,typename C>
std::vector<C>
Draft<R,P,C>::getSelectedCards( int chairIndex ) const
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


template<typename R,typename P,typename C>
P
Draft<R,P,C>::getTopPackDescriptor( int chairIndex ) const
{
    return (getPackQueueSize( chairIndex ) > 0) ? mChairs[chairIndex]->getTopPack()->getPackDescriptor() : P();
}


template<typename R,typename P,typename C>
std::vector<C>
Draft<R,P,C>::getTopPackUnselectedCards( int chairIndex ) const
{
    return (getPackQueueSize( chairIndex ) > 0) ?
        mChairs[chairIndex]->getTopPack()->getUnselectedCardDescriptors() : std::vector<C>();
}


template<typename R,typename P,typename C>
void
Draft<R,P,C>::startNewRound()
{
    ScopeCounter sc( mScopeCount );

    // TODO should this be done after all packs are in place?
    for( auto obs : mObservers ) 
    {
        obs->notifyNewRound( *this, mCurrentRound, mRoundConfigs[mCurrentRound].getRoundDescriptor() );
    }

    // Use the current round configuration to create internal Pack and Card
    // objects, enqueuing them on Chair objects as called for.
    const RoundConfiguration& roundConfig = mRoundConfigs[mCurrentRound];
    for( int i = 0; i < getChairCount(); ++i )
    {
        if( roundConfig.hasPack( i ) )
        {
            const P& packDesc = roundConfig.getPackDescriptor( i );
            PackSharedPtr p = std::make_shared<Pack>( Pack( packDesc ) );

            const std::vector<C>& cardDescs = roundConfig.getPackCards( i );
            for( auto& cardDesc : cardDescs )
            {
                 CardSharedPtr c = std::make_shared<Card>( Card( cardDesc ) );
                 p->addCard( c );
            }

            mChairs[i]->enqueuePack(p);
            for( auto obs : mObservers ) 
            {
                obs->notifyPackQueueSizeChanged( *this, i, mChairs[i]->getPackQueueSize() );
            }
        }
    }

    // Reset timers and notify players to start making decisions.
    for( auto chair : mChairs )
    {
        chair->setTicksRemaining( roundConfig.getTimeoutTicks() );
        for( auto obs : mObservers ) 
        {
            // Notify any chairs that have packs queued.
            if( chair->getPackQueueSize() > 0 )
            {
                PackSharedPtr pack = chair->getTopPack();
                obs->notifyNewPack( *this, chair->getIndex(), pack->getPackDescriptor(), pack->getUnselectedCardDescriptors() );
            }
        }
    }

    processCardSelections();
}


template<typename R,typename P,typename C>
void
Draft<R,P,C>::processCardSelections()
{
    while( !mCardSelectionQueue.empty() )
    {
        auto cardSel = mCardSelectionQueue.front();
        mCardSelectionQueue.pop();
        processCardSelection( cardSel.first, cardSel.second );
    }
}


template<typename R,typename P,typename C>
void
Draft<R,P,C>::processCardSelection( int chairIndex, const C& cardDescriptor )
{
    mLogger->debug( "got chair {} selection: {}", chairIndex, cardDescriptor );

    // Check that chair is valid
    if( (chairIndex < 0) || (chairIndex > getChairCount()) )
    {
        // This should already have been checked!
        mLogger->critical( "invalid chair {}", chairIndex );
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
        mLogger->warn( "no unselected card {} in pack {}", cardDescriptor, (std::size_t)pack.get() );
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
        obs->notifyPackQueueSizeChanged( *this, chair->getIndex(), chair->getPackQueueSize() );
        obs->notifyCardSelected( *this, chairIndex, pack->getPackDescriptor(), card->getCardDescriptor(), false );
    }

    // Enqueue pack to next chair if there are still unselected cards in the pack.
    // Autopick the last card in the pack to the next player.
    int nextChairIndex = getNextChairIndex( chairIndex );
    Chair *nextChair = mChairs[nextChairIndex];
    bool nextChairWaiting = (nextChair->getPackQueueSize() == 0);

    if( (pack->getUnselectedCardCount() > 1) || !nextChairWaiting )
    {
        // If there is more than 1 card in the pack or if the next chair is still
        // working on another pack, enqueue the pack to the next chair.
        nextChair->enqueuePack( pack );
        for( auto obs : mObservers ) 
        {
            obs->notifyPackQueueSizeChanged( *this, nextChair->getIndex(), nextChair->getPackQueueSize() );
        }
        if( nextChairWaiting )
        {
            nextChair->setTicksRemaining( mRoundConfigs[mCurrentRound].getTimeoutTicks() );
            for( auto obs : mObservers ) 
            {
                obs->notifyNewPack( *this, nextChairIndex, pack->getPackDescriptor(), pack->getUnselectedCardDescriptors() );
            }
        }
    }
    else if( (pack->getUnselectedCardCount() == 1) && nextChairWaiting )
    {
        // If there is only 1 card left and the next chair is waiting, it
        // can be autopicked to that player.
        CardSharedPtr lastUnselectedCard = pack->getUnselectedCards()[0];  // TODO better checking
        int selectionIndexInRound = pack->getSelectedCardCount();
        lastUnselectedCard->setSelected( nextChair, mCurrentRound, selectionIndexInRound );
        nextChair->addSelectedCard( lastUnselectedCard );
        for( auto obs : mObservers ) 
        {
            obs->notifyCardSelected( *this, nextChairIndex, pack->getPackDescriptor(), lastUnselectedCard->getCardDescriptor(), true );
        }
    }

    // If the player has a new pack waiting in the queue, notify of a new pack to work on.
    // Otherwise it might be the end of the round, so check.
    bool newPackNotified = false;
    while( (chair->getPackQueueSize() > 0) && !newPackNotified )
    {
        PackSharedPtr pack = chair->getTopPack();
        if( pack->getUnselectedCardCount() == 1 )
        {
            // Auto-pick the last card from the pack.
            chair->popTopPack();
            CardSharedPtr lastUnselectedCard = pack->getUnselectedCards()[0];
            int selectionIndexInRound = pack->getSelectedCardCount();
            lastUnselectedCard->setSelected( chair, mCurrentRound, selectionIndexInRound );
            chair->addSelectedCard( lastUnselectedCard );
            for( auto obs : mObservers ) 
            {
                obs->notifyPackQueueSizeChanged( *this, chair->getIndex(), chair->getPackQueueSize() );
                obs->notifyCardSelected( *this, chairIndex, pack->getPackDescriptor(), lastUnselectedCard->getCardDescriptor(), true );
            }

            // This could be the end of the round, so check and take action if so.
            if( isRoundComplete() )
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
        }
        else
        {
            chair->setTicksRemaining( mRoundConfigs[mCurrentRound].getTimeoutTicks() );
            for( auto obs : mObservers ) 
            {
                obs->notifyNewPack( *this, chair->getIndex(), pack->getPackDescriptor(), pack->getUnselectedCardDescriptors() );
            }
            newPackNotified = true;
        }
    }
}

template<typename R,typename P,typename C>
int
Draft<R,P,C>::getNextChairIndex( int thisChairIndex )
{
    // Compute adjustment: +ve for even rounds, -ve for odd.
    int adj = (mRoundConfigs[mCurrentRound].getPassDirection() == CLOCKWISE) ? 1 : -1;
    return (getChairCount() + thisChairIndex + adj) % getChairCount();
}

template<typename R,typename P,typename C>
bool
Draft<R,P,C>::isRoundComplete()
{
    // If all chairs' pack queues are empty, the round is complete.
    for( auto chair : mChairs )
    {
        if( chair->getTopPack() != nullptr ) return false;
    }
    return true;
}


//------------------------------------------------------------------------
// Draft::RoundConfigurations template implementation
//------------------------------------------------------------------------


template<typename R,typename P,typename C>
typename Draft<R,P,C>::RoundConfiguration&
Draft<R,P,C>::RoundConfiguration::setPack(
        int chairIndex,
        const P& packDesc,
        const std::vector<C>& cardDescs )
{
    PackContents contents;
    contents.packDesc = packDesc;
    contents.cardDescs = cardDescs;
    chairsToPacksMap[chairIndex] = contents;
    return *this;
}


//------------------------------------------------------------------------
// Draft::Chair template implementation
//------------------------------------------------------------------------


template<typename R,typename P,typename C>
Draft<R,P,C>::Chair::Chair( int index )
  : mIndex( index ), mTicksRemaining( 0 )
{}

template<typename R,typename P,typename C>
void
Draft<R,P,C>::Chair::enqueuePack( PackSharedPtr pack )
{
    mPackQueue.push( pack );
}

// Return top pack or null if empty.
template<typename R,typename P,typename C>
typename::Draft<R,P,C>::PackSharedPtr
Draft<R,P,C>::Chair::getTopPack()
{
    return !mPackQueue.empty() ? mPackQueue.front() : PackSharedPtr(nullptr);
}

template<typename R,typename P,typename C>
void
Draft<R,P,C>::Chair::popTopPack()
{
    if( !mPackQueue.empty() ) mPackQueue.pop();
}

template<typename R,typename P,typename C>
void
Draft<R,P,C>::Chair::addSelectedCard( CardSharedPtr card )
{
    mSelectedCards.push_back( card );
}


//------------------------------------------------------------------------
// Draft::Pack template implementation
//------------------------------------------------------------------------


template<typename R,typename P,typename C>
void
Draft<R,P,C>::Pack::addCard( CardSharedPtr spCard )
{
    mCards.push_back( spCard );
}

template<typename R,typename P,typename C>
int
Draft<R,P,C>::Pack::getUnselectedCardCount() const
{
    return std::count_if(
            mCards.begin(),
            mCards.end(),
            [] (const CardSharedPtr& card) { return !card->isSelected(); } );
}

template<typename R,typename P,typename C>
int
Draft<R,P,C>::Pack::getSelectedCardCount() const
{
    return getCardCount() - getUnselectedCardCount();
}

template<typename R,typename P,typename C>
std::vector<C>
Draft<R,P,C>::Pack::getCardDescriptors() const
{
    std::vector<C> cardDescriptors;
    for( auto c : mCards )
    {
        cardDescriptors.push_back( c->getCardDescriptor() );
    }
    return cardDescriptors;
}

template<typename R,typename P,typename C>
std::vector<typename Draft<R,P,C>::CardSharedPtr>
Draft<R,P,C>::Pack::getUnselectedCards() const
{
    std::vector<CardSharedPtr> cards;
    std::copy_if(
            mCards.begin(),
            mCards.end(),
            std::back_inserter(cards),
            [] (const CardSharedPtr& card) { return !card->isSelected(); } );
    return cards;
}

template<typename R,typename P,typename C>
std::vector<C>
Draft<R,P,C>::Pack::getUnselectedCardDescriptors() const
{
    std::vector<CardSharedPtr> cards = getUnselectedCards();
    std::vector<C> cardDescriptors;
    for( auto c : cards )
    {
        cardDescriptors.push_back( c->getCardDescriptor() );
    }
    return cardDescriptors;
}

template<typename R,typename P,typename C>
typename Draft<R,P,C>::CardSharedPtr
Draft<R,P,C>::Pack::getFirstUnselectedCard( const C& cardDescriptor )
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


template<typename R,typename P,typename C>
void
Draft<R,P,C>::Card::setSelected( const Chair* const chairPtr, int round, int indexInRound )
{
    mSelectedChairPtr = chairPtr;
    mSelectedRound = round;
    mSelectedIndexInRound = indexInRound;
}

