#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

#define NUM_PLAYERS 8

class TickTestDraftObserver : public TestDraftObserver
{
public:
    TickTestDraftObserver() : mNotifications( NUM_PLAYERS, 0 ) {}

    virtual void notifyTimeExpired( Draft<>& draft,int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override
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
 
CATCH_TEST_CASE( "Tick", "[draft][tick]" )
{
    const int timeoutTicks = 30;
    int ticks = 0;

    DraftConfig dc = TestDefaults::getDraftConfig( 3, NUM_PLAYERS, timeoutTicks );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers, getLoggingConfig() );

    TickTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();
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


CATCH_TEST_CASE( "Tick - all packs move together", "[draft][tick]" )
{
    const int timeoutTicks = 30;
    bool result;

    DraftConfig dc = TestDefaults::getDraftConfig( 1, 2, timeoutTicks );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers, getLoggingConfig() );

    TickTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    // Have player 0 select a card.  Now both packs are queued on player 1 and
    // player 0 should not timeout.
    result = d.makeCardSelection( 0, ":card0" );
    CATCH_REQUIRE( result );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 0 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 2 );

    for( int t = 0; t < timeoutTicks; ++t )
    {
        d.tick();
    }

    CATCH_CHECK( obs.mNotifications[0] == 0 );
    CATCH_CHECK( obs.mNotifications[1] == 1 );

    // Have player 1 select a card from each pack.  Now both packs are queued
    // back on player 0 and player 1 should not timeout.
    result = d.makeCardSelection( 1, ":card1" );
    CATCH_REQUIRE( result );
    result = d.makeCardSelection( 1, ":card1" );
    CATCH_REQUIRE( result );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 2 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 0 );

    for( int t = 0; t < timeoutTicks; ++t )
    {
        d.tick();
    }

    CATCH_CHECK( obs.mNotifications[0] == 1 );
    CATCH_CHECK( obs.mNotifications[1] == 1 );
}

