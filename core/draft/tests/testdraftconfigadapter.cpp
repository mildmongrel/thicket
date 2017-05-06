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

    for( int r = 0; r < dc.rounds_size(); ++r )
    {
        CATCH_REQUIRE( adapter.isBoosterRound( r ) );
        CATCH_REQUIRE_FALSE( adapter.isSealedRound( r ) );
        CATCH_REQUIRE_FALSE( adapter.isGridRound( r ) );

        CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( r ) == 30 );
    }

    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 0 ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 1 ) == proto::DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( 2 ) == proto::DraftConfig::DIRECTION_CLOCKWISE );

    // past-the-end checks
    CATCH_REQUIRE_FALSE( adapter.isBoosterRound( dc.rounds_size() ) );
    CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( dc.rounds_size() ) == 0 );
    CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( dc.rounds_size() ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
}


CATCH_TEST_CASE( "Draft config adapter: simple sealed", "[draft][draftconfigadapter]" )
{
    const int NUM_PLAYERS = 8;

    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS );
    DraftConfigAdapter adapter( dc );

    for( int r = 0; r < dc.rounds_size(); ++r )
    {
        CATCH_REQUIRE( adapter.isSealedRound( r ) );
        CATCH_REQUIRE_FALSE( adapter.isBoosterRound( r ) );
        CATCH_REQUIRE_FALSE( adapter.isGridRound( r ) );

        // should return default values for non-booster
        CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( r ) == 0 );
        CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( r ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
    }

    // past-the-end check
    CATCH_REQUIRE_FALSE( adapter.isSealedRound( dc.rounds_size() ) );
}


CATCH_TEST_CASE( "Draft config adapter: simple grid", "[draft][draftconfigadapter]" )
{
    DraftConfig dc = TestDefaults::getSimpleGridDraftConfig();
    DraftConfigAdapter adapter( dc );

    for( int r = 0; r < dc.rounds_size(); ++r )
    {
        CATCH_REQUIRE( adapter.isGridRound( r ) );
        CATCH_REQUIRE_FALSE( adapter.isBoosterRound( r ) );
        CATCH_REQUIRE_FALSE( adapter.isSealedRound( r ) );

        // should return default values for non-booster
        CATCH_REQUIRE( adapter.getBoosterRoundSelectionTime( r ) == 0 );
        CATCH_REQUIRE( adapter.getBoosterRoundPassDirection( r ) == proto::DraftConfig::DIRECTION_CLOCKWISE );
    }

    // past-the-end check
    CATCH_REQUIRE_FALSE( adapter.isGridRound( dc.rounds_size() ) );
}
