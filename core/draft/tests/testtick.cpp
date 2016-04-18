#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

#define NUM_PLAYERS 8

class TickTestDraftObserver : public TestDraftObserver
{
public:
    TickTestDraftObserver() : mNotifications( NUM_PLAYERS, 0 ) {}

    virtual void notifyTimeExpired( Draft<>& draft,int chairIndex, const std::string& pack, const std::vector<std::string>& unselectedCards )
    {
        mNotifications[chairIndex]++;
    }

    std::vector<int> mNotifications;
};

static Logging::Config getLoggingConfig()
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "testtick" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );
    return loggingConfig;
}
 
CATCH_TEST_CASE( "Tick", "[tick]" )
{
    const int timeoutTicks = 30;
    int ticks = 0;

    Draft<> d( NUM_PLAYERS, TestDefaults::getRoundConfigurations( 3, NUM_PLAYERS, 15, timeoutTicks ), getLoggingConfig() );
    TickTestDraftObserver obs;
    d.addObserver( &obs );

    d.go();
    while( ticks < timeoutTicks )
    {
        d.tick();
        ticks++;
    }

    for( int i = 0; i < NUM_PLAYERS; ++i )
    {
        CATCH_CHECK( obs.mNotifications[i] == 1 );
    }
}

CATCH_TEST_CASE( "Tick - some players don't have packs", "[tick]" )
{
    const int timeoutTicks = 30;
    int ticks = 0;

    Draft<> d( NUM_PLAYERS, TestDefaults::getRoundConfigurations( 1, 2, 15, timeoutTicks ), getLoggingConfig() );
    TickTestDraftObserver obs;
    d.addObserver( &obs );

    d.go();

    // Have player 0 select a card.  Now packs are queued on player 1 and player 0 should not timeout.
    bool result = d.makeCardSelection( 0, "card0" );
    CATCH_REQUIRE( result );

    while( ticks < timeoutTicks )
    {
        d.tick();
        ticks++;
    }

    CATCH_CHECK( obs.mNotifications[0] == 0 );
    CATCH_CHECK( obs.mNotifications[1] == 1 );
}
