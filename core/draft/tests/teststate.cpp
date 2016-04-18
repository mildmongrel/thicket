#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

#define NUM_ROUNDS  3
#define NUM_PLAYERS 8

class StateTestDraftObserver : public TestDraftObserver
{
public:
    StateTestDraftObserver( const std::vector<Draft<>::RoundConfiguration>& roundConfigs )
      : mRoundConfigs( roundConfigs )
    {}

    virtual void notifyNewPack( Draft<>& draft, int chairIndex, const std::string& pack, const std::vector<std::string>& unselectedCards )
    {
        CATCH_REQUIRE( draft.getTopPackDescriptor(chairIndex) == pack );
        CATCH_REQUIRE( draft.getTopPackUnselectedCards(chairIndex) == unselectedCards );

        draft.makeCardSelection( chairIndex, unselectedCards[0] );
    }

    virtual void notifyCardSelected( Draft<>& draft, int chairIndex, const std::string& pack, const std::string& card, bool autoSelected )
    {
        mSelectedCards[chairIndex].push_back( card );
        CATCH_REQUIRE( draft.getSelectedCards(chairIndex) == mSelectedCards[chairIndex] );
    }

    virtual void notifyNewRound( Draft<>& draft, int roundIndex, const std::string& round )
    {
        CATCH_REQUIRE( draft.getCurrentRound() == roundIndex );
        CATCH_REQUIRE( draft.getCurrentRoundDescriptor() == mRoundConfigs[roundIndex].getRoundDescriptor() );
        CATCH_REQUIRE( draft.getState() == Draft<>::STATE_RUNNING );
    }

    const std::vector<Draft<>::RoundConfiguration>& mRoundConfigs;
    std::vector<std::string> mSelectedCards[NUM_PLAYERS];
};
 
CATCH_TEST_CASE( "State", "[state]" )
{
    std::vector<Draft<>::RoundConfiguration> roundConfigs = TestDefaults::getRoundConfigurations( NUM_ROUNDS, NUM_PLAYERS, 15, 30 );
    Draft<> d( NUM_PLAYERS, roundConfigs );
    StateTestDraftObserver obs( roundConfigs );
    d.addObserver( &obs );

    CATCH_REQUIRE( d.getCurrentRound() == -1 );
    CATCH_REQUIRE( d.getCurrentRoundDescriptor() == std::string() );
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_NEW );
    for( int chairIndex = 0; chairIndex < NUM_PLAYERS; ++chairIndex )
    {
        CATCH_REQUIRE( d.getSelectedCards(chairIndex).empty() );
    }

    d.go();

    CATCH_REQUIRE( d.getCurrentRound() == -1 );
    CATCH_REQUIRE( d.getCurrentRoundDescriptor() == std::string() );
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );

    for( int chairIndex = 0; chairIndex < NUM_PLAYERS; ++chairIndex )
    {
        CATCH_REQUIRE( d.getSelectedCards(chairIndex) == obs.mSelectedCards[chairIndex] );
    }
}
