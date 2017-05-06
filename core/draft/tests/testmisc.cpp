#include "catch.hpp"
#include "Draft.h"
#include "testdefaults.h"
#include "TestDraftObserver.h"

using proto::DraftConfig;

CATCH_TEST_CASE( "Simple booster draft created with not enough dispensers", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );
    DraftCardDispenserSharedPtrVector<> dispensers; // no dispensers
    Draft<> d( dc, dispensers );

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_ERROR );

    // Make sure the state is still in error after trying to start.
    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_ERROR );
}


CATCH_TEST_CASE( "Simple sealed draft created with not enough dispensers", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( 8 );
    DraftCardDispenserSharedPtrVector<> dispensers; // no dispensers
    Draft<> d( dc, dispensers );

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_ERROR );

    // Make sure the state is still in error after trying to start.
    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_ERROR );
}


CATCH_TEST_CASE( "Draft not restartable", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers );

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_NEW );

    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );

    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );
}


CATCH_TEST_CASE( "Invalid named selection pack id", "[draft][misc]" )
{
    class LocalTestDraftObserver : public TestDraftObserver
    {
    public:
        LocalTestDraftObserver() : mErrors( 0 ) {}
        virtual void notifyNamedCardSelectionResult( Draft<>& draft, int chairIndex,
                uint32_t packId, bool result, const std::string& card ) override
        {
            if( !result ) ++mErrors;
        }
        unsigned int mErrors;
    };

    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );
    auto dispensers = TestDefaults::getDispensers();

    Draft<> d( dc, dispensers );
    LocalTestDraftObserver obs;
    d.addObserver( &obs );
    d.start();

    d.makeNamedCardSelection( 0, d.getTopPackId( 1 ), "0:card0" );

    CATCH_REQUIRE( obs.mErrors == 1 );
}


CATCH_TEST_CASE( "Booster pass direction clockwise", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );
    auto dispensers = TestDefaults::getDispensers();

    dc.mutable_rounds(0)->mutable_booster_round()->set_pass_direction(
            DraftConfig::DIRECTION_CLOCKWISE );
    Draft<> d( dc, dispensers );

    d.start();

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 1 );

    d.makeNamedCardSelection( 0, d.getTopPackId( 0 ), "0:card0" );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 0 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 2 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 1 );
}


CATCH_TEST_CASE( "Booster pass direction counter-clockwise", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );
    auto dispensers = TestDefaults::getDispensers();

    dc.mutable_rounds(0)->mutable_booster_round()->set_pass_direction(
            DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
    Draft<> d( dc, dispensers );

    d.start();

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 1 );

    d.makeNamedCardSelection( 0, d.getTopPackId( 0 ), "0:card0" );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 0 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 2 );
}


CATCH_TEST_CASE( "Unusual dispensations", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );

    // re-do the dispensations in the config:
    //   dispensation 0: dispenser 0, chair 0,2,4,6
    //   dispensation 1: dispenser 0, chair 0,1,2,3
    //   dispensation 2: dispenser 1, chair 0,1
    DraftConfig::BoosterRound* boosterRound = dc.mutable_rounds(0)->mutable_booster_round();
    boosterRound->clear_dispensations();
    DraftConfig::CardDispensation* dispensation0 = boosterRound->add_dispensations();
    dispensation0->set_dispenser_index( 0 );
    dispensation0->set_dispense_all( true );
    for( int i : { 0, 2, 4, 6 } )
    {
        dispensation0->add_chair_indices( i );
    }
    DraftConfig::CardDispensation* dispensation1 = boosterRound->add_dispensations();
    dispensation1->set_dispenser_index( 0 );
    dispensation1->set_dispense_all( true );
    for( int i : { 0, 1, 2, 3 } )
    {
        dispensation1->add_chair_indices( i );
    }
    DraftConfig::CardDispensation* dispensation2 = boosterRound->add_dispensations();
    dispensation2->set_dispenser_index( 1 );
    dispensation2->set_dispense_all( true );
    for( int i : { 0, 1 } )
    {
        dispensation2->add_chair_indices( i );
    }
    CATCH_REQUIRE( boosterRound->dispensations_size() == 3 );

    // put 2 dispensers into the config
    dc.clear_dispensers();
    for( int i = 0; i < 2; ++i )
    {
        DraftConfig::CardDispenser* dispenser = dc.add_dispensers();
        dispenser->add_source_booster_set_codes( "" );
    }
    CATCH_REQUIRE( dc.dispensers_size() == 2 );

    CATCH_REQUIRE( dc.IsInitialized() );

    // create 2 dispensers to match the config
    DraftCardDispenserSharedPtrVector<> dispensers;
    dispensers.push_back( std::make_shared<TestDefaults::TestCardDispenser>( "a" ) );
    dispensers.push_back( std::make_shared<TestDefaults::TestCardDispenser>( "b" ) );

    Draft<> d( dc, dispensers );
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_NEW );

    // start the draft, make sure everything got placed properly
    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );

    CATCH_REQUIRE( d.getPackQueueSize( 5 ) == 0 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 0 );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 1 );
    std::vector<std::string> topPack0 = d.getTopPackUnselectedCards( 0 );
    CATCH_REQUIRE( topPack0.size() == 45 );
    CATCH_REQUIRE( std::count( topPack0.begin(), topPack0.end(), "a:card0" ) == 2 );
    CATCH_REQUIRE( std::count( topPack0.begin(), topPack0.end(), "b:card0" ) == 1 );

    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 1 );
    std::vector<std::string> topPack1 = d.getTopPackUnselectedCards( 1 );
    CATCH_REQUIRE( topPack1.size() == 30 );
    CATCH_REQUIRE( std::count( topPack1.begin(), topPack1.end(), "a:card0" ) == 1 );
    CATCH_REQUIRE( std::count( topPack1.begin(), topPack1.end(), "b:card0" ) == 1 );

    CATCH_REQUIRE( d.getPackQueueSize( 2 ) == 1 );
    std::vector<std::string> topPack2 = d.getTopPackUnselectedCards( 2 );
    CATCH_REQUIRE( topPack2.size() == 30 );
    CATCH_REQUIRE( std::count( topPack2.begin(), topPack2.end(), "a:card0" ) == 2 );
    CATCH_REQUIRE( std::count( topPack2.begin(), topPack2.end(), "b:card0" ) == 0 );

    CATCH_REQUIRE( d.getPackQueueSize( 3 ) == 1 );
    std::vector<std::string> topPack3 = d.getTopPackUnselectedCards( 3 );
    CATCH_REQUIRE( topPack3.size() == 15 );
    CATCH_REQUIRE( std::count( topPack3.begin(), topPack3.end(), "a:card0" ) == 1 );
    CATCH_REQUIRE( std::count( topPack3.begin(), topPack3.end(), "b:card0" ) == 0 );

    CATCH_REQUIRE( d.getPackQueueSize( 4 ) == 1 );
    std::vector<std::string> topPack4 = d.getTopPackUnselectedCards( 4 );
    CATCH_REQUIRE( topPack4.size() == 15 );
    CATCH_REQUIRE( std::count( topPack4.begin(), topPack4.end(), "a:card0" ) == 1 );
    CATCH_REQUIRE( std::count( topPack4.begin(), topPack4.end(), "b:card0" ) == 0 );

    CATCH_REQUIRE( d.getPackQueueSize( 6 ) == 1 );
    std::vector<std::string> topPack6 = d.getTopPackUnselectedCards( 6 );
    CATCH_REQUIRE( topPack6.size() == 15 );
    CATCH_REQUIRE( std::count( topPack6.begin(), topPack6.end(), "a:card0" ) == 1 );
    CATCH_REQUIRE( std::count( topPack6.begin(), topPack6.end(), "b:card0" ) == 0 );
}


// With no chairs specified, a booster draft should default to dispensing to each chair.
CATCH_TEST_CASE( "Simple booster draft with no chairs specified", "[draft][misc]" )
{
    auto dispensers = TestDefaults::getDispensers();
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );

    DraftConfig::BoosterRound* boosterRound = dc.mutable_rounds(0)->mutable_booster_round();
    boosterRound->clear_dispensations();
    DraftConfig::CardDispensation* dispensation = boosterRound->add_dispensations();
    dispensation->set_dispenser_index( 0 );
    dispensation->clear_chair_indices();
    dispensation->set_dispense_all( true );

    Draft<> d( dc, dispensers );

    d.start();

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );

    for( int i = 0; i < 8; ++i )
    {
        CATCH_REQUIRE( d.getPackQueueSize( i ) == 1 );
        std::vector<std::string> topPack = d.getTopPackUnselectedCards( i );
        CATCH_REQUIRE( topPack.size() == 15 );
    }
}


CATCH_TEST_CASE( "Booster draft with varying dispensation qty", "[draft][misc]" )
{
    auto dispensers = TestDefaults::getDispensers();

    class LocalTestDraftObserver : public TestDraftObserver
    {
    public:
        LocalTestDraftObserver() : mCards( 0 ) {}
        virtual void notifyNewPack( Draft<>& draft, int chairIndex, uint32_t packId,
                const std::vector<std::string>& unselectedCards ) override
        {
            mCards += unselectedCards.size();
        }
        unsigned int mCards;
    };

    // Draft Config: rounds=3, chairs=8, time=60, dispQty=0
    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( 3, 8, 60 );

    DraftConfig::BoosterRound* boosterRound = dc.mutable_rounds(0)->mutable_booster_round();
    DraftConfig::CardDispensation* dispensation = boosterRound->mutable_dispensations( 0 );
    dispensation->set_dispense_all( false );

    CATCH_SECTION( "dispenser qty 1" )
    {
        dispensation->set_quantity( 1 );

        Draft<> d( dc, dispensers );
        LocalTestDraftObserver obs;
        d.addObserver( &obs );

        d.start();

        // Should have seen (8 players * 1 cards) in the new pack notifications.
        CATCH_REQUIRE( obs.mCards == 8 * 1 );
    }

    CATCH_SECTION( "dispenser qty 10" )
    {
        dispensation->set_quantity( 10 );

        Draft<> d( dc, dispensers );
        LocalTestDraftObserver obs;
        d.addObserver( &obs );

        d.start();

        // Should have seen (8 players * 1 cards) in the new pack notifications.
        CATCH_REQUIRE( obs.mCards == 8 * 10 );
    }

    CATCH_SECTION( "dispenser qty 100" )
    {
        dispensation->set_quantity( 100 );

        Draft<> d( dc, dispensers );
        LocalTestDraftObserver obs;
        d.addObserver( &obs );

        d.start();

        // Should have seen (8 players * 100 cards) in the new pack notifications.
        CATCH_REQUIRE( obs.mCards == 8 * 100 );
    }
}

