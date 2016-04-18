#include "catch.hpp"
#include "RoomConfigAdapter.h"

static void
addBasicBoosterRound( thicket::RoomConfiguration& roomConfig )
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

static void
addBasicSealedRound( thicket::RoomConfiguration& roomConfig )
{
    thicket::RoomConfiguration::Round* round = roomConfig.add_rounds();
    thicket::RoomConfiguration::SealedRoundConfiguration* sealedRoundConfig = round->mutable_sealed_round_config();
    sealedRoundConfig->set_time( 600 );

    for( int i = 0; i < 6; ++i )
    {
        thicket::RoomConfiguration::CardBundle* bundle = sealedRoundConfig->add_card_bundles();
        bundle->set_set_code( "LEA" );
        bundle->set_method( bundle->METHOD_BOOSTER );
        bundle->set_set_replacement( true );
    }
}

CATCH_TEST_CASE( "RoomConfigAdapter - Basic Booster", "[roomconfigadapter]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigadapter" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    //
    // Create a model "Basic Booster Draft" RoomConfiguration that test cases can tweak.
    //

    thicket::RoomConfiguration roomConfig;
    roomConfig.set_name( "test" );
    roomConfig.set_password_protected( true );
    roomConfig.set_chair_count( 8 );
    roomConfig.set_bot_count( 4 );

    for( int i = 0; i < 3; ++i )
    {
        addBasicBoosterRound( roomConfig );
    }

    CATCH_REQUIRE( roomConfig.IsInitialized() );

    //
    // Test cases.
    //

    CATCH_SECTION( "Sunny Day" )
    {
        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );
        CATCH_REQUIRE( rca.getRoomId() == 0 );
        CATCH_REQUIRE( rca.getName() == "test" );
        CATCH_REQUIRE( rca.getChairCount() == 8 );
        CATCH_REQUIRE( rca.getBotCount() == 4 );
        CATCH_REQUIRE( rca.isPasswordProtected() == true );

        CATCH_REQUIRE( rca.getRoundCount() == 3 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.size() == 3 );
        CATCH_REQUIRE( setCodes[0] == "LEA" );
        CATCH_REQUIRE( setCodes[1] == "LEA" );
        CATCH_REQUIRE( setCodes[2] == "LEA" );

        CATCH_REQUIRE( rca.isRoundClockwise( 0 ) );
        CATCH_REQUIRE( rca.isRoundClockwise( 1 ) );
        CATCH_REQUIRE( rca.isRoundClockwise( 2 ) );
        CATCH_REQUIRE( rca.isRoundClockwise( 3 ) == false );

        CATCH_REQUIRE( rca.getRoundTime( 0 ) == 60 );
        CATCH_REQUIRE( rca.getRoundTime( 1 ) == 60 );
        CATCH_REQUIRE( rca.getRoundTime( 2 ) == 60 );
        CATCH_REQUIRE( rca.getRoundTime( 3 ) == 0 );
    }

    CATCH_SECTION( "Nonbasic: wrong card gen method" )
    {
        auto boosterRoundConfig = roomConfig.mutable_rounds( 2 )->mutable_booster_round_config();
        thicket::RoomConfiguration::CardBundle* bundle = boosterRoundConfig->mutable_card_bundles( 0 );
        bundle->set_method( bundle->METHOD_RANDOM );
        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );

        CATCH_REQUIRE( rca.getRoundCount() == 3 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.empty() );
    }

    CATCH_SECTION( "Nonbasic: too few rounds" )
    {
        roomConfig.clear_rounds();
        addBasicBoosterRound( roomConfig );
        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );

        CATCH_REQUIRE( rca.getRoundCount() == 1 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.empty() );
    }

    CATCH_SECTION( "Nonbasic: too many rounds" )
    {
        addBasicBoosterRound( roomConfig );
        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );

        CATCH_REQUIRE( rca.getRoundCount() == 4 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.empty() );
    }
}

CATCH_TEST_CASE( "RoomConfigAdapter - Basic Sealed", "[roomconfigadapter]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigadapter" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    //
    // Create a model "Basic Sealed Draft RoomConfiguration that test cases can tweak.
    //

    thicket::RoomConfiguration roomConfig;
    roomConfig.set_name( "test" );
    roomConfig.set_password_protected( true );
    roomConfig.set_chair_count( 8 );
    roomConfig.set_bot_count( 4 );

    addBasicSealedRound( roomConfig );

    CATCH_REQUIRE( roomConfig.IsInitialized() );

    //
    // Test cases.
    //

    CATCH_SECTION( "Sunny Day" )
    {
        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );
        CATCH_REQUIRE( rca.getRoomId() == 0 );
        CATCH_REQUIRE( rca.getName() == "test" );
        CATCH_REQUIRE( rca.getChairCount() == 8 );
        CATCH_REQUIRE( rca.getBotCount() == 4 );
        CATCH_REQUIRE( rca.isPasswordProtected() == true );

        CATCH_REQUIRE( rca.getRoundCount() == 1 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.size() == 6 );
        CATCH_REQUIRE( setCodes[0] == "LEA" );
        CATCH_REQUIRE( setCodes[1] == "LEA" );
        CATCH_REQUIRE( setCodes[2] == "LEA" );
        CATCH_REQUIRE( setCodes[3] == "LEA" );
        CATCH_REQUIRE( setCodes[4] == "LEA" );
        CATCH_REQUIRE( setCodes[5] == "LEA" );

        CATCH_REQUIRE( rca.getRoundTime( 0 ) == 600 );
        CATCH_REQUIRE( rca.getRoundTime( 3 ) == 0 );
    }

    CATCH_SECTION( "Nonbasic Round (nonbasic method)" )
    {
        auto sealedRoundConfig = roomConfig.mutable_rounds( 0 )->mutable_sealed_round_config();
        thicket::RoomConfiguration::CardBundle* bundle = sealedRoundConfig->mutable_card_bundles( 5 );
        bundle->set_method( bundle->METHOD_RANDOM );
        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );

        CATCH_REQUIRE( rca.getRoundCount() == 1 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.empty() );
    }

    CATCH_SECTION( "Nonbasic: too many rounds" )
    {
        addBasicSealedRound( roomConfig );
        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );

        CATCH_REQUIRE( rca.getRoundCount() == 2 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.empty() );
    }
}

CATCH_TEST_CASE( "RoomConfigAdapter - Mixed", "[roomconfigadapter]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "roomconfigadapter" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    //
    // Create an incomplete RoomConfiguration that test cases can finish off.
    //

    thicket::RoomConfiguration roomConfig;
    roomConfig.set_name( "test" );
    roomConfig.set_password_protected( true );
    roomConfig.set_chair_count( 8 );
    roomConfig.set_bot_count( 4 );

    //
    // Test cases.
    //

    CATCH_SECTION( "Almost basic booster draft" )
    {
        addBasicBoosterRound( roomConfig );
        addBasicBoosterRound( roomConfig );
        addBasicBoosterRound( roomConfig );
        addBasicSealedRound( roomConfig );

        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );
        CATCH_REQUIRE( rca.getRoomId() == 0 );
        CATCH_REQUIRE( rca.getName() == "test" );
        CATCH_REQUIRE( rca.getChairCount() == 8 );
        CATCH_REQUIRE( rca.getBotCount() == 4 );
        CATCH_REQUIRE( rca.isPasswordProtected() == true );

        CATCH_REQUIRE( rca.getRoundCount() == 4 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.empty() );

        CATCH_REQUIRE( rca.getRoundTime( 0 ) == 60 );
        CATCH_REQUIRE( rca.getRoundTime( 3 ) == 600 );
    }

    CATCH_SECTION( "Almost basic sealed draft" )
    {
        addBasicSealedRound( roomConfig );
        addBasicBoosterRound( roomConfig );
        addBasicBoosterRound( roomConfig );
        addBasicBoosterRound( roomConfig );

        RoomConfigAdapter rca( 0, roomConfig, loggingConfig );
        CATCH_REQUIRE( rca.getRoomId() == 0 );
        CATCH_REQUIRE( rca.getName() == "test" );
        CATCH_REQUIRE( rca.getChairCount() == 8 );
        CATCH_REQUIRE( rca.getBotCount() == 4 );
        CATCH_REQUIRE( rca.isPasswordProtected() == true );

        CATCH_REQUIRE( rca.getRoundCount() == 4 );

        auto setCodes = rca.getBasicSetCodes();
        CATCH_REQUIRE( setCodes.empty() );

        CATCH_REQUIRE( rca.getRoundTime( 0 ) == 600 );
        CATCH_REQUIRE( rca.getRoundTime( 3 ) == 60 );
    }
}
