#include "catch.hpp"
#include "Draft.h"
#include "testdefaults.h"

CATCH_TEST_CASE( "Draft created with not enough dispensers", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getDraftConfig( 3, 8, 60 );
    DraftCardDispenserSharedPtrVector<> dispensers; // no dispensers
    Draft<> d( dc, dispensers );

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_ERROR );

    // Make sure the state is still in error after trying to start.
    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_ERROR );
}


CATCH_TEST_CASE( "Draft not restartable", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getDraftConfig( 3, 8, 60 );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers );

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_NEW );

    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );

    d.start();
    CATCH_REQUIRE( d.getState() == Draft<>::STATE_RUNNING );
}


CATCH_TEST_CASE( "Pass direction clockwise", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getDraftConfig( 3, 8, 60 );
    auto dispensers = TestDefaults::getDispensers();

    dc.mutable_rounds(0)->mutable_booster_round()->set_pass_direction(
            DraftConfig::DIRECTION_CLOCKWISE );
    Draft<> d( dc, dispensers );

    d.start();

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 1 );

    d.makeCardSelection( 0, ":card0" );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 0 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 2 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 1 );
}


CATCH_TEST_CASE( "Pass direction counter-clockwise", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getDraftConfig( 3, 8, 60 );
    auto dispensers = TestDefaults::getDispensers();

    dc.mutable_rounds(0)->mutable_booster_round()->set_pass_direction(
            DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
    Draft<> d( dc, dispensers );

    d.start();

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 1 );

    d.makeCardSelection( 0, ":card0" );

    CATCH_REQUIRE( d.getPackQueueSize( 0 ) == 0 );
    CATCH_REQUIRE( d.getPackQueueSize( 1 ) == 1 );
    CATCH_REQUIRE( d.getPackQueueSize( 7 ) == 2 );
}


CATCH_TEST_CASE( "Unusual dispensations", "[draft][misc]" )
{
    DraftConfig dc = TestDefaults::getDraftConfig( 3, 8, 60 );

    // re-do the dispensations in the config:
    //   dispensation 0: dispenser 0, chair 0,2,4,6
    //   dispensation 1: dispenser 0, chair 0,1,2,3
    //   dispensation 2: dispenser 1, chair 0,1
    DraftConfig::BoosterRound* boosterRound = dc.mutable_rounds(0)->mutable_booster_round();
    boosterRound->clear_dispensations();
    DraftConfig::CardDispensation* dispensation0 = boosterRound->add_dispensations();
    dispensation0->set_card_dispenser_index( 0 );
    for( int i : { 0, 2, 4, 6 } )
    {
        dispensation0->add_chair_indices( i );
    }
    DraftConfig::CardDispensation* dispensation1 = boosterRound->add_dispensations();
    dispensation1->set_card_dispenser_index( 0 );
    for( int i : { 0, 1, 2, 3 } )
    {
        dispensation1->add_chair_indices( i );
    }
    DraftConfig::CardDispensation* dispensation2 = boosterRound->add_dispensations();
    dispensation2->set_card_dispenser_index( 1 );
    for( int i : { 0, 1 } )
    {
        dispensation2->add_chair_indices( i );
    }
    CATCH_REQUIRE( boosterRound->dispensations_size() == 3 );

    // put 2 dispensers into the config
    dc.clear_card_dispensers();
    for( int i = 0; i < 2; ++i )
    {
        DraftConfig::CardDispenser* dispenser = dc.add_card_dispensers();
        dispenser->set_set_code( "" );
        dispenser->set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
        dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_ALWAYS );
    }
    CATCH_REQUIRE( dc.card_dispensers_size() == 2 );

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
