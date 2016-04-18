#include "catch.hpp"
#include "DeckHashing.h"
#include "SimpleCardData.h"

// QString stream output operator.
inline std::ostream& operator <<( std::ostream &os, const QString &str )
{
   return (os << str.toStdString());
}

CATCH_TEST_CASE( "DeckHashing", "[deckhashing]" )
{
    //
    // Test cases.
    //

    PlayerInventory inv;

    CATCH_SECTION( "Empty" )
    {
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "r8sq7riu" );
    }

    CATCH_SECTION( "Main, single card, ASCII chars" )
    {
        inv.add( std::make_shared<SimpleCardData>( "Disenchant" ), PlayerInventory::ZONE_MAIN );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "68i24pc9" );
    }

    CATCH_SECTION( "Main, UTF-8 Æ" )
    {
        inv.add( std::make_shared<SimpleCardData>( "Æther Burst" ), PlayerInventory::ZONE_MAIN );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "hcroa9dk" );
    }

    CATCH_SECTION( "Main, ASCII AE" )
    {
        inv.add( std::make_shared<SimpleCardData>( "AEther Burst" ), PlayerInventory::ZONE_MAIN );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "hcroa9dk" );
    }

    CATCH_SECTION( "Main, UTF-8 right apostrophe" )
    {
        inv.add( std::make_shared<SimpleCardData>( "Chainer’s Edict" ), PlayerInventory::ZONE_MAIN );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "5kdt8hr6" );
    }

    CATCH_SECTION( "Main, single card, Fire/Ice variants" )
    {
        inv.add( std::make_shared<SimpleCardData>( "Fire/Ice" ), PlayerInventory::ZONE_MAIN );
        inv.add( std::make_shared<SimpleCardData>( "Fire / Ice" ), PlayerInventory::ZONE_MAIN );
        inv.add( std::make_shared<SimpleCardData>( "Fire//Ice" ), PlayerInventory::ZONE_MAIN );
        inv.add( std::make_shared<SimpleCardData>( "Fire // Ice" ), PlayerInventory::ZONE_MAIN );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "iq0uqup7" );
    }

    CATCH_SECTION( "Main only - multiple cards" )
    {
        // Test proper sorting, spaces, apostrophes
        inv.add( std::make_shared<SimpleCardData>( "Fireball" ), PlayerInventory::ZONE_MAIN );
        inv.add( std::make_shared<SimpleCardData>( "Chainer's Edict" ), PlayerInventory::ZONE_MAIN );
        inv.add( std::make_shared<SimpleCardData>( "Disenchant" ), PlayerInventory::ZONE_MAIN );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "9ed4d2v3" );
    }

    CATCH_SECTION( "SB only" )
    {
        // Test proper sorting, spaces, apostrophes
        inv.add( std::make_shared<SimpleCardData>( "Fireball" ), PlayerInventory::ZONE_SIDEBOARD );
        inv.add( std::make_shared<SimpleCardData>( "Chainer's Edict" ), PlayerInventory::ZONE_SIDEBOARD );
        inv.add( std::make_shared<SimpleCardData>( "Disenchant" ), PlayerInventory::ZONE_SIDEBOARD );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "9kkv730r" );
    }

    CATCH_SECTION( "Mixed" )
    {
        inv.add( std::make_shared<SimpleCardData>( "Fireball" ), PlayerInventory::ZONE_MAIN );
        inv.add( std::make_shared<SimpleCardData>( "Chainer's Edict" ), PlayerInventory::ZONE_MAIN );
        inv.add( std::make_shared<SimpleCardData>( "Disenchant" ), PlayerInventory::ZONE_MAIN );

        inv.add( std::make_shared<SimpleCardData>( "Fireball" ), PlayerInventory::ZONE_SIDEBOARD );
        inv.add( std::make_shared<SimpleCardData>( "Chainer's Edict" ), PlayerInventory::ZONE_SIDEBOARD );
        inv.add( std::make_shared<SimpleCardData>( "Disenchant" ), PlayerInventory::ZONE_SIDEBOARD );
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "fi8ie39c" );
    }

    CATCH_SECTION( "Basic lands, main" )
    {
        for( auto basic : gBasicLandTypeArray )
        {
            inv.adjustBasicLand( basic, PlayerInventory::ZONE_MAIN, 1 );
        }
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "lhdodeb5" );
    }

    CATCH_SECTION( "Basic lands, sideboard" )
    {
        for( auto basic : gBasicLandTypeArray )
        {
            inv.adjustBasicLand( basic, PlayerInventory::ZONE_SIDEBOARD, 1 );
        }
        QString hash = computeCockatriceHash( inv );
        CATCH_REQUIRE( hash == "g233t401" );
    }
}
