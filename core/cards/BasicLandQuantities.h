#ifndef BASICLANDQUANTITIES_H
#define BASICLANDQUANTITIES_H

#include <map>
#include "BasicLand.h"

class BasicLandQuantities
{
public:

    int getQuantity( BasicLandType land ) const
    {
        const auto iter = mQtyMap.find( land );
        if( iter != mQtyMap.end() )
        {
            return iter->second;
        }
        else
        {
            return 0;
        }
    }

    void setQuantity( BasicLandType land, int qty ) { mQtyMap[land] = qty; }

    int getTotalQuantity() const
    {
        int total = 0;
        for( auto mapPair : mQtyMap )
        {
            total += mapPair.second;
        }
        return total;
    }

    void clear() { mQtyMap.clear(); }

    bool operator==( const BasicLandQuantities &other ) const
    {
        return mQtyMap == other.mQtyMap;
    }

private:
    std::map<BasicLandType,int> mQtyMap;
};

#endif
