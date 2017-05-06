#ifndef DRAFT_H
#define DRAFT_H

#include <memory>
#include <queue>
#include <vector>

#include "Logging.h"
#include "DraftCardDispenser.h"
#include "DraftConfigAdapter.h"
#include "DraftConfig.pb.h"


template< typename TCardDescriptor = std::string >
class Draft
{
public:

    enum StateType
    {
        STATE_NEW,
        STATE_RUNNING,
        STATE_COMPLETE,
        STATE_ERROR
    };

    //--------------------------------------------------------------------

    class Observer
    {
    public:

        class PublicCardState
        {
        public:
            PublicCardState( const TCardDescriptor& cardArg, int selectedChairIndexArg, int selectedOrderArg )
              : mCard( cardArg ), mSelectedChairIndex( selectedChairIndexArg ), mSelectedOrder( selectedOrderArg ) {};

            // Card data.
            const TCardDescriptor& getCard() const { return mCard; }

            // Chair that selected the card; -1 if not selected
            int getSelectedChairIndex() const { return mSelectedChairIndex; }

            // Order of selection within round starting with zero; -1 if not selected
            int getSelectedOrder() const { return mSelectedOrder; }

        private:
            TCardDescriptor mCard;
            int             mSelectedChairIndex;
            int             mSelectedOrder;
        };

        // Called when size of a chair's personal pack queue is changed.
        virtual void notifyPackQueueSizeChanged( Draft& draft, int chairIndex, int packQueueSize ) = 0;

        // Called when a new personal pack is available for a chair.
        virtual void notifyNewPack( Draft& draft, int chairIndex, uint32_t packId, const std::vector<TCardDescriptor>& unselectedCards ) = 0;

        // Called anytime public state changes.  'activeChairIndex' is -1 if no chair active (i.e. end of round)
        virtual void notifyPublicState( Draft& draft,
                                        uint32_t packId,
                                        const std::vector<PublicCardState>& cardStates,
                                        int activeChairIndex ) = 0;

        // Called in response to a named card selection.  'result' indicates success/failure.
        virtual void notifyNamedCardSelectionResult( Draft&                 draft,
                                                     int                    chairIndex,
                                                     uint32_t               packId,
                                                     bool                   result,
                                                     const TCardDescriptor& card ) = 0;

        // Called in response to an indexed card selection.  'result' indicates success/failure.  'cards' is only populated on success.
        virtual void notifyIndexedCardSelectionResult( Draft&                              draft,
                                                       int                                 chairIndex,
                                                       uint32_t                            packId,
                                                       bool                                result,
                                                       const std::vector<int>&             selectionIndices,
                                                       const std::vector<TCardDescriptor>& cards ) = 0;

        // Called when a card is auto-selected to a chair.
        virtual void notifyCardAutoselection( Draft&                 draft,
                                              int                    chairIndex,
                                              uint32_t               packId,
                                              const TCardDescriptor& card ) = 0;

        // Called when time expires.  (Both personal and public packs).  Does not imply anything has been selected.
        virtual void notifyTimeExpired( Draft& draft,int chairIndex, uint32_t packId ) = 0;

        // Called when a post-round timer kicks in.
        virtual void notifyPostRoundTimerStarted( Draft& draft, int roundIndex, int ticksRemaining ) = 0;

        // Called when a new round begins.
        virtual void notifyNewRound( Draft& draft, int roundIndex ) = 0;

        // Called when the draft is complete.
        virtual void notifyDraftComplete( Draft& draft ) = 0;

        // Called when the draft enters an unrecoverable error state.
        virtual void notifyDraftError( Draft& draft ) = 0;
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
    Draft( const proto::DraftConfig&  draftConfig,
           const DraftCardDispenserSharedPtrVector<TCardDescriptor>&
                                      cardDispensers,
           const Logging::Config&     loggingConfig = Logging::Config() );
    ~Draft();

    void addObserver( Observer* observer ) { mObservers.push_back( observer ); }
    void removeObserver( Observer* observer );

    // Start the draft.
    void start();

    // These methods are safe to call from callback (Observer) contexts.
    bool makeNamedCardSelection( int chairIndex, uint32_t packId, const TCardDescriptor& cardDescriptor );
    bool makeIndexedCardSelection( int chairIndex, uint32_t packId, const std::vector<int>& selectionIndices );
    void tick();

    int getChairCount() const { return mChairs.size(); }
    int getRoundCount() const { return mDraftConfig.rounds_size(); }

    StateType getState() const { return mState; }

    // Gets current round index.  Returns -1 if draft is not running.
    int getCurrentRound() const { return mCurrentRound; }

    // Checks for type of current round.
    bool isBoosterRound() const;
    bool isSealedRound() const;
    bool isGridRound() const;

    int getTicksRemaining( int chairIndex ) const;
    int getPackQueueSize( int chairIndex ) const;

    // Get all cards selected by a chair.
    std::vector<TCardDescriptor> getSelectedCards( int chairIndex ) const;

    // Get pack descriptor for top pack for a chair.
    uint32_t getTopPackId( int chairIndex ) const;

    // Get unselected cards in top pack for a chair.
    std::vector<TCardDescriptor> getTopPackUnselectedCards( int chairIndex ) const;

private:

    enum MessageType
    {
        MESSAGE_START,
        MESSAGE_TICK,
        MESSAGE_NAMED_CARD_SELECTION,
        MESSAGE_INDEXED_CARD_SELECTION
    };

    struct Message
    {
        virtual ~Message() {};
        const MessageType messageType;
    protected:
        // Cannot construct Message, only inherit.
        Message( MessageType m ) : messageType( m ) {}
    };

    struct StartMessage : public Message
    {
        StartMessage() : Message( MESSAGE_START ) {}
    };

    struct TickMessage : public Message
    {
        TickMessage() : Message( MESSAGE_TICK ) {}
    };

    struct NamedCardSelectionMessage : public Message
    {
        NamedCardSelectionMessage( int chairIndexArg, uint32_t packIdArg, const TCardDescriptor& cardDescArg )
          : Message( MESSAGE_NAMED_CARD_SELECTION ),
            chairIndex( chairIndexArg ),
            packId( packIdArg ),
            cardDescriptor( cardDescArg )
        {}

        const int             chairIndex;
        const uint32_t        packId;
        const TCardDescriptor cardDescriptor;
    };

    struct IndexedCardSelectionMessage : public Message
    {
        IndexedCardSelectionMessage( int chairIndexArg, uint32_t packIdArg, const std::vector<int>& selectionIndicesArg )
          : Message( MESSAGE_INDEXED_CARD_SELECTION ),
            chairIndex( chairIndexArg ),
            packId( packIdArg ),
            selectionIndices( selectionIndicesArg )
        {}

        const int              chairIndex;
        const uint32_t         packId;
        const std::vector<int> selectionIndices;
    };

    typedef std::shared_ptr<Message> MessageSharedPtr;
    typedef google::protobuf::RepeatedPtrField<proto::DraftConfig::CardDispensation> CardDispensationRepeatedPtrField;


    void processMessageQueue();

    void processStart();
    void processNamedCardSelection( int chairIndex, uint32_t packId, const TCardDescriptor& cardDescriptor );
    void processIndexedCardSelection( int chairIndex, uint32_t packId, const std::vector<int>& selectionIndices );
    void processTick();

    void startNewRound();
    PackSharedPtr createPackFromDispensations( int chairIndex, const CardDispensationRepeatedPtrField& dispensations );
    PackSharedPtr createGridPackFromDispenser( uint32_t cardDispenserIndex );
    bool isSelectionComplete();
    bool checkRoundTransition();

    int getNextChairIndex( int thisChairIndex );

    void enterDraftErrorState();

    const proto::DraftConfig mDraftConfig;
    const DraftConfigAdapter mDraftConfigAdapter;
    const DraftCardDispenserSharedPtrVector<TCardDescriptor> mCardDispensers;

    StateType                     mState;
    int                           mCurrentRound;
    bool                          mPostRoundTimerStarted;
    uint32_t                      mPostRoundTimerTicksRemaining;
    PackSharedPtr                 mPublicPack;
    Chair*                        mPublicActiveChair;
    std::vector<Chair*>           mChairs;
    std::vector<Observer*>        mObservers;
    std::queue<MessageSharedPtr>  mMessageQueue;
    uint32_t                      mNextPackId;
    bool                          mProcessingMessageQueue;

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
        std::size_t getPackQueueSize() { return mPackQueue.size(); }
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

        Pack( uint32_t packId ) : mPackId( packId ) {}

        std::size_t getCardCount() const { return mCards.size(); }
        std::size_t getSelectedCardCount() const;
        std::size_t getUnselectedCardCount() const;

        void addCard( CardSharedPtr spCard );
        CardSharedPtr getCard( std::size_t index ) const;
        std::vector<TCardDescriptor> getCardDescriptors() const;
        std::vector<CardSharedPtr> getUnselectedCards() const;
        std::vector<TCardDescriptor> getUnselectedCardDescriptors() const;
        CardSharedPtr getFirstUnselectedCard( const TCardDescriptor& cardDescriptor );
        uint32_t getPackId() const { return mPackId; }

    private:

        uint32_t mPackId;
        std::vector<CardSharedPtr> mCards;
    };

    //--------------------------------------------------------------------

    class Card
    {
    public:

        Card( const TCardDescriptor& cardDescriptor )
          : mCardDescriptor( cardDescriptor ),
            mSelectedChairPtr( 0 ),
            mSelectedRound( -1 ),
            mSelectedIndexInRound( -1 ) {}

        const TCardDescriptor& getCardDescriptor() const { return mCardDescriptor; }

        bool isSelected() const { return (mSelectedChairPtr != 0); }
        void setSelected( const Chair* const chairPtr, int round, int indexInRound );

        const Chair* getSelectedChair() const { return mSelectedChairPtr; }
        int getSelectedIndexInRound() const { return mSelectedIndexInRound; }

    private:

        const TCardDescriptor mCardDescriptor;
        const Chair* mSelectedChairPtr;
        int mSelectedRound;
        int mSelectedIndexInRound;
    };

};

#include "Draft.tpp"

#endif
