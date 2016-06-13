#ifndef PLAYERINVENTORY_H
#define PLAYERINVENTORY_H

#include <memory>
#include <vector>
#include "CardData.h"
#include "BasicLandQuantities.h"

class PlayerInventory
{
public:

    typedef std::shared_ptr<CardData> CardDataSharedPtr;

    enum ZoneType
    {
        ZONE_AUTO,
        ZONE_MAIN,
        ZONE_SIDEBOARD,
        ZONE_JUNK
    };

    const static unsigned int ZONE_TYPE_COUNT = 4;
    const static std::array<ZoneType,PlayerInventory::ZONE_TYPE_COUNT> gZoneTypeArray;

    // Adjust basic land quantities.
    bool adjustBasicLand( BasicLandType basic, ZoneType zone, int adj = 1 );

    // Add a card.
    bool add( const std::shared_ptr<CardData>& card, ZoneType zone );

    // Move a card.
    bool move( const std::shared_ptr<CardData>& card, ZoneType zoneFrom, ZoneType zoneTo );

    BasicLandQuantities getBasicLandQuantities( ZoneType zone ) const;
    std::vector<CardDataSharedPtr> getCards( ZoneType zone ) const;

    unsigned int size() const;
    unsigned int size( ZoneType zone ) const;

    void clear();

private:

    std::vector<CardDataSharedPtr> mCardData[ZONE_TYPE_COUNT];
    BasicLandQuantities mBasicLandQtys[ZONE_TYPE_COUNT];

};

std::string stringify( const PlayerInventory::ZoneType& zone );

#endif // PLAYERINVENTORY_H
