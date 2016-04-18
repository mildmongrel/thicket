#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"
#include <vector>

#define NUM_ROUNDS  3
#define NUM_PLAYERS 8

enum Event {
    EVENT_SELECTION,
    EVENT_NEW_ROUND,
    EVENT_COMPLETE
};

class EventsTestDraftObserver : public TestDraftObserver
{
public:

    EventsTestDraftObserver() : mSelectionErrors(0) {}

    virtual void notifyCardSelected( Draft<>& draft, int chairIndex, const std::string& pack, const std::string& card, bool autoSelected )
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
    virtual void notifyCardSelectionError( Draft<>& draft, int chairIndex, const std::string& card )
    {
        mSelectionErrors++;
    }
    virtual void notifyNewPack( Draft<>& draft, int chairIndex, const std::string& pack, const std::vector<std::string>& unselectedCards )
    {
        std::string cardToSelect = unselectedCards[0];
        mRequestedCardnamesByChair[chairIndex].push_back( cardToSelect );
        bool result = draft.makeCardSelection( chairIndex, cardToSelect );
        if( !result )
            mSelectionErrors++;
    }
    virtual void notifyNewRound( Draft<>& draft, int roundIndex, const std::string& round )
    {
        mEvents.push_back( EVENT_NEW_ROUND );
    }
    virtual void notifyDraftComplete( Draft<>& draft )
    {
        mEvents.push_back( EVENT_COMPLETE );
    }

    std::vector<Event> mEvents;
    int mSelectionErrors;
    std::map<int, std::vector<std::string> > mRequestedCardnamesByChair;
    std::map<int, std::vector<std::string> > mGrantedCardnamesByChair;
    std::map<int, std::vector<std::string> > mAutoselectedCardnamesByChair;
};
 
CATCH_TEST_CASE( "Events", "[events]" )
{
    std::vector<Draft<>::RoundConfiguration> roundConfigs = TestDefaults::getRoundConfigurations( NUM_ROUNDS, NUM_PLAYERS, 15, 30 );
    Draft<> d( NUM_PLAYERS, roundConfigs );
    EventsTestDraftObserver obs;
    d.addObserver( &obs );

    // This will auto-run the draft to conclusion with the observer in place.
    d.go();

    // Number of events: (15 card selections * NUM_PLAYERS * rounds) + rounds + 1 end
    CATCH_REQUIRE( obs.mEvents.size() == ((15*NUM_PLAYERS) * NUM_ROUNDS) + NUM_ROUNDS + 1  );

    // Copy the events, then verify type and order by testing and popping items.
    const int numCardSelectionsPerRound = 15 * NUM_PLAYERS;
    std::vector<Event> eventsCopy( obs.mEvents );
    std::vector<Event>::iterator rangeStart;
    std::vector<Event>::iterator rangeEnd;

    for( size_t i = 0; i < NUM_ROUNDS; ++i )
    {
        CATCH_REQUIRE( eventsCopy[0] == EVENT_NEW_ROUND );
        eventsCopy.erase( eventsCopy.begin() );

        rangeStart = eventsCopy.begin();
        rangeEnd = rangeStart + numCardSelectionsPerRound;
        CATCH_REQUIRE( std::count( rangeStart, rangeEnd, EVENT_SELECTION ) == numCardSelectionsPerRound );

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
                       "card14" ) == NUM_ROUNDS );
    }
}
