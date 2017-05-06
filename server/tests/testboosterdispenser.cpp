#include "catch.hpp"
#include "messages.pb.h"
#include "BoosterDispenser.h"
#include "MtgJsonAllSetsData.h"

using namespace proto;

CATCH_TEST_CASE( "BoosterDispenser", "[boosterdispenser]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "boosterdispenser" );
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

    //
    // Create baseline proto messages that test cases can tweak.
    //

    DraftConfig::CardDispenser dispenserSpec;
    dispenserSpec.add_source_booster_set_codes( "LEA" );

    CATCH_SECTION( "Sunny Day" )
    {
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( disp.isValid() );
    }

    CATCH_SECTION( "Bad Set Code" )
    {
        dispenserSpec.set_source_booster_set_codes( 0, "" );
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( !disp.isValid() );
    }

    CATCH_SECTION( "Dispensing all" )
    {
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( disp.isValid() );
        std::vector<DraftCard> cardsDispensed;
        for( int i = 0; i < 10; ++i )
        {
            auto d = disp.dispenseAll();
            cardsDispensed.insert( cardsDispensed.end(), d.begin(), d.end() );
        }

        CATCH_REQUIRE( cardsDispensed.size() == 10 * 15 );
    }

    CATCH_SECTION( "Dispensing one" )
    {
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( disp.isValid() );
        std::vector<DraftCard> cardsDispensed;
        for( int i = 0; i < 100; ++i )
        {
            auto d = disp.dispense( 1 );
            cardsDispensed.insert( cardsDispensed.end(), d.begin(), d.end() );
        }

        CATCH_REQUIRE( cardsDispensed.size() == 100 );
    }
}
