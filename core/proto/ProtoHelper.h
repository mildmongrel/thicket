#ifndef COMMONPROTOHELPER_H
#define COMMONPROTOHELPER_H

#include "messages.pb.h"
#include "PlayerInventory.h"
#include "BasicLand.h"


inline std::string
stringify( const proto::Zone& zone )
{
    switch( zone )
    {
        case proto::ZONE_MAIN:      return "Main";
        case proto::ZONE_SIDEBOARD: return "Sideboard";
        case proto::ZONE_JUNK:      return "Junk";
        case proto::ZONE_AUTO:      return "Auto";
        default:                      return std::string();
    }
}


inline std::string
stringify( const proto::BasicLand& basic )
{
    switch( basic )
    {
        case proto::BASIC_LAND_PLAINS:   return "Plains";
        case proto::BASIC_LAND_ISLAND:   return "Island";
        case proto::BASIC_LAND_SWAMP:    return "Swamp";
        case proto::BASIC_LAND_MOUNTAIN: return "Mountain";
        case proto::BASIC_LAND_FOREST:   return "Forest";
        default:                           return std::string();
    }
}


//
// For these conversion functions, ignore return-type warnings.  Returns
// happen from within each case of a switch statement; there should not
// be a default value that is returned if a case isn't hit.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"


inline proto::Zone convertZone( const PlayerInventory::ZoneType& z )
{
    switch( z )
    {
        case PlayerInventory::ZONE_MAIN:      return proto::ZONE_MAIN;
        case PlayerInventory::ZONE_SIDEBOARD: return proto::ZONE_SIDEBOARD;
        case PlayerInventory::ZONE_JUNK:      return proto::ZONE_JUNK;
        case PlayerInventory::ZONE_AUTO:      return proto::ZONE_AUTO;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline PlayerInventory::ZoneType convertZone( const proto::Zone& z )
{
    switch( z )
    {
        case proto::ZONE_MAIN:      return PlayerInventory::ZONE_MAIN;
        case proto::ZONE_SIDEBOARD: return PlayerInventory::ZONE_SIDEBOARD;
        case proto::ZONE_JUNK:      return PlayerInventory::ZONE_JUNK;
        case proto::ZONE_AUTO:      return PlayerInventory::ZONE_AUTO;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline proto::BasicLand convertBasicLand( const BasicLandType& basicLand )
{
    switch( basicLand )
    {
        case BASIC_LAND_PLAINS:    return proto::BASIC_LAND_PLAINS;
        case BASIC_LAND_ISLAND:    return proto::BASIC_LAND_ISLAND;
        case BASIC_LAND_SWAMP:     return proto::BASIC_LAND_SWAMP;
        case BASIC_LAND_MOUNTAIN:  return proto::BASIC_LAND_MOUNTAIN;
        case BASIC_LAND_FOREST:    return proto::BASIC_LAND_FOREST;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline BasicLandType convertBasicLand( const proto::BasicLand& basicLand )
{
    switch( basicLand )
    {
        case proto::BASIC_LAND_PLAINS:    return BASIC_LAND_PLAINS;
        case proto::BASIC_LAND_ISLAND:    return BASIC_LAND_ISLAND;
        case proto::BASIC_LAND_SWAMP:     return BASIC_LAND_SWAMP;
        case proto::BASIC_LAND_MOUNTAIN:  return BASIC_LAND_MOUNTAIN;
        case proto::BASIC_LAND_FOREST:    return BASIC_LAND_FOREST;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


#pragma GCC diagnostic pop


#endif
