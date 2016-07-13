#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

using proto::DraftConfig;

#define NUM_ROUNDS  3
#define NUM_PLAYERS 8

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
 

CATCH_TEST_CASE( "Round", "[draft][round]" )
{
    DraftConfig dc = TestDefaults::getDraftConfig( NUM_ROUNDS, NUM_PLAYERS, 30 );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers, getLoggingConfig() );

    RoundTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    CATCH_REQUIRE( obs.mNotifications == NUM_ROUNDS );
    CATCH_REQUIRE( obs.mSelectionErrors == 0 );
}
