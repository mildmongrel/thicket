#include "catch.hpp"
//#include "spdlog/spdlog.h"
#include "PlayerInventory.h"
#include "SimpleCardData.h"

CATCH_TEST_CASE( "Player Inventory", "[playerinventory]" )
{
    PlayerInventory inv;
    CATCH_REQUIRE( inv.size() == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 0 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 0 );

    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ) ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ) ) );
    CATCH_REQUIRE( inv.add( std::make_shared<SimpleCardData>( "Fireball", "4ED" ) ) );

    CATCH_REQUIRE( inv.size() == 3 );
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

    CATCH_REQUIRE( inv.size() == 6 );
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

    CATCH_REQUIRE( inv.size() == 9 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
    CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 3 );

    CATCH_SECTION( "Rotate Zones" )
    {
        //
        // Rotate everything around.
        //

        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_SIDEBOARD ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_SIDEBOARD ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_SIDEBOARD ) );

        CATCH_REQUIRE( inv.size() == 9 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 6 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 3 );

        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_JUNK ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Lightning Bolt", "3ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_JUNK ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_SIDEBOARD, PlayerInventory::ZONE_JUNK ) );

        CATCH_REQUIRE( inv.size() == 9 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 0 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 6 );

        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Disenchant", "3ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_MAIN ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fear", "3ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_MAIN ) );
        CATCH_REQUIRE( inv.move( std::make_shared<SimpleCardData>( "Fireball", "4ED" ),
                PlayerInventory::ZONE_JUNK, PlayerInventory::ZONE_MAIN ) );

        CATCH_REQUIRE( inv.size() == 9 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_MAIN ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_SIDEBOARD ) == 3 );
        CATCH_REQUIRE( inv.size( PlayerInventory::ZONE_JUNK ) == 3 );
    }

    CATCH_SECTION( "Bad Moves" )
    {
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
