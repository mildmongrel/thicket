#ifndef BASICLANDCARDDATAMAP_H
#define BASICLANDCARDDATAMAP_H

#include <map>
#include <memory>

#include "clienttypes.h"
#include "BasicLand.h"
#include "SimpleCardData.h"

class BasicLandCardDataMap
{
public:
    BasicLandCardDataMap()
    {
        // This will initialize all card data to safe values.
        for( auto land : gBasicLandTypeArray )
        {
            setCardData( land, nullptr );
        }
    }
    CardDataSharedPtr getCardData( BasicLandType land ) { return mCardDataMap[land]; }
    void setCardData( BasicLandType land, const CardDataSharedPtr& cardData )
    {
        mCardDataMap[land] = cardData ? cardData : std::shared_ptr<CardData>( new SimpleCardData( stringify(land) ) );
    }
private:
    std::map<BasicLandType,CardDataSharedPtr> mCardDataMap;
};

#endif
