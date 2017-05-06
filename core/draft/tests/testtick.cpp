#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

using proto::DraftConfig;

#define NUM_PLAYERS 8

class TickTestDraftObserver : public TestDraftObserver
{
public:
    TickTestDraftObserver()
      : mChairNotifications( NUM_PLAYERS, 0 ),
        mPostRoundTimerNotifications( 0 ),
        mCompleteNotifications( 0 ) {}

    virtual void notifyTimeExpired( Draft<>& draft,int chairIndex, uint32_t packId ) override
    {
        mChairNotifications[chairIndex]++;
    }
    virtual void notifyPostRoundTimerStarted( Draft<>& draft, int roundIndex, int ticksRemaining ) override
    {
        mPostRoundTimerNotifications++;
        mPostRoundTimerRoundIndex = roundIndex;
        mPostRoundTimerTicks = ticksRemaining;
    }
    virtual void notifyDraftComplete( Draft<>& draft ) override
    {
        mCompleteNotifications++;
    }

    std::vector<int> mChairNotifications;
    int mPostRoundTimerNotifications;
    int mPostRoundTimerRoundIndex;
    int mPostRoundTimerTicks;
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


CATCH_TEST_CASE( "Tick: simple sealed with post-round timer", "[draft][tick]" )
{
    const int postRoundTicks = 30;
    int ticks = 0;

    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS, postRoundTicks );
    auto dispensers = TestDefaults::getDispensers( 6 );
    Draft<> d( dc, dispensers, getLoggingConfig() );

    TickTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    // The round should end immediately, but the draft is still in a
    // running state and the post-round timer indication should have
    // been sent.

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );
    CATCH_REQUIRE( d.getCurrentRound() == 0 );

    CATCH_REQUIRE( obs.mPostRoundTimerNotifications == 1 );
    CATCH_REQUIRE( obs.mPostRoundTimerRoundIndex == 0 );
    CATCH_REQUIRE( obs.mPostRoundTimerTicks == postRoundTicks );

    while( ticks < postRoundTicks )
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

 
CATCH_TEST_CASE( "Tick: simple grid", "[draft][tick]" )
{
    const int timeoutTicks = 30;
    int ticks = 0;

    DraftConfig dc = TestDefaults::getSimpleGridDraftConfig();
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

    // Active chair should be expired, none other.
    CATCH_REQUIRE( obs.mChairNotifications[0] == 1 );
    CATCH_REQUIRE( obs.mChairNotifications[1] == 0 );

    // Grid round can't end while selections aren't made.
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );
    CATCH_REQUIRE( obs.mCompleteNotifications == 0 );
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
    result = d.makeNamedCardSelection( 0, d.getTopPackId( 0 ), "0:card0" );
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
    result = d.makeNamedCardSelection( 1, d.getTopPackId( 1 ), "0:card1" );
    CATCH_REQUIRE( result );
    result = d.makeNamedCardSelection( 1, d.getTopPackId( 1 ), "0:card1" );
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

