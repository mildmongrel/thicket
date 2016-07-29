#include "catch.hpp"
#include "DraftConfigAdapter.h"
#include "testdefaults.h"

using proto::DraftConfig;

CATCH_TEST_CASE( "Draft config adapter: simple booster", "[draft][draftconfigadapter]" )
{
    const int NUM_ROUNDS  = 3;
    const int NUM_PLAYERS = 8;

    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( NUM_ROUNDS, NUM_PLAYERS, 30 );
    DraftConfigAdapter adapter( dc );

    CATCH_REQUIRE( adapter.isBoosterRound( 0 ) );
    CATCH_REQUIRE( adapter.isBoosterRound( 1 ) );
    CATCH_REQUIRE( adapter.isBoosterRound( 2 ) );
    CATCH_REQUIRE_FALSE( adapter.isBoosterRound( 3 ) );

    CATCH_REQUIRE_FALSE( adapter.isSealedRound( 0 ) );
    CATCH_REQUIRE_FALSE( adapter.isSealedRound( 1 ) );
    CATCH_REQUIRE_FALSE( adapter.isSealedRound( 2 ) );
    CATCH_REQUIRE_FALSE( adapter.isSealedRound( 3 ) );

    CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( 0 ) == 30 );
    CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( 1 ) == 30 );
    CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( 2 ) == 30 );
    CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( 3 ) == 0 );

    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 0 ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 1 ) == proto::DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 2 ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 3 ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
}

CATCH_TEST_CASE( "Draft config adapter: simple sealed", "[draft][draftconfigadapter]" )
{
    const int NUM_PLAYERS = 8;

    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS );
    DraftConfigAdapter adapter( dc );

    CATCH_REQUIRE( adapter.isSealedRound( 0 ) );
    CATCH_REQUIRE_FALSE( adapter.isSealedRound( 1 ) );

    CATCH_REQUIRE_FALSE( adapter.isBoosterRound( 0 ) );
    CATCH_REQUIRE_FALSE( adapter.isBoosterRound( 1 ) );

    // should return default values for non-booster
    CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( 0 ) == 0 );
    CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( 1 ) == 0 );

    // should return default values for non-booster
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 0 ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 1 ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
}
