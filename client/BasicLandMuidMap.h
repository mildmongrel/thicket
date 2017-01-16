#ifndef BASICLANDMUIDMAP_H
#define BASICLANDMUIDMAP_H

#include <map>

#include "BasicLand.h"

class BasicLandMuidMap
{
public:

    typedef std::map<BasicLandType,int> MapType;

    BasicLandMuidMap() {}

    BasicLandMuidMap( std::initializer_list<MapType::value_type> init )
      : mMap( init ) {}

    // Return zero for any unset basic land type.
    int getMuid( BasicLandType land ) const
    {
        return (mMap.count( land ) > 0) ? mMap.at( land ) : 0;
    }

    void setMuid( BasicLandType land, int muid )
    {
        mMap[land] = muid;
    }

    bool operator==( const BasicLandMuidMap& that ) const
    {
        return (this->mMap == that.mMap);
    }
    bool operator!=( const BasicLandMuidMap& that ) const
    {
        return !(*this == that);
    }

private:

    MapType mMap;
};

#endif
