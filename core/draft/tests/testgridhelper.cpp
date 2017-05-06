#include "catch.hpp"
#include "GridHelper.h"

CATCH_TEST_CASE( "Grid Helper - basics", "[draft][gridhelper]" )
{
    GridHelper::IndexSet unavailableIndices = {};
    GridHelper gh;

    CATCH_REQUIRE( gh.getSliceCount() == 6 );

    CATCH_REQUIRE( gh.isSliceRow( 0 ) );
    CATCH_REQUIRE( gh.isSliceRow( 1 ) );
    CATCH_REQUIRE( gh.isSliceRow( 2 ) );
    CATCH_REQUIRE_FALSE( gh.isSliceRow( 3 ) );
    CATCH_REQUIRE_FALSE( gh.isSliceRow( 4 ) );
    CATCH_REQUIRE_FALSE( gh.isSliceRow( 5 ) );
    CATCH_REQUIRE_FALSE( gh.isSliceRow( 6 ) );

    CATCH_REQUIRE_FALSE( gh.isSliceCol( 0 ) );
    CATCH_REQUIRE_FALSE( gh.isSliceCol( 1 ) );
    CATCH_REQUIRE_FALSE( gh.isSliceCol( 2 ) );
    CATCH_REQUIRE( gh.isSliceCol( 3 ) );
    CATCH_REQUIRE( gh.isSliceCol( 4 ) );
    CATCH_REQUIRE( gh.isSliceCol( 5 ) );
    CATCH_REQUIRE_FALSE( gh.isSliceCol( 6 ) );
}


CATCH_TEST_CASE( "Grid Helper - available selections within fully available grid", "[draft][gridhelper]" )
{
    GridHelper gh;

    const std::map<GridHelper::IndexSet,unsigned int> expected = { { { 0, 1, 2 }, 0 },
                                                                   { { 3, 4, 5 }, 1 },
                                                                   { { 6, 7, 8 }, 2 },
                                                                   { { 0, 3, 6 }, 3 },
                                                                   { { 1, 4, 7 }, 4 },
                                                                   { { 2, 5, 8 }, 5 },
                                                                 };

    auto availableSelectionsMap = gh.getAvailableSelectionsMap( {} );

    CATCH_REQUIRE( availableSelectionsMap == expected );
}

CATCH_TEST_CASE( "Grid Helper - available selections with slices unavailable", "[draft][gridhelper]" )
{
    GridHelper gh;
    std::map<GridHelper::IndexSet,unsigned int> expected;

    CATCH_SECTION( "row 0" )
    {
        expected = { { { 3, 4, 5 }, 1 },
                     { { 6, 7, 8 }, 2 },
                     {    { 3, 6 }, 3 },
                     {    { 4, 7 }, 4 },
                     {    { 5, 8 }, 5 },
                   };

        auto availableSelectionsMap = gh.getAvailableSelectionsMap( { 0, 1, 2 } );

        CATCH_REQUIRE( availableSelectionsMap == expected );
    }

    CATCH_SECTION( "row 1" )
    {
        expected = { { { 0, 1, 2 }, 0 },
                     { { 6, 7, 8 }, 2 },
                     {    { 0, 6 }, 3 },
                     {    { 1, 7 }, 4 },
                     {    { 2, 8 }, 5 },
                   };

        auto availableSelectionsMap = gh.getAvailableSelectionsMap( { 3, 4, 5 } );

        CATCH_REQUIRE( availableSelectionsMap == expected );
    }

    CATCH_SECTION( "row 2" )
    {
        expected = { { { 0, 1, 2 }, 0 },
                     { { 3, 4, 5 }, 1 },
                     {    { 0, 3 }, 3 },
                     {    { 1, 4 }, 4 },
                     {    { 2, 5 }, 5 },
                   };

        auto availableSelectionsMap = gh.getAvailableSelectionsMap( { 6, 7, 8 } );

        CATCH_REQUIRE( availableSelectionsMap == expected );
    }

    CATCH_SECTION( "col 0" )
    {
        expected = { {    { 1, 2 }, 0 },
                     {    { 4, 5 }, 1 },
                     {    { 7, 8 }, 2 },
                     { { 1, 4, 7 }, 4 },
                     { { 2, 5, 8 }, 5 },
                   };

        auto availableSelectionsMap = gh.getAvailableSelectionsMap( { 0, 3, 6 } );

        CATCH_REQUIRE( availableSelectionsMap == expected );
    }

    CATCH_SECTION( "col 1" )
    {
        expected = { {    { 0, 2 }, 0 },
                     {    { 3, 5 }, 1 },
                     {    { 6, 8 }, 2 },
                     { { 0, 3, 6 }, 3 },
                     { { 2, 5, 8 }, 5 },
                   };

        auto availableSelectionsMap = gh.getAvailableSelectionsMap( { 1, 4, 7 } );

        CATCH_REQUIRE( availableSelectionsMap == expected );
    }

    CATCH_SECTION( "col 2" )
    {
        expected = { {    { 0, 1 }, 0 },
                     {    { 3, 4 }, 1 },
                     {    { 6, 7 }, 2 },
                     { { 0, 3, 6 }, 3 },
                     { { 1, 4, 7 }, 4 },
                   };

        auto availableSelectionsMap = gh.getAvailableSelectionsMap( { 2, 5, 8 } );

        CATCH_REQUIRE( availableSelectionsMap == expected );
    }
}

