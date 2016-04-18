#include "SimpleRandGen.h"

#include <ctime>

SimpleRandGen::SimpleRandGen()
{
    // Instances of this class may be generated in rapid succession,
    // so using the time can easily produce identical RNGs.  Create
    // an initial seed from the system time, then roll it over the
    // course of the class' usage.
    // NOTE: This method was not effective for 'default_random_engine' -
    // it produced repetitive results across new instances of this class.
    // The 'mt19937' engine behaves much more reasonably with small
    // seed changes.
    static auto seed = std::time( nullptr );
    mRandEng.seed( seed++ );
}


int
SimpleRandGen::generateInRange( int min, int max )
{
    std::uniform_int_distribution<> dis( min, max );
    return dis( mRandEng );
}


float
SimpleRandGen::generateCanonical()
{
    return std::generate_canonical<float,std::numeric_limits<float>::digits>( mRandEng );
}

