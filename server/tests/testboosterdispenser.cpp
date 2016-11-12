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
    dispenserSpec.set_set_code( "LEA" );
    dispenserSpec.set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
    dispenserSpec.set_replacement( DraftConfig::CardDispenser::REPLACEMENT_ALWAYS );

    CATCH_SECTION( "Sunny Day" )
    {
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( disp.isValid() );
    }

    CATCH_SECTION( "Invalid Method Types" )
    {
        dispenserSpec.set_method( DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM );
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( !disp.isValid() );
    }

    CATCH_SECTION( "Invalid Replacement Types" )
    {
        for( auto r : { DraftConfig::CardDispenser::REPLACEMENT_UNDERFLOW_ONLY,
                        DraftConfig::CardDispenser::REPLACEMENT_START_OF_ROUND } )
        {
            dispenserSpec.set_replacement( r );
            BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
            CATCH_REQUIRE( !disp.isValid() );
        }
    }

    CATCH_SECTION( "Bad Set Code" )
    {
        dispenserSpec.set_set_code( "" );
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( !disp.isValid() );
    }

    CATCH_SECTION( "Dispensing" )
    {
        BoosterDispenser disp( dispenserSpec, allSetsSharedPtr, loggingConfig );
        CATCH_REQUIRE( disp.isValid() );

        std::vector<DraftCard> cardsDispensed;
        for( int i = 0; i < 10; ++i )
        {
            auto d = disp.dispense();
            cardsDispensed.insert( cardsDispensed.end(), d.begin(), d.end() );
        }
        CATCH_REQUIRE( cardsDispensed.size() == 10 * 15 );
    }
}
