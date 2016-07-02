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
        virtual void notifyPackQueueSizeChanged( Draft& draft, int chairIndex, int packQueueSize ) = 0;
        virtual void notifyNewPack( Draft& draft, int chairIndex, uint32_t packId, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
        virtual void notifyCardSelected( Draft& draft, int chairIndex, uint32_t packId, const TCardDescriptor& card, bool autoSelected ) = 0;
        virtual void notifyCardSelectionError( Draft& draft, int chairIndex, const TCardDescriptor& card ) = 0;
        virtual void notifyTimeExpired( Draft& draft,int chairIndex, uint32_t packId, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
        virtual void notifyNewRound( Draft& draft, int roundIndex ) = 0;
        virtual void notifyDraftComplete( Draft& draft ) = 0;
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
    Draft( const DraftConfig&      draftConfig,
           const DraftCardDispenserSharedPtrVector<TCardDescriptor>&
                                   cardDispensers,
           const Logging::Config&  loggingConfig = Logging::Config() );
    ~Draft();

    void addObserver( Observer* observer ) { mObservers.push_back( observer ); }
    void removeObserver( Observer* observer );

    // Start the draft.
    void start();

    // These methods are safe to call from callback (Observer) contexts.
    bool makeCardSelection( int chairIndex, const TCardDescriptor& cardDescriptor );
    void tick();

    int getChairCount() const { return mChairs.size(); }
    int getRoundCount() const { return mDraftConfig.rounds_size(); }

    StateType getState() const { return mState; }

    // Gets current round index.  Returns -1 if draft is not running.
    int getCurrentRound() const { return mCurrentRound; }

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
        MESSAGE_CARD_SELECTION
    };

    struct Message
    {
        virtual ~Message() {};
        MessageType messageType;
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

    struct CardSelectionMessage : public Message
    {
        CardSelectionMessage( int chairIdx, const TCardDescriptor& cardDesc )
          : Message( MESSAGE_CARD_SELECTION ),
            chairIndex( chairIdx ),
            cardDescriptor( cardDesc )
        {}

        int             chairIndex;
        TCardDescriptor cardDescriptor;
    };

    typedef std::shared_ptr<Message> MessageSharedPtr;


    void processMessageQueue();

    void processStart();
    void processCardSelection( int chairIndex, const TCardDescriptor& cardDescriptor );
    void processTick();

    void startNewRound();
    bool isRoundComplete();
    int getNextChairIndex( int thisChairIndex );

    void enterDraftErrorState();

    const DraftConfig mDraftConfig;
    const DraftConfigAdapter mDraftConfigAdapter;
    const DraftCardDispenserSharedPtrVector<TCardDescriptor> mCardDispensers;

    StateType                     mState;
    int                           mCurrentRound;
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

        Pack( uint32_t packId ) : mPackId( packId ) {}

        int getCardCount() const { return mCards.size(); }
        int getSelectedCardCount() const;
        int getUnselectedCardCount() const;

        void addCard( CardSharedPtr spCard );
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
          : mCardDescriptor( cardDescriptor ), mSelectedChairPtr( 0 ) {}

        const TCardDescriptor& getCardDescriptor() const { return mCardDescriptor; }

        bool isSelected() const { return (mSelectedChairPtr != 0); }
        void setSelected( const Chair* const chairPtr, int round, int indexInRound );

    private:

        const TCardDescriptor mCardDescriptor;
        const Chair* mSelectedChairPtr;
        int mSelectedRound;
        int mSelectedIndexInRound;
    };

};

#include "Draft.tpp"

#endif
