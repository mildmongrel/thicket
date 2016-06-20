#include "catch.hpp"
#include "SimpleVersion.h"

CATCH_TEST_CASE( "SimpleVersion", "[simpleversion]" )
{
    SimpleVersion v0_0( 0, 0 );
    SimpleVersion v1_0( 1, 0 );
    SimpleVersion v1_1( 1, 1 );

    CATCH_REQUIRE( v0_0.getMajor() == v0_0.getMajor() );
    CATCH_REQUIRE( v0_0.getMajor() < v1_0.getMajor() );
    CATCH_REQUIRE( v0_0.getMajor() < v1_1.getMajor() );

    CATCH_REQUIRE_FALSE( v0_0.olderThan( v0_0 ) );
    CATCH_REQUIRE( v0_0.olderThan( v1_0 ) );
    CATCH_REQUIRE( v0_0.olderThan( v1_1 ) );

    CATCH_REQUIRE( v1_0.getMajor() > v0_0.getMajor() );
    CATCH_REQUIRE( v1_0.getMajor() == v1_0.getMajor() );
    CATCH_REQUIRE( v1_0.getMajor() == v1_1.getMajor() );
    CATCH_REQUIRE( v1_0.getMinor() == v1_0.getMinor() );
    CATCH_REQUIRE( v1_0.getMinor() < v1_1.getMinor() );

    CATCH_REQUIRE_FALSE( v1_0.olderThan( v0_0 ) );
    CATCH_REQUIRE_FALSE( v1_0.olderThan( v1_0 ) );
    CATCH_REQUIRE( v1_0.olderThan( v1_1 ) );

    CATCH_REQUIRE( v1_1.getMajor() > v0_0.getMajor() );
    CATCH_REQUIRE( v1_1.getMajor() == v1_0.getMajor() );
    CATCH_REQUIRE( v1_1.getMajor() == v1_1.getMajor() );
    CATCH_REQUIRE( v1_1.getMinor() > v1_0.getMinor() );
    CATCH_REQUIRE( v1_1.getMinor() == v1_1.getMinor() );

    CATCH_REQUIRE_FALSE( v1_1.olderThan( v0_0 ) );
    CATCH_REQUIRE_FALSE( v1_1.olderThan( v1_0 ) );
    CATCH_REQUIRE_FALSE( v1_1.olderThan( v1_1 ) );
}
