#include "catch.hpp"
#include "PlayerInventory.h"
#include "SimpleCardData.h"

CATCH_TEST_CASE( "Player Inventory", "[playerinventory]" )
{
    PlayerInventory inv;
    CATCH_REQUIRE( inv.size() == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 0 );

    // Add some cards to auto.
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Dark Ritual", "3ED" ),
            PlayerInventory::ZONE_AUTO ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Sol Ring", "3ED" ),
            PlayerInventory::ZONE_AUTO ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
            PlayerInventory::ZONE_AUTO ) );

    CATCH_REQUIRE( inv.size() == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 0 );

    // Add some cards to main, including some copies of cards in auto.
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
            PlayerInventory::ZONE_MAIN ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
            PlayerInventory::ZONE_MAIN ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
            PlayerInventory::ZONE_MAIN ) );

    CATCH_REQUIRE( inv.size() == 6 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 0 );

    // Add some cards to sideboard, including copies of cards in main.
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
            PlayerInventory::ZONE_SIDEBOARD ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
            PlayerInventory::ZONE_SIDEBOARD ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
            PlayerInventory::ZONE_SIDEBOARD ) );

    CATCH_REQUIRE( inv.size() == 9 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 0 );

    // Add some cards to junk, including copies of cards in main and sideboard.
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
            PlayerInventory::ZONE_JUNK ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
            PlayerInventory::ZONE_JUNK ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
            PlayerInventory::ZONE_JUNK ) );

    CATCH_REQUIRE( inv.size() == 12 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 3 );

#if 1
    CATCH_SECTION( "Get Cards" )
    {
        auto cards = inv.getCards( PlayerInventory::ZONE_AUTO );
        CATCH_REQUIRE( cards.size() == 3 );
        CATCH_REQUIRE( std::find_if( cards.begin(), cards.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Sol Ring", "3ED" );
                        } ) != cards.end() );
        CATCH_REQUIRE_FALSE( std::find_if( cards.begin(), cards.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Fear", "3ED" );
                        } ) != cards.end() );
    }
#endif

    CATCH_SECTION( "Rotate Zones" )
    {
        //
        // Rotate everything around.
        //

        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Dark Ritual", "3ED" ),
                PlayerInventory::ZONE_AUTO, PlayerInventory::ZONE_MAIN ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Sol Ring", "3ED" ),
                PlayerInventory::ZONE_AUTO, PlayerInventory::ZONE_MAIN ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_AUTO, PlayerInventory::ZONE_MAIN ) );

        CATCH_REQUIRE( inv.size() == 12 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 6 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 3 );

        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_SIDEBOARD ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_SIDEBOARD ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_SIDEBOARD ) );

        CATCH_REQUIRE( inv.size() == 12 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 6 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 3 );

        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_JUNK ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_JUNK ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_JUNK ) );

        CATCH_REQUIRE( inv.size() == 12 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 6 );

        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_AUTO ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_AUTO ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_AUTO ) );

        CATCH_REQUIRE( inv.size() == 12 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 3 );

        //
        // Make sure everything is in the right place now.
        //

        auto cardsAuto = inv.getCards( PlayerInventory::ZONE_AUTO );
        CATCH_REQUIRE( cardsAuto.size() == 3 );
        CATCH_REQUIRE( std::find_if( cardsAuto.begin(), cardsAuto.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Disenchant", "3ED" );
                        } ) != cardsAuto.end() );
        CATCH_REQUIRE( std::find_if( cardsAuto.begin(), cardsAuto.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Fear", "3ED" );
                        } ) != cardsAuto.end() );
        CATCH_REQUIRE( std::find_if( cardsAuto.begin(), cardsAuto.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Fireball", "4ED" );
                        } ) != cardsAuto.end() );

        auto cardsMain = inv.getCards( PlayerInventory::ZONE_MAIN );
        CATCH_REQUIRE( cardsMain.size() == 3 );
        CATCH_REQUIRE( std::find_if( cardsMain.begin(), cardsMain.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Dark Ritual", "3ED" );
                        } ) != cardsMain.end() );
        CATCH_REQUIRE( std::find_if( cardsMain.begin(), cardsMain.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Sol Ring", "3ED" );
                        } ) != cardsMain.end() );
        CATCH_REQUIRE( std::find_if( cardsMain.begin(), cardsMain.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Fireball", "4ED" );
                        } ) != cardsMain.end() );

        auto cardsSB = inv.getCards( PlayerInventory::ZONE_SIDEBOARD );
        CATCH_REQUIRE( cardsSB.size() == 3 );
        CATCH_REQUIRE( std::find_if( cardsSB.begin(), cardsSB.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Disenchant", "3ED" );
                        } ) != cardsSB.end() );
        CATCH_REQUIRE( std::find_if( cardsSB.begin(), cardsSB.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Lightning Bolt", "3ED" );
                        } ) != cardsSB.end() );
        CATCH_REQUIRE( std::find_if( cardsSB.begin(), cardsSB.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Fireball", "4ED" );
                        } ) != cardsSB.end() );

        auto cardsJunk = inv.getCards( PlayerInventory::ZONE_JUNK );
        CATCH_REQUIRE( cardsAuto.size() == 3 );
        CATCH_REQUIRE( std::find_if( cardsJunk.begin(), cardsJunk.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Fear", "3ED" );
                        } ) != cardsJunk.end() );
        CATCH_REQUIRE( std::find_if( cardsJunk.begin(), cardsJunk.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Lightning Bolt", "3ED" );
                        } ) != cardsJunk.end() );
        CATCH_REQUIRE( std::find_if( cardsJunk.begin(), cardsJunk.end(),
                       [&]( const PlayerInventory::CardDataSharedPtr& c ) {
                           return *c == SimpleCardData( "Fireball", "4ED" );
                        } ) != cardsJunk.end() );

    }

    CATCH_SECTION( "Moves to self" )
    {
        CATCH_REQUIRE(
                inv.move( std::make_shared<SimpleCardData>( "Dark Ritual", "3ED" ),
                PlayerInventory::ZONE_AUTO, PlayerInventory::ZONE_AUTO ) );
        CATCH_REQUIRE(
                inv.move( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_MAIN ) );
        CATCH_REQUIRE(
                inv.move( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_SIDEBOARD ) );
        CATCH_REQUIRE(
                inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_JUNK ) );
    }

    CATCH_SECTION( "Bad Moves" )
    {
        CATCH_REQUIRE_FALSE(
                inv.move( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
                PlayerInventory::ZONE_AUTO, PlayerInventory::ZONE_AUTO ) );
        CATCH_REQUIRE_FALSE(
                inv.move( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_MAIN ) );
        CATCH_REQUIRE_FALSE(
                inv.move( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_SIDEBOARD ) );
        CATCH_REQUIRE_FALSE(
                inv.move( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_JUNK ) );
    }

    CATCH_SECTION( "Clear" )
    {
        inv.clear();
        CATCH_REQUIRE( inv.size() == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_AUTO ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 0 );
    }

    CATCH_SECTION( "Basic Lands" )
    {
        PlayerInventory invBasics;

        invBasics.adjustBasicLand( BASIC_LAND_PLAINS,   PlayerInventory::ZONE_MAIN, 1 );
        invBasics.adjustBasicLand( BASIC_LAND_ISLAND,   PlayerInventory::ZONE_MAIN, 2 );
        invBasics.adjustBasicLand( BASIC_LAND_SWAMP,    PlayerInventory::ZONE_MAIN, 3 );
        invBasics.adjustBasicLand( BASIC_LAND_MOUNTAIN, PlayerInventory::ZONE_MAIN, 4 );
        invBasics.adjustBasicLand( BASIC_LAND_FOREST,   PlayerInventory::ZONE_MAIN, 5 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_MAIN ) == 15 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_SIDEBOARD ) == 0 );
        CATCH_REQUIRE( invBasics.size() == 15 );

        invBasics.adjustBasicLand( BASIC_LAND_PLAINS,   PlayerInventory::ZONE_SIDEBOARD, 5 );
        invBasics.adjustBasicLand( BASIC_LAND_ISLAND,   PlayerInventory::ZONE_SIDEBOARD, 4 );
        invBasics.adjustBasicLand( BASIC_LAND_SWAMP,    PlayerInventory::ZONE_SIDEBOARD, 3 );
        invBasics.adjustBasicLand( BASIC_LAND_MOUNTAIN, PlayerInventory::ZONE_SIDEBOARD, 2 );
        invBasics.adjustBasicLand( BASIC_LAND_FOREST,   PlayerInventory::ZONE_SIDEBOARD, 1 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_MAIN ) == 15 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_SIDEBOARD ) == 15 );
        CATCH_REQUIRE( invBasics.size() == 30 );

        invBasics.adjustBasicLand( BASIC_LAND_PLAINS,   PlayerInventory::ZONE_MAIN, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_ISLAND,   PlayerInventory::ZONE_MAIN, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_SWAMP,    PlayerInventory::ZONE_MAIN, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_MOUNTAIN, PlayerInventory::ZONE_MAIN, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_FOREST,   PlayerInventory::ZONE_MAIN, -1 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_MAIN ) == 10 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_SIDEBOARD ) == 15 );
        CATCH_REQUIRE( invBasics.size() == 25 );

        invBasics.adjustBasicLand( BASIC_LAND_PLAINS,   PlayerInventory::ZONE_SIDEBOARD, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_ISLAND,   PlayerInventory::ZONE_SIDEBOARD, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_SWAMP,    PlayerInventory::ZONE_SIDEBOARD, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_MOUNTAIN, PlayerInventory::ZONE_SIDEBOARD, -1 );
        invBasics.adjustBasicLand( BASIC_LAND_FOREST,   PlayerInventory::ZONE_SIDEBOARD, -1 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_MAIN ) == 10 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_SIDEBOARD ) == 10 );
        CATCH_REQUIRE( invBasics.size() == 20 );

        // Check can't go below zero.
        CATCH_REQUIRE_FALSE( invBasics.adjustBasicLand(
                    BASIC_LAND_PLAINS, PlayerInventory::ZONE_MAIN, -1 ) );
        CATCH_REQUIRE_FALSE( invBasics.adjustBasicLand(
                    BASIC_LAND_FOREST, PlayerInventory::ZONE_SIDEBOARD, -1 ) );
        CATCH_REQUIRE( invBasics.size() == 20 );

        BasicLandQuantities qtys;
        qtys = invBasics.getBasicLandQuantities( PlayerInventory::ZONE_MAIN );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_PLAINS )   == 0 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_ISLAND )   == 1 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_SWAMP )    == 2 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_MOUNTAIN ) == 3 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_FOREST )   == 4 );

        qtys = invBasics.getBasicLandQuantities( PlayerInventory::ZONE_SIDEBOARD );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_PLAINS )   == 4 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_ISLAND )   == 3 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_SWAMP )    == 2 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_MOUNTAIN ) == 1 );
        CATCH_REQUIRE( qtys.getQuantity( BASIC_LAND_FOREST )   == 0 );

        invBasics.clear();
        CATCH_REQUIRE( invBasics.size() == 0 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_MAIN ) == 0 );
        CATCH_REQUIRE( invBasics.size( PlayerInventory::ZONE_SIDEBOARD ) == 0 );
    }
}
