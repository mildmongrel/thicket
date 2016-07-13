#include "catch.hpp"
#include "messages.pb.h"
#include "RoomConfigValidator.h"
#include "MtgJsonAllSetsData.h"

using proto::DraftConfig;

CATCH_TEST_CASE( "RoomConfigPrototype", "[roomconfigprototype]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigvalidator" );
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
    static RoomConfigValidator roomConfigValidator( allSetsSharedPtr, loggingConfig );
    thicket::CreateRoomFailureRsp_ResultType failureResult;

    //
    // Create a model RoomConfiguration that test cases can tweak.
    //

    const int CHAIR_COUNT = 8;
    thicket::RoomConfig roomConfig;
    roomConfig.set_name( "testroom" );
    roomConfig.set_password_protected( false );
    roomConfig.set_bot_count( 0 );

    DraftConfig* draftConfig = roomConfig.mutable_draft_config();
    draftConfig->set_version( DraftConfig::VERSION );
    draftConfig->set_chair_count( CHAIR_COUNT );

    // Currently this is hardcoded for three booster rounds.
    for( int i = 0; i < 3; ++i )
    {
        DraftConfig::CardDispenser* dispenser = draftConfig->add_dispensers();
        dispenser->set_set_code( "10E" );
        dispenser->set_method( DraftConfig::CardDispenser::METHOD_BOOSTER );
        dispenser->set_replacement( DraftConfig::CardDispenser::REPLACEMENT_ALWAYS );

        DraftConfig::Round* round = draftConfig->add_rounds();
        DraftConfig::BoosterRound* boosterRound = round->mutable_booster_round();
        boosterRound->set_selection_time( 60 );
        boosterRound->set_pass_direction( (i%2) == 0 ?
                DraftConfig::DIRECTION_CLOCKWISE :
                DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
        DraftConfig::CardDispensation* dispensation = boosterRound->add_dispensations();
        dispensation->set_dispenser_index( i );
        for( int i = 0; i < CHAIR_COUNT; ++i )
        {
            dispensation->add_chair_indices( i );
        }
    }

    CATCH_SECTION( "Sunny Day" )
    {
        CATCH_REQUIRE( roomConfigValidator.validate( roomConfig, failureResult ) );
    }

    CATCH_SECTION( "Bad Chair Count" )
    {
        draftConfig->set_chair_count( 0 );
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_CHAIR_COUNT );
    }

    CATCH_SECTION( "Bad Bot Count" )
    {
        roomConfig.set_bot_count( 8 );
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_BOT_COUNT );
    }

    CATCH_SECTION( "Bad Round Count" )
    {
        draftConfig->clear_rounds();
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_ROUND_COUNT );
    }

    CATCH_SECTION( "Bad Set Code" )
    {
        draftConfig->mutable_dispensers( 0 )->set_set_code( "BADSETCODE" );
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_SET_CODE );
    }

    CATCH_SECTION( "Non-booster Set Code" )
    {
        // Invalid to use non-booster code with booster method
        draftConfig->mutable_dispensers( 0 )->set_set_code( "EVG" );
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_CONFIG );

        // Valid if method is random.
        draftConfig->mutable_dispensers( 0 )->set_method( DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM );
        CATCH_REQUIRE( roomConfigValidator.validate( roomConfig, failureResult ) );
    }

    CATCH_SECTION( "Bad Draft Type - sealed" )
    {
        // Add a sealed round to make draft sealed-only and invalid (for now).
        draftConfig->clear_rounds();
        DraftConfig::Round* round = draftConfig->add_rounds();
        DraftConfig::SealedRound* sealedRound = round->mutable_sealed_round();
        (void) sealedRound;
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_DRAFT_TYPE );
    }

    CATCH_SECTION( "Bad Draft Type - mixed" )
    {
        // Add a sealed round to make draft mixed and invalid (for now).
        DraftConfig::Round* round = draftConfig->add_rounds();
        DraftConfig::SealedRound* sealedRound = round->mutable_sealed_round();
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_DRAFT_TYPE );
    }

    CATCH_SECTION( "Bad Booster Round - no dispensations" )
    {
        DraftConfig::Round* round = draftConfig->mutable_rounds( 0 );
        DraftConfig::BoosterRound* boosterRound = round->mutable_booster_round();
        boosterRound->clear_dispensations();
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_ROUND_CONFIG );
    }

    CATCH_SECTION( "Bad Booster Round - bad dispensation index" )
    {
        DraftConfig::Round* round = draftConfig->mutable_rounds( 0 );
        DraftConfig::BoosterRound* boosterRound = round->mutable_booster_round();
        boosterRound->mutable_dispensations( 0 )->set_dispenser_index( 10 );
        CATCH_REQUIRE_FALSE( roomConfigValidator.validate( roomConfig, failureResult ) );
        CATCH_REQUIRE( failureResult == thicket::CreateRoomFailureRsp::RESULT_INVALID_ROUND_CONFIG );
    }
}
