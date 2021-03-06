#include "catch.hpp"
#include "RoomConfigAdapter.h"

using namespace proto;

CATCH_TEST_CASE( "RoomConfigAdapter - Simple Booster Config", "[roomconfigadapter]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigadapter" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    //
    // Create a RoomConfig
    //

    const int CHAIR_COUNT = 8;
    RoomConfig roomConfig;
    roomConfig.set_name( "testroom" );
    roomConfig.set_password_protected( false );
    roomConfig.set_bot_count( 4 );

    DraftConfig* draftConfig = roomConfig.mutable_draft_config();
    draftConfig->set_chair_count( CHAIR_COUNT );

    //
    // Hardcode for three booster rounds with two dispensers, 10E/3ED/10E.
    //

    DraftConfig::CardDispenser* dispenser;
    dispenser = draftConfig->add_dispensers();
    dispenser->add_source_booster_set_codes( "10E" );

    dispenser = draftConfig->add_dispensers();
    dispenser->add_source_booster_set_codes( "3ED" );

    for( int i = 0; i < 3; ++i )
    {
        DraftConfig::Round* round = draftConfig->add_rounds();
        DraftConfig::BoosterRound* boosterRound = round->mutable_booster_round();
        boosterRound->set_selection_time( 60 );
        boosterRound->set_pass_direction( (i%2) == 0 ?
                DraftConfig::DIRECTION_CLOCKWISE :
                DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
        DraftConfig::CardDispensation* dispensation = boosterRound->add_dispensations();
        dispensation->set_dispenser_index( i % 2 );
        for( int i = 0; i < CHAIR_COUNT; ++i )
        {
            dispensation->add_chair_indices( i );
        }
    }

    //
    // Tests
    //

    RoomConfigAdapter rca( 0, roomConfig, loggingConfig );
    CATCH_REQUIRE( rca.getRoomId() == 0 );
    CATCH_REQUIRE( rca.getName() == "testroom" );
    CATCH_REQUIRE( rca.getChairCount() == 8 );
    CATCH_REQUIRE( rca.getBotCount() == 4 );
    CATCH_REQUIRE( rca.isPasswordProtected() == false );

    CATCH_REQUIRE( rca.getDraftConfig().rounds_size() == 3 );

    CATCH_REQUIRE( rca.getPassDirection( 0 ) == PASS_DIRECTION_CW );
    CATCH_REQUIRE( rca.getPassDirection( 1 ) == PASS_DIRECTION_CCW );
    CATCH_REQUIRE( rca.getPassDirection( 2 ) == PASS_DIRECTION_CW );

    CATCH_REQUIRE( rca.getBoosterRoundSelectionTime( 0 ) == 60 );
    CATCH_REQUIRE( rca.getBoosterRoundSelectionTime( 1 ) == 60 );
    CATCH_REQUIRE( rca.getBoosterRoundSelectionTime( 2 ) == 60 );

    CATCH_REQUIRE( rca.isBoosterDraft() );
    CATCH_REQUIRE_FALSE( rca.isSealedDraft() );
    CATCH_REQUIRE_FALSE( rca.isGridDraft() );

    auto setCodes = rca.getSetCodes();
    CATCH_REQUIRE( setCodes.size() == 3 );
    CATCH_REQUIRE( setCodes[0] == "10E" );
    CATCH_REQUIRE( setCodes[1] == "3ED" );
    CATCH_REQUIRE( setCodes[2] == "10E" );
}


CATCH_TEST_CASE( "RoomConfigAdapter - Simple Sealed Config", "[roomconfigadapter]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigadapter" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    //
    // Create a RoomConfig
    //

    const int CHAIR_COUNT = 4;
    RoomConfig roomConfig;
    roomConfig.set_name( "testroom" );
    roomConfig.set_password_protected( true );
    roomConfig.set_bot_count( 2 );

    DraftConfig* draftConfig = roomConfig.mutable_draft_config();
    draftConfig->set_chair_count( CHAIR_COUNT );

    //
    // Hardcode for one sealed rounds with three dispensers and
    // six dispensations, 10E/3ED/cube/10E/3ED/cube.
    //

    DraftConfig::CardDispenser* dispenser;
    dispenser = draftConfig->add_dispensers();
    dispenser->add_source_booster_set_codes( "10E" );

    dispenser = draftConfig->add_dispensers();
    dispenser->add_source_booster_set_codes( "3ED" );

    // (Custom card list not actually being initialized, not required for test)
    dispenser = draftConfig->add_dispensers();
    dispenser->set_source_custom_card_list_index( 0 );

    DraftConfig::Round* round = draftConfig->add_rounds();
    DraftConfig::SealedRound* sealedRound = round->mutable_sealed_round();
    for( int d = 0; d < 6; ++d )
    {
        DraftConfig::CardDispensation* dispensation = sealedRound->add_dispensations();
        dispensation->set_dispenser_index( d % 3 );
        for( int i = 0; i < CHAIR_COUNT; ++i )
        {
            dispensation->add_chair_indices( i );
        }
    }

    //
    // Tests
    //

    RoomConfigAdapter rca( 0, roomConfig, loggingConfig );
    CATCH_REQUIRE( rca.getRoomId() == 0 );
    CATCH_REQUIRE( rca.getName() == "testroom" );
    CATCH_REQUIRE( rca.getChairCount() == 4 );
    CATCH_REQUIRE( rca.getBotCount() == 2 );
    CATCH_REQUIRE( rca.isPasswordProtected() == true );

    CATCH_REQUIRE( rca.getDraftConfig().rounds_size() == 1 );

    CATCH_REQUIRE_FALSE( rca.isBoosterDraft() );
    CATCH_REQUIRE( rca.isSealedDraft() );
    CATCH_REQUIRE_FALSE( rca.isGridDraft() );

    CATCH_REQUIRE( rca.getPassDirection( 0 ) == PASS_DIRECTION_NONE );

    auto setCodes = rca.getSetCodes();
    CATCH_REQUIRE( setCodes.size() == 6 );
    CATCH_REQUIRE( setCodes[0] == "10E" );
    CATCH_REQUIRE( setCodes[1] == "3ED" );
    CATCH_REQUIRE( setCodes[2] == "\u00B3" );
    CATCH_REQUIRE( setCodes[3] == "10E" );
    CATCH_REQUIRE( setCodes[4] == "3ED" );
    CATCH_REQUIRE( setCodes[5] == "\u00B3" );
}


CATCH_TEST_CASE( "RoomConfigAdapter - Simple Grid Config", "[roomconfigadapter]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigadapter" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    //
    // Create a RoomConfig
    //

    const int CHAIR_COUNT = 2;
    RoomConfig roomConfig;
    roomConfig.set_name( "testroom" );
    roomConfig.set_password_protected( true );
    roomConfig.set_bot_count( 1 );

    DraftConfig* draftConfig = roomConfig.mutable_draft_config();
    draftConfig->set_chair_count( CHAIR_COUNT );

    //
    // Hardcode for 18 grid rounds with one cube dispenser
    //

    DraftConfig::CardDispenser* dispenser;

    // (Custom card list not actually being initialized, not required for test)
    dispenser = draftConfig->add_dispensers();
    dispenser->set_source_custom_card_list_index( 0 );

    for( int r = 0; r < 18; ++r )
    {
        DraftConfig::Round* round = draftConfig->add_rounds();
        DraftConfig::GridRound* gridRound = round->mutable_grid_round();
        gridRound->set_dispenser_index( 0 );
    }

    //
    // Tests
    //

    RoomConfigAdapter rca( 0, roomConfig, loggingConfig );
    CATCH_REQUIRE( rca.getRoomId() == 0 );
    CATCH_REQUIRE( rca.getName() == "testroom" );
    CATCH_REQUIRE( rca.getChairCount() == 2 );
    CATCH_REQUIRE( rca.getBotCount() == 1 );
    CATCH_REQUIRE( rca.isPasswordProtected() == true );

    CATCH_REQUIRE( rca.getDraftConfig().rounds_size() == 18 );

    CATCH_REQUIRE_FALSE( rca.isBoosterDraft() );
    CATCH_REQUIRE_FALSE( rca.isSealedDraft() );
    CATCH_REQUIRE( rca.isGridDraft() );

    CATCH_REQUIRE( rca.getPassDirection( 0 ) == PASS_DIRECTION_NONE );

    auto setCodes = rca.getSetCodes();
    CATCH_REQUIRE( setCodes.size() == 18 );
    for( int i = 0; i < 18; ++i )
    {
        CATCH_REQUIRE( setCodes[i] == "\u00B3" );
    }
}

