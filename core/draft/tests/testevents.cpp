#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"
#include <vector>

using proto::DraftConfig;

enum Event {
    EVENT_SELECTION,
    EVENT_NEW_ROUND,
    EVENT_COMPLETE
};

class EventsTestDraftObserver : public TestDraftObserver
{
public:

    EventsTestDraftObserver() : mSelectionErrors(0) {}

    virtual void notifyCardSelected( Draft<>& draft, int chairIndex, uint32_t packId, const std::string& card, bool autoSelected ) override
    {
        mEvents.push_back( EVENT_SELECTION );
        if( !autoSelected )
        {
            mGrantedCardnamesByChair[chairIndex].push_back( card );
        }
        else
        {
            mAutoselectedCardnamesByChair[chairIndex].push_back( card );
        }
    }
    virtual void notifyCardSelectionError( Draft<>& draft, int chairIndex, const std::string& card ) override
    {
        mSelectionErrors++;
    }
    virtual void notifyNewPack( Draft<>& draft, int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override
    {
        std::string cardToSelect = unselectedCards[0];
        mRequestedCardnamesByChair[chairIndex].push_back( cardToSelect );
        bool result = draft.makeCardSelection( chairIndex, cardToSelect );
        if( !result )
            mSelectionErrors++;
    }
    virtual void notifyNewRound( Draft<>& draft, int roundIndex ) override
    {
        mEvents.push_back( EVENT_NEW_ROUND );
    }
    virtual void notifyDraftComplete( Draft<>& draft ) override
    {
        mEvents.push_back( EVENT_COMPLETE );
    }

    std::vector<Event> mEvents;
    int mSelectionErrors;
    std::map<int, std::vector<std::string> > mRequestedCardnamesByChair;
    std::map<int, std::vector<std::string> > mGrantedCardnamesByChair;
    std::map<int, std::vector<std::string> > mAutoselectedCardnamesByChair;
};
 

CATCH_TEST_CASE( "Events: simple booster", "[draft][events]" )
{
    const int NUM_ROUNDS  = 3;
    const int NUM_PLAYERS = 8;
    const int NUM_CARD_SELECTIONS_PER_ROUND = 15 * NUM_PLAYERS;

    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( NUM_ROUNDS, NUM_PLAYERS, 30 );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers );

    EventsTestDraftObserver obs;
    d.addObserver( &obs );

    // This will auto-run the draft to conclusion with the observer in place.
    d.start();

    // Number of events: (15 card selections * NUM_PLAYERS * rounds) + rounds + 1 end
    CATCH_REQUIRE( obs.mEvents.size() == (NUM_CARD_SELECTIONS_PER_ROUND * NUM_ROUNDS) + NUM_ROUNDS + 1  );

    // Copy the events, then verify type and order by testing and popping items.
    std::vector<Event> eventsCopy( obs.mEvents );
    std::vector<Event>::iterator rangeStart;
    std::vector<Event>::iterator rangeEnd;

    for( size_t i = 0; i < NUM_ROUNDS; ++i )
    {
        CATCH_REQUIRE( eventsCopy[0] == EVENT_NEW_ROUND );
        eventsCopy.erase( eventsCopy.begin() );

        rangeStart = eventsCopy.begin();
        rangeEnd = rangeStart + NUM_CARD_SELECTIONS_PER_ROUND;
        CATCH_REQUIRE( std::count( rangeStart, rangeEnd, EVENT_SELECTION ) == NUM_CARD_SELECTIONS_PER_ROUND );

        eventsCopy.erase( rangeStart, rangeEnd );
    }
    CATCH_REQUIRE( eventsCopy[0] == EVENT_COMPLETE );


    // Ensure there weren't any selection errors.
    CATCH_REQUIRE( obs.mSelectionErrors == 0 );

    // Compare the requested cards for all players to the granted cards for all
    // players.  This comparison ensures order of selection is maintained as well.
    // Simple check, huge test.
    CATCH_REQUIRE( obs.mRequestedCardnamesByChair == obs.mGrantedCardnamesByChair );

    // Verify autoselected cards map.
    CATCH_REQUIRE( obs.mAutoselectedCardnamesByChair.size() == NUM_PLAYERS );
    for( size_t i = 0; i < NUM_PLAYERS; ++i )
    {
        CATCH_REQUIRE( obs.mAutoselectedCardnamesByChair[i].size() == NUM_ROUNDS );
        CATCH_REQUIRE( std::count( obs.mAutoselectedCardnamesByChair[i].begin(),
                       obs.mAutoselectedCardnamesByChair[i].end(),
                       "0:card14" ) == NUM_ROUNDS );
    }
}

CATCH_TEST_CASE( "Events: simple sealed", "[draft][events]" )
{
    const int NUM_PLAYERS    = 8;
    const int NUM_DISPENSERS = 6;
    const int NUM_CARD_SELECTIONS_PER_ROUND = 15 * NUM_PLAYERS * NUM_DISPENSERS;

    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS );
    auto dispensers = TestDefaults::getDispensers( NUM_DISPENSERS );
    Draft<> d( dc, dispensers );

    EventsTestDraftObserver obs;
    d.addObserver( &obs );

    // This will auto-run the draft to conclusion with the observer in place.
    d.start();

    // Number of events: (15 card selections * NUM_PLAYERS * NUM_DISPENSERS) + 1 round + 1 end
    CATCH_REQUIRE( obs.mEvents.size() == NUM_CARD_SELECTIONS_PER_ROUND + 1 + 1  );

    // Copy the events, then verify type and order by testing and popping items.
    std::vector<Event> eventsCopy( obs.mEvents );
    std::vector<Event>::iterator rangeStart;
    std::vector<Event>::iterator rangeEnd;

    CATCH_REQUIRE( eventsCopy[0] == EVENT_NEW_ROUND );
    eventsCopy.erase( eventsCopy.begin() );

    rangeStart = eventsCopy.begin();
    rangeEnd = rangeStart + NUM_CARD_SELECTIONS_PER_ROUND;
    CATCH_REQUIRE( std::count( rangeStart, rangeEnd, EVENT_SELECTION ) == NUM_CARD_SELECTIONS_PER_ROUND );
    eventsCopy.erase( rangeStart, rangeEnd );

    CATCH_REQUIRE( eventsCopy[0] == EVENT_COMPLETE );

    // Ensure there weren't any selection errors.
    CATCH_REQUIRE( obs.mSelectionErrors == 0 );

    // For sealed, there are no requested or granted cards, everything is autoselected.
    CATCH_REQUIRE( obs.mRequestedCardnamesByChair.empty() );
    CATCH_REQUIRE( obs.mGrantedCardnamesByChair.empty() );

    // Verify autoselected cards map.
    CATCH_REQUIRE( obs.mAutoselectedCardnamesByChair.size() == NUM_PLAYERS );

    for( size_t i = 0; i < NUM_PLAYERS; ++i )
    {
        // Check that right quantity of cards are there.
        CATCH_REQUIRE( obs.mAutoselectedCardnamesByChair[i].size() == 15 * NUM_DISPENSERS );

        // Check some edge-case cards.
        CATCH_REQUIRE( std::count( obs.mAutoselectedCardnamesByChair[i].begin(),
                       obs.mAutoselectedCardnamesByChair[i].end(), "0:card0" ) == 1 );
        CATCH_REQUIRE( std::count( obs.mAutoselectedCardnamesByChair[i].begin(),
                       obs.mAutoselectedCardnamesByChair[i].end(), "0:card14" ) == 1 );
        CATCH_REQUIRE( std::count( obs.mAutoselectedCardnamesByChair[i].begin(),
                       obs.mAutoselectedCardnamesByChair[i].end(), "5:card0" ) == 1 );
        CATCH_REQUIRE( std::count( obs.mAutoselectedCardnamesByChair[i].begin(),
                       obs.mAutoselectedCardnamesByChair[i].end(), "5:card14" ) == 1 );
    }
}
