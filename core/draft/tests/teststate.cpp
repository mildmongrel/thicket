#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

using proto::DraftConfig;

#define NUM_ROUNDS  3
#define NUM_PLAYERS 8

class StateTestDraftObserver : public TestDraftObserver
{
public:
    StateTestDraftObserver() {}

    virtual void notifyNewPack( Draft<>& draft, int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards )
    {
        CATCH_REQUIRE( draft.getTopPackId(chairIndex) == packId );
        CATCH_REQUIRE( draft.getTopPackUnselectedCards(chairIndex) == unselectedCards );

        draft.makeCardSelection( chairIndex, unselectedCards[0] );
    }

    virtual void notifyCardSelected( Draft<>& draft, int chairIndex, uint32_t packId, const std::string& card, bool autoSelected ) override
    {
        mSelectedCards[chairIndex].push_back( card );
        CATCH_REQUIRE( draft.getSelectedCards(chairIndex) == mSelectedCards[chairIndex] );
    }

    virtual void notifyNewRound( Draft<>& draft, int roundIndex ) override
    {
        CATCH_REQUIRE( draft.getCurrentRound() == roundIndex );
        CATCH_REQUIRE( draft.getState() == Draft<>::STATE_RUNNING );
    }

    std::vector<std::string> mSelectedCards[NUM_PLAYERS];
};
 

CATCH_TEST_CASE( "State: simple booster", "[draft][state]" )
{
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( NUM_ROUNDS, NUM_PLAYERS, 30 );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers );

    StateTestDraftObserver obs;
    d.addObserver( &obs );

    CATCH_REQUIRE( d.getCurrentRound() == -1 );
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_NEW );
    for( int chairIndex = 0; chairIndex < NUM_PLAYERS; ++chairIndex )
    {
        CATCH_REQUIRE( d.getSelectedCards(chairIndex).empty() );
    }

    d.start();

    CATCH_REQUIRE( d.getCurrentRound() == -1 );
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );

    for( int chairIndex = 0; chairIndex < NUM_PLAYERS; ++chairIndex )
    {
        CATCH_REQUIRE( d.getSelectedCards(chairIndex) == obs.mSelectedCards[chairIndex] );
    }
}


CATCH_TEST_CASE( "State: simple sealed", "[draft][state]" )
{
    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS );
    auto dispensers = TestDefaults::getDispensers( 6 );
    Draft<> d( dc, dispensers );

    StateTestDraftObserver obs;
    d.addObserver( &obs );

    CATCH_REQUIRE( d.getCurrentRound() == -1 );
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_NEW );
    for( int chairIndex = 0; chairIndex < NUM_PLAYERS; ++chairIndex )
    {
        CATCH_REQUIRE( d.getSelectedCards(chairIndex).empty() );
    }

    d.start();

    CATCH_REQUIRE( d.getCurrentRound() == -1 );
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );

    for( int chairIndex = 0; chairIndex < NUM_PLAYERS; ++chairIndex )
    {
        CATCH_REQUIRE( d.getSelectedCards(chairIndex) == obs.mSelectedCards[chairIndex] );
    }
}
