#include "Decklist.h"
#include <sstream>


void
Decklist::addCard( std::string cardName, Decklist::ZoneType zone, uint16_t qty )
{
    if( qty == 0 ) return;

    std::map<std::string,uint16_t>& cardQtyMap =
            (zone == ZONE_MAIN) ? mCardQtyMainMap : mCardQtySideboardMap;
    cardQtyMap[cardName] += qty;
}


std::string
Decklist::getFormattedString( Decklist::FormatType format ) const
{
    std::stringstream ss;

    // Priority main cards.
    for( auto& kv : mCardQtyMainMap )
    {
        if( mPriorityCardNames.count( kv.first ) > 0 )
        {
            ss << kv.second << " " << kv.first << std::endl;
        }
    }

    // Non-priority main cards.
    for( auto& kv : mCardQtyMainMap )
    {
        if( mPriorityCardNames.count( kv.first ) == 0 )
        {
            ss << kv.second << " " << kv.first << std::endl;
        }
    }

    ss << std::endl;

    // Priority sideboard cards.
    for( auto& kv : mCardQtySideboardMap )
    {
        if( mPriorityCardNames.count( kv.first ) > 0 )
        {
            ss << "SB: " << kv.second << " " << kv.first << std::endl;
        }
    }

    // Non-priority sideboard cards.
    for( auto& kv : mCardQtySideboardMap )
    {
        if( mPriorityCardNames.count( kv.first ) == 0 )
        {
            ss << "SB: " << kv.second << " " << kv.first << std::endl;
        }
    }

    return ss.str();
}

