#include "PlayerInventory.h"

#include <algorithm>

const std::array<PlayerInventory::ZoneType,PlayerInventory::ZONE_TYPE_COUNT>
PlayerInventory::gZoneTypeArray = { PlayerInventory::ZONE_MAIN,
                                    PlayerInventory::ZONE_SIDEBOARD,
                                    PlayerInventory::ZONE_JUNK };


std::string
stringify( const PlayerInventory::ZoneType& zone )
{
    switch( zone )
    {
        case PlayerInventory::ZONE_MAIN:      return "Main";
        case PlayerInventory::ZONE_SIDEBOARD: return "Sideboard";
        case PlayerInventory::ZONE_JUNK:      return "Junk";
        default:                              return std::string();
    }
}


bool
PlayerInventory::adjustBasicLand( BasicLandType basic,
                                  ZoneType      zone,
                                  int           adj )
{
    if( basic > BASIC_LAND_TYPE_COUNT ) return false;
    if( zone > ZONE_TYPE_COUNT ) return false;

    int origQty = mBasicLandQtys[zone].getQuantity( basic );
    int newQty = origQty + adj;

    if( newQty < 0 ) return false;

    mBasicLandQtys[zone].setQuantity( basic, newQty );
    return true;
}


bool
PlayerInventory::add( const std::shared_ptr<CardData>& card,
                      ZoneType                         zone )
{
    if( zone > ZONE_TYPE_COUNT ) return false;
    mCardData[zone].push_back( card );
    return true;
}


bool
PlayerInventory::move( const std::shared_ptr<CardData>& card,
                       ZoneType                         zoneFrom,
                       ZoneType                         zoneTo )
{
    if( zoneFrom > ZONE_TYPE_COUNT ) return false;
    if( zoneTo > ZONE_TYPE_COUNT ) return false;

    auto iter = std::find_if( mCardData[zoneFrom].begin(),
            mCardData[zoneFrom].end(),
            [&]( const CardDataSharedPtr& c ) { return *c == *card; } );

    if( iter == mCardData[zoneFrom].end() ) return false;

    mCardData[zoneTo].push_back( *iter );
    mCardData[zoneFrom].erase( iter );

    return true;
}


unsigned int
PlayerInventory::size() const
{
    unsigned int sum = 0;
    for( unsigned int i = 0; i < ZONE_TYPE_COUNT; ++i )
    {
        sum += mCardData[i].size();
        sum += mBasicLandQtys[i].getTotalQuantity();
    }
    return sum;
}


unsigned int
PlayerInventory::size( ZoneType zone ) const
{
    if( zone > ZONE_TYPE_COUNT ) return false;
    return mCardData[zone].size() + mBasicLandQtys[zone].getTotalQuantity();
}


BasicLandQuantities
PlayerInventory::getBasicLandQuantities( ZoneType zone ) const
{
    if( zone > ZONE_TYPE_COUNT ) return BasicLandQuantities();
    return mBasicLandQtys[zone];
}


std::vector<PlayerInventory::CardDataSharedPtr>
PlayerInventory::getCards( ZoneType zone ) const
{
    if( zone > ZONE_TYPE_COUNT ) return std::vector<CardDataSharedPtr>();
    return mCardData[zone];
}


void
PlayerInventory::clear()
{
    for( unsigned int i = 0; i < ZONE_TYPE_COUNT; ++i )
    {
        mCardData[i].clear();
        mBasicLandQtys[i].clear();
    }
}
