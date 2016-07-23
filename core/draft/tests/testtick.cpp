#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

using proto::DraftConfig;

#define NUM_PLAYERS 8

class TickTestDraftObserver : public TestDraftObserver
{
public:
    TickTestDraftObserver() : mChairNotifications( NUM_PLAYERS, 0 ), mCompleteNotifications( 0 ) {}

    virtual void notifyTimeExpired( Draft<>& draft,int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override
    {
        mChairNotifications[chairIndex]++;
    }
    virtual void notifyDraftComplete( Draft<>& draft ) override
    {
        mCompleteNotifications++;
    }

    std::vector<int> mChairNotifications;
    int mCompleteNotifications;
};

static Logging::Config getLoggingConfig()
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "testtick" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );
    return loggingConfig;
}
 
CATCH_TEST_CASE( "Tick: simple booster", "[draft][tick]" )
{
    const int timeoutTicks = 30;
    int ticks = 0;

    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, NUM_PLAYERS, timeoutTicks );
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
        CATCH_REQUIRE( obs.mChairNotifications[i] == 1 );
    }

    // Booster round can't end while selections aren't made.
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );
    CATCH_REQUIRE( obs.mCompleteNotifications == 0 );
}


CATCH_TEST_CASE( "Tick: simple sealed", "[draft][tick]" )
{
    const int timeoutTicks = 30;
    int ticks = 0;

    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS, timeoutTicks );
    auto dispensers = TestDefaults::getDispensers( 6 );
    Draft<> d( dc, dispensers, getLoggingConfig() );

    TickTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    // Make sure round still running.
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );
    CATCH_REQUIRE( d.getCurrentRound() == 0 );

    while( ticks < timeoutTicks )
    {
        d.tick();
        ticks++;
    }

    // Sealed round should end when round timer expires.
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );
    CATCH_REQUIRE( obs.mCompleteNotifications == 1 );

    // Sealed should have no chair notifications.
    for( int i = 0; i < NUM_PLAYERS; ++i )
    {
        CATCH_REQUIRE( obs.mChairNotifications[i] == 0 );
    }
}


CATCH_TEST_CASE( "Tick - all packs move together", "[draft][tick]" )
{
    const int timeoutTicks = 30;
    bool result;

    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 1, 2, timeoutTicks );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers, getLoggingConfig() );

    TickTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    // Have player 0 select a card.  Now both packs are queued on player 1 and
    // player 0 should not timeout.
    result = d.makeCardSelection( 0, "0:card0" );
    CATCH_REQUIRE( result );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 0 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 2 );

    for( int t = 0; t < timeoutTicks; ++t )
    {
        d.tick();
    }

    CATCH_CHECK( obs.mChairNotifications[0] == 0 );
    CATCH_CHECK( obs.mChairNotifications[1] == 1 );

    // Have player 1 select a card from each pack.  Now both packs are queued
    // back on player 0 and player 1 should not timeout.
    result = d.makeCardSelection( 1, "0:card1" );
    CATCH_REQUIRE( result );
    result = d.makeCardSelection( 1, "0:card1" );
    CATCH_REQUIRE( result );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 2 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 0 );

    for( int t = 0; t < timeoutTicks; ++t )
    {
        d.tick();
    }

    CATCH_CHECK( obs.mChairNotifications[0] == 1 );
    CATCH_CHECK( obs.mChairNotifications[1] == 1 );
}

