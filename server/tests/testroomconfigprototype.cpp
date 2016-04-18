#include "catch.hpp"
#include "RoomConfigPrototype.h"
#include "MtgJsonAllSetsData.h"

CATCH_TEST_CASE( "RoomConfigPrototype", "[roomconfigprototype]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigprototype" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    const std::string allSetsDataFilename = "AllSets.json";
    FILE* allSetsDataFile = fopen( allSetsDataFilename.c_str(), "r" );
    if( allSetsDataFile == NULL )
        CATCH_FAIL( "Failed to open AllSets.json: it must be co-located with the test executable." );

    // Avoid reconstruction/parsing for every test case.
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
    // Create a model RoomConfiguration that test cases can tweak.
    //

    thicket::RoomConfiguration roomConfig;
    roomConfig.set_name( "test" );
    roomConfig.set_password_protected( true );
    roomConfig.set_chair_count( 8 );
    roomConfig.set_bot_count( 4 );

    for( int i = 0; i < 3; ++i )
    {
        thicket::RoomConfiguration::Round* round = roomConfig.add_rounds();
        thicket::RoomConfiguration::BoosterRoundConfiguration* boosterRoundConfig = round->mutable_booster_round_config();
        boosterRoundConfig->set_time( 60 );

        thicket::RoomConfiguration::CardBundle* bundle = boosterRoundConfig->add_card_bundles();
        bundle->set_set_code( "LEA" );
        bundle->set_method( bundle->METHOD_BOOSTER );
        bundle->set_set_replacement( true );
        boosterRoundConfig->set_clockwise( true );
    }

    //
    // Test cases.
    //

    CATCH_SECTION( "Sunny Day" )
    {
        RoomConfigPrototype rcp( allSetsSharedPtr, roomConfig, "password", loggingConfig );
        CATCH_REQUIRE( rcp.getStatus() == rcp.STATUS_OK );
        CATCH_REQUIRE( rcp.getRoomName() == "test" );
        CATCH_REQUIRE( rcp.getPassword() == "password" );
        CATCH_REQUIRE( rcp.getChairCount() == 8 );
        CATCH_REQUIRE( rcp.getBotCount() == 4 );
        CATCH_REQUIRE( rcp.generateDraftRoundConfigs().size() == 3 );
    }

    CATCH_SECTION( "Bad Chair Count" )
    {
        roomConfig.set_chair_count( 0 );
        RoomConfigPrototype rcp( allSetsSharedPtr, roomConfig, "password", loggingConfig );
        CATCH_REQUIRE( rcp.getStatus() == rcp.STATUS_BAD_CHAIR_COUNT );
    }

    CATCH_SECTION( "Bad Bot Count" )
    {
        roomConfig.set_bot_count( 8 );
        RoomConfigPrototype rcp( allSetsSharedPtr, roomConfig, "password", loggingConfig );
        CATCH_REQUIRE( rcp.getStatus() == rcp.STATUS_BAD_BOT_COUNT );
    }

    CATCH_SECTION( "Bad Round Count" )
    {
        roomConfig.clear_rounds();
        RoomConfigPrototype rcp( allSetsSharedPtr, roomConfig, "password", loggingConfig );
        CATCH_REQUIRE( rcp.getStatus() == rcp.STATUS_BAD_ROUND_COUNT );
    }

    CATCH_SECTION( "Bad Round Config (no bundles)" )
    {
        roomConfig.clear_rounds();

        thicket::RoomConfiguration::Round* round = roomConfig.add_rounds();
        thicket::RoomConfiguration::BoosterRoundConfiguration* boosterRoundConfig =
                round->mutable_booster_round_config();
        boosterRoundConfig->set_time( 60 );

        RoomConfigPrototype rcp( allSetsSharedPtr, roomConfig, "password", loggingConfig );
        CATCH_REQUIRE( rcp.getStatus() == rcp.STATUS_BAD_ROUND_CONFIG );
    }

    CATCH_SECTION( "Bad Draft Type" )
    {
        roomConfig.clear_rounds();

        thicket::RoomConfiguration::Round* round = roomConfig.add_rounds();
        thicket::RoomConfiguration::SealedRoundConfiguration* sealedRoundConfig =
                round->mutable_sealed_round_config();

        RoomConfigPrototype rcp( allSetsSharedPtr, roomConfig, "password", loggingConfig );
        CATCH_REQUIRE( rcp.getStatus() == rcp.STATUS_BAD_DRAFT_TYPE );
    }

    CATCH_SECTION( "Bad Set Code" )
    {
        roomConfig.clear_rounds();

        thicket::RoomConfiguration::Round* round = roomConfig.add_rounds();
        thicket::RoomConfiguration::BoosterRoundConfiguration* boosterRoundConfig =
                round->mutable_booster_round_config();
        boosterRoundConfig->set_time( 60 );

        thicket::RoomConfiguration::CardBundle* bundle = boosterRoundConfig->add_card_bundles();
        bundle->set_set_code( "BADSETCODE" );
        bundle->set_method( bundle->METHOD_BOOSTER );
        bundle->set_set_replacement( true );
        boosterRoundConfig->set_clockwise( true );

        RoomConfigPrototype rcp( allSetsSharedPtr, roomConfig, "password", loggingConfig );
        CATCH_REQUIRE( rcp.getStatus() == rcp.STATUS_BAD_SET_CODE );
    }

}
