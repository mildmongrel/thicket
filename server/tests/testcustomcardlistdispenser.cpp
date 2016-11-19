#include "catch.hpp"
#include "messages.pb.h"
#include "CustomCardListDispenser.h"

using namespace proto;

CATCH_TEST_CASE( "CustomCardListDispenser", "[customcardlistdispenser]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "customcardlistdispenser" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    //
    // Create baseline proto messages that test cases can tweak.
    //

    DraftConfig::CustomCardList customCardListSpec;
    customCardListSpec.set_name( "Test List" );
    DraftConfig::CustomCardList::CardQuantity* cardQty = customCardListSpec.add_card_quantities();
    cardQty->set_quantity( 1 );
    cardQty->set_name( "Test Card" );
    cardQty->set_set_code( "TST" );

    DraftConfig::CardDispenser dispenserSpec;
    dispenserSpec.set_custom_card_list_index( 0 );
    dispenserSpec.set_method( DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM );
    dispenserSpec.set_replacement( DraftConfig::CardDispenser::REPLACEMENT_UNDERFLOW_ONLY );


    CATCH_SECTION( "Sunny Day" )
    {
        CustomCardListDispenser disp( dispenserSpec, customCardListSpec, loggingConfig );
        CATCH_REQUIRE( disp.isValid() );
    }

    CATCH_SECTION( "Empty List" )
    {
        customCardListSpec.clear_card_quantities();
        CustomCardListDispenser disp( dispenserSpec, customCardListSpec, loggingConfig );
        CATCH_REQUIRE( !disp.isValid() );
    }

    CATCH_SECTION( "Invalid Method Types" )
    {
        dispenserSpec.set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
        CustomCardListDispenser disp( dispenserSpec, customCardListSpec, loggingConfig );
        CATCH_REQUIRE( !disp.isValid() );
    }

    CATCH_SECTION( "Invalid Replacement Types" )
    {
        for( auto r : { DraftConfig::CardDispenser::REPLACEMENT_ALWAYS,
                        DraftConfig::CardDispenser::REPLACEMENT_START_OF_ROUND } )
        {
            dispenserSpec.set_replacement( r );
            CustomCardListDispenser disp( dispenserSpec, customCardListSpec, loggingConfig );
            CATCH_REQUIRE( !disp.isValid() );
        }
    }

    CATCH_SECTION( "Dispensing" )
    {
        customCardListSpec.clear_card_quantities();

        for( int i = 0; i < 3; ++i )
        {
            int cardNum = i + 1;
            DraftConfig::CustomCardList::CardQuantity* cardQty = customCardListSpec.add_card_quantities();
            cardQty->set_quantity( cardNum );
            cardQty->set_set_code( "TST" );
            cardQty->set_name( "card" + std::to_string(cardNum) );
        }
        CustomCardListDispenser disp( dispenserSpec, customCardListSpec, loggingConfig );
        CATCH_REQUIRE( disp.isValid() );
        CATCH_REQUIRE( disp.getPoolSize() == 6 );

        std::vector<DraftCard> cardsDispensed;
        for( int i = 0; i < 60; ++i )
        {
            auto d = disp.dispense();
            cardsDispensed.insert( cardsDispensed.end(), d.begin(), d.end() );
        }
        CATCH_REQUIRE( cardsDispensed.size() == 60 );
        CATCH_REQUIRE( std::count_if( cardsDispensed.begin(), cardsDispensed.end(), [] (const DraftCard& dc) { return dc.name == "card1"; } ) == 10 );
        CATCH_REQUIRE( std::count_if( cardsDispensed.begin(), cardsDispensed.end(), [] (const DraftCard& dc) { return dc.name == "card2"; } ) == 20 );
        CATCH_REQUIRE( std::count_if( cardsDispensed.begin(), cardsDispensed.end(), [] (const DraftCard& dc) { return dc.name == "card3"; } ) == 30 );
    }
}
