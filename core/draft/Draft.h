#ifndef DRAFT_H
#define DRAFT_H

#include <memory>
#include <map>
#include <queue>
#include <vector>

#include "Logging.h"

template< typename TRoundDescriptor = std::string,
          typename TPackDescriptor  = std::string,
          typename TCardDescriptor  = std::string >
class Draft
{
public:

    enum StateType
    {
        STATE_NEW,
        STATE_RUNNING,
        STATE_COMPLETE
    };

    enum PassDirectionType
    {
        CLOCKWISE,
        COUNTERCLOCKWISE
    };

    //--------------------------------------------------------------------

    // Draft object gets a collection of these that configure each round
    // Setting ticks to zero implies no timeout.
    class RoundConfiguration
    {
    public:

        RoundConfiguration( const TRoundDescriptor& roundDescriptor )
          : mRoundDescriptor( roundDescriptor ), mPassDirection( CLOCKWISE ), mTimeoutTicks( 30 ) {}

        RoundConfiguration& setRoundDescriptor( const TRoundDescriptor &roundDescriptor )
        {
            mRoundDescriptor = roundDescriptor;
            return *this;
        }
        const TRoundDescriptor& getRoundDescriptor() const { return mRoundDescriptor; }

        PassDirectionType getPassDirection() const { return mPassDirection; }
        RoundConfiguration& setPassDirection( const PassDirectionType& passDirection )
        {
            mPassDirection = passDirection;
            return *this;
        }

        RoundConfiguration& setTimeoutTicks( int timeoutTicks )
        {
            mTimeoutTicks = timeoutTicks;
            return *this;
        }
        int getTimeoutTicks() const { return mTimeoutTicks; }

        bool hasPack( int chairIndex ) const { return chairsToPacksMap.count( chairIndex ) > 0; }

        const TPackDescriptor& getPackDescriptor( int chairIndex ) const
        {
            return chairsToPacksMap.at(chairIndex).packDesc;
        }
        const std::vector<TCardDescriptor>& getPackCards( int chairIndex ) const
        {
            return chairsToPacksMap.at(chairIndex).cardDescs;
        }

        RoundConfiguration& setPack(
                int chairIndex,
                const TPackDescriptor& packDesc,
                const std::vector<TCardDescriptor>& cardDescs );

    private:

        struct PackContents
        {
            TPackDescriptor              packDesc;
            std::vector<TCardDescriptor> cardDescs;
        };

        const TRoundDescriptor      mRoundDescriptor;
        PassDirectionType           mPassDirection;
        int                         mTimeoutTicks;

        // This is a map of chair indices to the packs assigned to them.
        std::map<int,PackContents>  chairsToPacksMap;
    };

    //--------------------------------------------------------------------

    class Observer
    {
    public:
        virtual void notifyPackQueueSizeChanged( Draft& draft, int chairIndex, int packQueueSize ) = 0;
        virtual void notifyNewPack( Draft& draft, int chairIndex, const TPackDescriptor& pack, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
        virtual void notifyCardSelected( Draft& draft, int chairIndex, const TPackDescriptor& pack, const TCardDescriptor& card, bool autoSelected ) = 0;
        virtual void notifyCardSelectionError( Draft& draft, int chairIndex, const TCardDescriptor& card ) = 0;
        virtual void notifyTimeExpired( Draft& draft,int chairIndex, const TPackDescriptor& pack, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
        virtual void notifyNewRound( Draft& draft, int roundIndex, const TRoundDescriptor& round ) = 0;
        virtual void notifyDraftComplete( Draft& draft ) = 0;
    };

    //--------------------------------------------------------------------

private:
    class Chair;
    class Card;
    typedef std::shared_ptr<Card> CardSharedPtr;
    class Pack;
    typedef std::shared_ptr<Pack> PackSharedPtr;

    //--------------------------------------------------------------------

public:
    Draft( int chairCount, const std::vector<RoundConfiguration>& roundConfigurations, const Logging::Config &loggingConfig = Logging::Config() );
    ~Draft();

    void addObserver( Observer* observer ) { mObservers.push_back( observer ); }
    void removeObserver( Observer* observer );

    int getChairCount() const { return mChairs.size(); }
    int getRoundCount() const { return mRoundConfigs.size(); }

    void go();
    StateType getState() const { return mState; }

    // Gets current round descriptor.  Returns default-constructed value if draft is not running.
    TRoundDescriptor getCurrentRoundDescriptor() const;

    // Gets current round index.  Returns -1 if draft is not running.
    int getCurrentRound() const { return mCurrentRound; }

    bool makeCardSelection( int chairIndex, const TCardDescriptor& cardDescriptor );
    void tick();
    int getTicksRemaining( int chairIndex ) const;
    int getPackQueueSize( int chairIndex ) const;

    // Get all cards selected by a chair.
    std::vector<TCardDescriptor> getSelectedCards( int chairIndex ) const;

    // Get pack descriptor for top pack for a chair.
    TPackDescriptor getTopPackDescriptor( int chairIndex ) const;

    // Get unselected cards in top pack for a chair.
    std::vector<TCardDescriptor> getTopPackUnselectedCards( int chairIndex ) const;

private:

    // Generic helper class that incs/decs a counter reference and can
    // be inspected to check for scope nesting.
    class ScopeCounter
    {
        public:
            ScopeCounter( unsigned int& scopeCounter )
              : mScopeCounterRef( ++scopeCounter ) {}
            ~ScopeCounter() { --mScopeCounterRef; }
            bool inNestedScope() const { return mScopeCounterRef > 1; }
        private:
            unsigned int &mScopeCounterRef;
    };

    void processCardSelections();
    void processCardSelection( int chairIndex, const TCardDescriptor& cardDescriptor );
    bool isRoundComplete();
    void startNewRound();
    int getNextChairIndex( int thisChairIndex );

    // All of the configuration of rounds, packs, and cards lives here.  Pack
    // and Card references to descriptors are referenced to items within these
    // objects, so they must not change after initialization.
    const std::vector<RoundConfiguration> mRoundConfigs;

    StateType              mState;
    int                    mCurrentRound;
    std::vector<Chair*>    mChairs;
    std::vector<Observer*> mObservers;
    std::queue< std::pair<int,TCardDescriptor> > mCardSelectionQueue;
    unsigned int           mScopeCount;
    std::shared_ptr<spdlog::logger> mLogger;

    //--------------------------------------------------------------------

    class Chair
    {
    public:

        Chair( int index );

        int getIndex() const { return mIndex; }

        int getTicksRemaining() const { return mTicksRemaining; }
        void setTicksRemaining( int ticksRemaining ) { mTicksRemaining = ticksRemaining; }

        void enqueuePack( PackSharedPtr pack );
        int getPackQueueSize() { return mPackQueue.size(); }
        PackSharedPtr getTopPack();
        void popTopPack();

        void addSelectedCard( CardSharedPtr card );

        std::vector<CardSharedPtr> getSelectedCards() const { return mSelectedCards; }

    private:

        const int mIndex;
        std::vector<CardSharedPtr> mSelectedCards;
        std::queue<PackSharedPtr> mPackQueue;
        int mTicksRemaining;
    };

    //--------------------------------------------------------------------

    class Pack
    {
    public:

        Pack( const TPackDescriptor& packDescriptor ) : mPackDescriptor( packDescriptor ) {}

        int getCardCount() const { return mCards.size(); }
        int getSelectedCardCount() const;
        int getUnselectedCardCount() const;

        void addCard( CardSharedPtr spCard );
        std::vector<TCardDescriptor> getCardDescriptors() const;
        std::vector<CardSharedPtr> getUnselectedCards() const;
        std::vector<TCardDescriptor> getUnselectedCardDescriptors() const;
        CardSharedPtr getFirstUnselectedCard( const TCardDescriptor& cardDescriptor );
        const TPackDescriptor& getPackDescriptor() const { return mPackDescriptor; }

    private:

        // This is ultimately referenced back to a RoundConfiguration object.
        const TPackDescriptor& mPackDescriptor;

        std::vector<CardSharedPtr> mCards;
    };

    //--------------------------------------------------------------------

    class Card
    {
    public:

        Card( const TCardDescriptor& cardDescriptor ) : mCardDescriptor( cardDescriptor ), mSelectedChairPtr( 0 ) {}

        const TCardDescriptor& getCardDescriptor() const { return mCardDescriptor; }

        bool isSelected() const { return (mSelectedChairPtr != 0); }
        void setSelected( const Chair* const chairPtr, int round, int indexInRound );

    private:

        // This is ultimately referenced back to a RoundConfiguration object.
        const TCardDescriptor& mCardDescriptor;

        const Chair* mSelectedChairPtr;
        int mSelectedRound;
        int mSelectedIndexInRound;
    };

};

#include "Draft.tpp"

#endif
