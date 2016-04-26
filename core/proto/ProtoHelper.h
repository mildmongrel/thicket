#ifndef COMMONPROTOHELPER_H
#define COMMONPROTOHELPER_H

#include "messages.pb.h"
#include "PlayerInventory.h"
#include "BasicLand.h"

inline std::string
stringify( const thicket::Zone& zone )
{
    switch( zone )
    {
        case thicket::ZONE_MAIN:      return "Main";
        case thicket::ZONE_SIDEBOARD: return "Sideboard";
        case thicket::ZONE_JUNK:      return "Junk";
        default:                      return std::string();
    }
}


inline std::string
stringify( const thicket::BasicLand& basic )
{
    switch( basic )
    {
        case thicket::BASIC_LAND_PLAINS:   return "Plains";
        case thicket::BASIC_LAND_ISLAND:   return "Island";
        case thicket::BASIC_LAND_SWAMP:    return "Swamp";
        case thicket::BASIC_LAND_MOUNTAIN: return "Mountain";
        case thicket::BASIC_LAND_FOREST:   return "Forest";
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


inline thicket::Zone convertZone( const PlayerInventory::ZoneType& z )
{
    switch( z )
    {
        case PlayerInventory::ZONE_MAIN:      return thicket::ZONE_MAIN;
        case PlayerInventory::ZONE_SIDEBOARD: return thicket::ZONE_SIDEBOARD;
        case PlayerInventory::ZONE_JUNK:      return thicket::ZONE_JUNK;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline PlayerInventory::ZoneType convertZone( const thicket::Zone& z )
{
    switch( z )
    {
        case thicket::ZONE_MAIN:      return PlayerInventory::ZONE_MAIN;
        case thicket::ZONE_SIDEBOARD: return PlayerInventory::ZONE_SIDEBOARD;
        case thicket::ZONE_JUNK:      return PlayerInventory::ZONE_JUNK;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline thicket::BasicLand convertBasicLand( const BasicLandType& basicLand )
{
    switch( basicLand )
    {
        case BASIC_LAND_PLAINS:    return thicket::BASIC_LAND_PLAINS;
        case BASIC_LAND_ISLAND:    return thicket::BASIC_LAND_ISLAND;
        case BASIC_LAND_SWAMP:     return thicket::BASIC_LAND_SWAMP;
        case BASIC_LAND_MOUNTAIN:  return thicket::BASIC_LAND_MOUNTAIN;
        case BASIC_LAND_FOREST:    return thicket::BASIC_LAND_FOREST;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline BasicLandType convertBasicLand( const thicket::BasicLand& basicLand )
{
    switch( basicLand )
    {
        case thicket::BASIC_LAND_PLAINS:    return BASIC_LAND_PLAINS;
        case thicket::BASIC_LAND_ISLAND:    return BASIC_LAND_ISLAND;
        case thicket::BASIC_LAND_SWAMP:     return BASIC_LAND_SWAMP;
        case thicket::BASIC_LAND_MOUNTAIN:  return BASIC_LAND_MOUNTAIN;
        case thicket::BASIC_LAND_FOREST:    return BASIC_LAND_FOREST;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


#pragma GCC diagnostic pop


#endif
