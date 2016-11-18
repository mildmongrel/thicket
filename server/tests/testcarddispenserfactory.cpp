#include "catch.hpp"
#include "messages.pb.h"
#include "CardDispenserFactory.h"
#include "MtgJsonAllSetsData.h"
#include "BoosterDispenser.h"
#include "CustomCardListDispenser.h"

using namespace proto;

CATCH_TEST_CASE( "CardDispenserFactory", "[carddispenserfactory]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "carddispenserfactory" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    const std::string allSetsDataFilename = "AllSets.json";
    FILE* allSetsDataFile = fopen( allSetsDataFilename.c_str(), "r" );
    if( allSetsDataFile == NULL )
        CATCH_FAIL( "Failed to open AllSets.json: it must be co-located with the test executable." );

    // Static to avoid reconstruction/parsing for every test case.
    static MtgJsonAllSetsData* allSets = nullptr;
    static auto allSetsSharedPtr = std::shared_ptr<const AllSetsData>( nullptr );
    if( !allSets )
    {
        allSets = new MtgJsonAllSetsData();
        allSetsSharedPtr = std::shared_ptr<const AllSetsData>( allSets );
        bool parseResult = allSets->parse( allSetsDataFile );
        fclose( allSetsDataFile );
        CATCH_REQUIRE( parseResult );
    }

    // Static to avoid reconstruction/parsing for every test case.
    static CardDispenserFactory factory( allSetsSharedPtr, loggingConfig );

    CATCH_SECTION( "Booster Dispensers" )
    {
        DraftConfig draftConfig;
        draftConfig.set_chair_count( 8 );

        for( int i = 0; i < 3; ++i )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.add_dispensers();
            dispenser->set_set_code( "10E" );
            dispenser->set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
            dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_ALWAYS );
        }

        CATCH_SECTION( "sunny day" )
        {
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.size() == 3 );
        }
        CATCH_SECTION( "one invalid set" )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.mutable_dispensers( 2 );
            dispenser->set_set_code( "XXX" );
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.empty() );
        }
        CATCH_SECTION( "one invalid method" )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.mutable_dispensers( 2 );
            dispenser->set_method( DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM );
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.empty() );
        }
        CATCH_SECTION( "one invalid replacement" )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.mutable_dispensers( 2 );
            dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_UNDERFLOW_ONLY );
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.empty() );
        }
    }

    CATCH_SECTION( "Custom Card List Dispensers" )
    {
        DraftConfig draftConfig;
        draftConfig.set_chair_count( 8 );

        DraftConfig::CustomCardList* ccl = draftConfig.add_custom_card_lists();
        ccl->set_name( "test list" );
        DraftConfig::CustomCardList::CardQuantity* q = ccl->add_card_quantities();
        q->set_quantity( 1 );
        q->set_name( "test card" );
        q->set_name( "TST" );

        for( int i = 0; i < 3; ++i )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.add_dispensers();
            dispenser->set_custom_card_list_index( 0 );
            dispenser->set_method( DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM );
            dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_UNDERFLOW_ONLY );
        }

        CATCH_SECTION( "sunny day" )
        {
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.size() == 3 );
        }
        CATCH_SECTION( "one invalid index" )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.mutable_dispensers( 2 );
            dispenser->set_custom_card_list_index( 1 );
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.empty() );
        }
        CATCH_SECTION( "one invalid method" )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.mutable_dispensers( 2 );
            dispenser->set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.empty() );
        }
        CATCH_SECTION( "one invalid replacement" )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.mutable_dispensers( 2 );
            dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_ALWAYS );
            auto disps = factory.createCardDispensers( draftConfig );
            CATCH_REQUIRE( disps.empty() );
        }
    }
    CATCH_SECTION( "Mixed Dispensers" )
    {
        DraftConfig draftConfig;
        draftConfig.set_chair_count( 8 );

        DraftConfig::CustomCardList* ccl = draftConfig.add_custom_card_lists();
        ccl->set_name( "test list" );
        DraftConfig::CustomCardList::CardQuantity* q = ccl->add_card_quantities();
        q->set_quantity( 1 );
        q->set_name( "test card" );
        q->set_name( "TST" );

        //
        // Create two dispensers of each type
        //

        for( int i = 0; i < 2; ++i )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.add_dispensers();
            dispenser->set_custom_card_list_index( 0 );
            dispenser->set_method( DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM );
            dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_UNDERFLOW_ONLY );
        }
        for( int i = 0; i < 2; ++i )
        {
            DraftConfig::CardDispenser* dispenser = draftConfig.add_dispensers();
            dispenser->set_set_code( "10E" );
            dispenser->set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
            dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_ALWAYS );
        }

        auto disps = factory.createCardDispensers( draftConfig );
        CATCH_REQUIRE( disps.size() == 4 );

        // Take a look and make sure the factory created the right dispensers.
        CATCH_REQUIRE( dynamic_cast<CustomCardListDispenser*>( disps[0].get() ) != nullptr );
        CATCH_REQUIRE( dynamic_cast<CustomCardListDispenser*>( disps[1].get() ) != nullptr );
        CATCH_REQUIRE( dynamic_cast<BoosterDispenser*>( disps[2].get() ) != nullptr );
        CATCH_REQUIRE( dynamic_cast<BoosterDispenser*>( disps[3].get() ) != nullptr );
    }
}
