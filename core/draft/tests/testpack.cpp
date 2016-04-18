#include "catch.hpp"
#include "Pack.h"
#include "Chair.h"

CATCH_TEST_CASE( "Packs" "[pack]" )
{
    Pack p;
    std::vector<CardSharedPtr> cards;
    for( int i = 0; i < 15; ++i )
    {
        CardSharedPtr c( new Card( "card" + std::to_string(i) ) );
        cards.push_back( c );
    }

    for( auto card : cards )
    {
        p.addCard( card );
    }

    CATCH_REQUIRE( p.getNumCards() == 15 );
    CATCH_REQUIRE( p.getNumUnselectedCards() == 15 );
    CATCH_REQUIRE( p.getNumSelectedCards() == 0 );

    CATCH_SECTION( "A card is selected from the middle" )
    {
        CATCH_REQUIRE_FALSE( cards[10]->isSelected() );
        cards[10]->setSelected( ChairSharedPtr(new Chair(0)), 0, 0 );
        CATCH_REQUIRE( cards[10]->isSelected() );

        CATCH_CHECK( p.getNumCards() == 15 );
        CATCH_CHECK( p.getNumUnselectedCards() == 14 );
        CATCH_CHECK( p.getNumSelectedCards() == 1 );

        CardSharedPtr c = p.getFirstUnselectedCardByName( "card10" );
        CATCH_REQUIRE( c.get() == nullptr );
        c = p.getFirstUnselectedCardByName( "card9" );
        CATCH_REQUIRE( c.get() != nullptr );
        c = p.getFirstUnselectedCardByName( "card11" );
        CATCH_REQUIRE( c.get() != nullptr );
    }

    CATCH_SECTION( "The pack is drained" )
    {
        while( p.getNumUnselectedCards() > 0 )
        {
            p.getUnselectedCards()[0]->setSelected( ChairSharedPtr(new Chair(0)), 0, 0 );
        }
        CATCH_CHECK( p.getNumCards() == 15 );
        CATCH_CHECK( p.getNumUnselectedCards() == 0 );
        CATCH_CHECK( p.getNumSelectedCards() == 15 );
    }

}
