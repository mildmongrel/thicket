#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

#define NUM_ROUNDS  3
#define NUM_PLAYERS 8

class RoundTestDraftObserver : public TestDraftObserver
{
public:
    RoundTestDraftObserver() : mSelectionErrors( 0 ), mNotifications( 0 ) {}

    virtual void notifyNewPack( Draft<>& draft, int chairIndex, const std::string& pack, const std::vector<std::string>& unselectedCards )
    {
        draft.makeCardSelection( chairIndex, unselectedCards[0] );
    }
    virtual void notifyCardSelectionError( Draft<>& draft, int chairIndex, const std::string& card )
    {
        mSelectionErrors++;
    }
    virtual void notifyNewRound( Draft<>& draft, int roundIndex, const std::string& round )
    {
        mNotifications++;
    }

    int mSelectionErrors;
    int mNotifications;
};
 
CATCH_TEST_CASE( "Round", "[round]" )
{
    std::vector<Draft<>::RoundConfiguration> roundConfigs = TestDefaults::getRoundConfigurations( NUM_ROUNDS, NUM_PLAYERS, 15, 30 );
    Draft<> d( NUM_PLAYERS, roundConfigs );
    RoundTestDraftObserver obs;
    d.addObserver( &obs );

    d.go();

    CATCH_REQUIRE( obs.mNotifications == NUM_ROUNDS );
    CATCH_REQUIRE( obs.mSelectionErrors == 0 );
}
