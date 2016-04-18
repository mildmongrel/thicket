#ifndef SIMPLERANDGEN_H
#define SIMPLERANDGEN_H

#include "RandGen.h"
#include <random>

//
// std::random_device works great on Unix (/dev/urandom) and maybe other
// platforms, but on Windows using MinGW it's a Pseudo-RNG that can
// return the same value on subsequent calls.  Detecting the nature of
// std::random_device isn't well supported.
//
// This simple generator uses a time-based approach to seeding and should
// be fine for purposes of drafting: creating boosters, auto-picking, etc.
//

class SimpleRandGen : public RandGen
{
public:

    SimpleRandGen();

    virtual int generateInRange( int min, int max );
    virtual float generateCanonical();

private:
    std::mt19937 mRandEng;
};


#endif
