#include "catch.hpp"

//
// IMPORTANT: This is a test of private internals within the Draft class.
//

#define private public
#include "Draft.h"
#undef private

CATCH_TEST_CASE( "Packs (private)", "[draft]" )
{
    Draft<>::Pack p(0);
    std::vector<Draft<>::CardSharedPtr> cards;
    for( int i = 0; i < 15; ++i )
    {
        Draft<>::CardSharedPtr c( new Draft<>::Card( "card" + std::to_string(i) ) );
        cards.push_back( c );
    }

    for( auto card : cards )
    {
        p.addCard( card );
    }

    CATCH_SECTION( "Check initial assumptions" )
    {
        CATCH_REQUIRE( p.getCardCount() == 15 );
        CATCH_REQUIRE( p.getUnselectedCardCount() == 15 );
        CATCH_REQUIRE( p.getSelectedCardCount() == 0 );

        for( std::size_t i = 0; i < p.getCardCount(); ++i )
        {
            CATCH_REQUIRE( p.getCard(i) != nullptr );
        }
        CATCH_REQUIRE( p.getCard( p.getCardCount() ) == nullptr );
    }

    CATCH_SECTION( "A card is selected from the middle" )
    {
        Draft<>::Chair* chairPtr = new Draft<>::Chair(0);

        CATCH_REQUIRE_FALSE( cards[10]->isSelected() );
        cards[10]->setSelected( chairPtr, 0, 0 );
        CATCH_REQUIRE( cards[10]->isSelected() );

        CATCH_CHECK( p.getCardCount() == 15 );
        CATCH_CHECK( p.getUnselectedCardCount() == 14 );
        CATCH_CHECK( p.getSelectedCardCount() == 1 );

        Draft<>::CardSharedPtr card;
        card = p.getCard( 10 );
        CATCH_CHECK( card->getSelectedChair() == chairPtr );

        card = p.getCard( 11 );
        CATCH_CHECK( card->getSelectedChair() == nullptr );

        delete chairPtr;
    }

    CATCH_SECTION( "The pack is drained" )
    {
        Draft<>::Chair* chairPtr = new Draft<>::Chair(0);

        while( p.getUnselectedCardCount() > 0 )
        {
            p.getUnselectedCards()[0]->setSelected( chairPtr, 0, 0 );
        }
        CATCH_CHECK( p.getCardCount() == 15 );
        CATCH_CHECK( p.getUnselectedCardCount() == 0 );
        CATCH_CHECK( p.getSelectedCardCount() == 15 );

        delete chairPtr;
    }

}
