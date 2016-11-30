#include "catch.hpp"
#include "StringUtil.h"

using namespace StringUtil;

CATCH_TEST_CASE( "icompare", "[stringutil]" )
{
    CATCH_REQUIRE( icompare( "test", "test" ) );
    CATCH_REQUIRE( icompare( "Test", "Test" ) );
    CATCH_REQUIRE( icompare( "TEST", "TEST" ) );

    CATCH_REQUIRE( icompare( "test", "Test" ) );
    CATCH_REQUIRE( icompare( "Test", "TEST" ) );
    CATCH_REQUIRE( icompare( "TEST", "test" ) );

    CATCH_REQUIRE_FALSE( icompare( "test", "test1234" ) );
    CATCH_REQUIRE_FALSE( icompare( "test", "Test1234" ) );
    CATCH_REQUIRE_FALSE( icompare( "test", "TEST1234" ) );

    CATCH_REQUIRE_FALSE( icompare( "test", "1234" ) );
    CATCH_REQUIRE_FALSE( icompare( "Test", "1234" ) );
    CATCH_REQUIRE_FALSE( icompare( "TEST", "1234" ) );
}

CATCH_TEST_CASE( "trim", "[stringutil]" )
{
    CATCH_REQUIRE( trim( "" ) == "" );
    CATCH_REQUIRE( trim( "TEST" ) == "TEST" );
    CATCH_REQUIRE( trim( "  TEST" ) == "TEST" );
    CATCH_REQUIRE( trim( "TEST  " ) == "TEST" );
    CATCH_REQUIRE( trim( "  TEST  " ) == "TEST" );
}
