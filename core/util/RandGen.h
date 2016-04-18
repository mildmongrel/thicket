#ifndef RANDGEN_H
#define RANDGEN_H

// Simple RNG interface.
class RandGen
{
public:

    virtual int generateInRange( int min, int max ) = 0;
    virtual float generateCanonical() = 0;
};

#endif
