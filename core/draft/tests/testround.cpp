#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

using proto::DraftConfig;

static Logging::Config getLoggingConfig()
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "testround" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::trace );
    return loggingConfig;
}
 
class RoundTestDraftObserver : public TestDraftObserver
{
public:
    RoundTestDraftObserver() : mSelectionErrors( 0 ), mNotifications( 0 ) {}

    virtual void notifyNewPack( Draft<>& draft, int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override
    {
        draft.makeCardSelection( chairIndex, unselectedCards[0] );
    }
    virtual void notifyCardSelectionError( Draft<>& draft, int chairIndex, const std::string& card ) override
    {
        mSelectionErrors++;
    }
    virtual void notifyNewRound( Draft<>& draft, int roundIndex ) override
    {
        mNotifications++;
    }

    int mSelectionErrors;
    int mNotifications;
};
 

CATCH_TEST_CASE( "Rounds: simple booster", "[draft][round]" )
{
    const int NUM_ROUNDS  = 3;
    const int NUM_PLAYERS = 8;

    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( NUM_ROUNDS, NUM_PLAYERS, 30 );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers, getLoggingConfig() );

    RoundTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );
    CATCH_REQUIRE( obs.mNotifications == NUM_ROUNDS );
    CATCH_REQUIRE( obs.mSelectionErrors == 0 );
}

CATCH_TEST_CASE( "Rounds: simple sealed", "[draft][round]" )
{
    const int NUM_PLAYERS = 8;

    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS );
    auto dispensers = TestDefaults::getDispensers( 6 );
    Draft<> d( dc, dispensers, getLoggingConfig() );

    RoundTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );
    CATCH_REQUIRE( obs.mNotifications == 1 );
    CATCH_REQUIRE( obs.mSelectionErrors == 0 );
}
