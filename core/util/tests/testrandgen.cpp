#include "catch.hpp"
#include "SimpleRandGen.h"

CATCH_TEST_CASE( "SimpleRandGen generateInRange", "[randgen]" )
{
    const int BUCKET_COUNT = 10;
    const int ITERATIONS = 10000;
    const int EXPECTED_COUNT = (ITERATIONS / BUCKET_COUNT);
    const int LOWER_THRESHOLD = EXPECTED_COUNT - (EXPECTED_COUNT / 10);
    const int UPPER_THRESHOLD = EXPECTED_COUNT + (EXPECTED_COUNT / 10);

    int bucketsSameObj[BUCKET_COUNT] = {0};
    int bucketsDiffObj[BUCKET_COUNT] = {0};

    SimpleRandGen rng;
    for( int i = 0; i < ITERATIONS; ++i )
    {
        bucketsSameObj[ rng.generateInRange( 0, BUCKET_COUNT - 1 ) ]++;

        SimpleRandGen tempRng;
        bucketsDiffObj[ tempRng.generateInRange( 0, BUCKET_COUNT - 1 ) ]++;
    }

    for( int i = 0; i < BUCKET_COUNT; ++i )
    {
        CATCH_REQUIRE( bucketsSameObj[i] > LOWER_THRESHOLD );
        CATCH_REQUIRE( bucketsSameObj[i] < UPPER_THRESHOLD );

        CATCH_REQUIRE( bucketsDiffObj[i] > LOWER_THRESHOLD );
        CATCH_REQUIRE( bucketsDiffObj[i] < UPPER_THRESHOLD );
    }
}
