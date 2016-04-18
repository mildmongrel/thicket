#ifndef BASICLAND_H
#define BASICLAND_H

#include <string>

// Basic lands in WUBRG order.
enum BasicLandType
{
    BASIC_LAND_PLAINS,
    BASIC_LAND_ISLAND,
    BASIC_LAND_SWAMP,
    BASIC_LAND_MOUNTAIN,
    BASIC_LAND_FOREST
};

const int BASIC_LAND_TYPE_COUNT = 5;
const std::array<BasicLandType,BASIC_LAND_TYPE_COUNT> gBasicLandTypeArray = {
    BASIC_LAND_PLAINS, BASIC_LAND_ISLAND, BASIC_LAND_SWAMP, BASIC_LAND_MOUNTAIN, BASIC_LAND_FOREST };

inline std::string
stringify( const BasicLandType& land )
{
    switch( land )
    {
        case BASIC_LAND_PLAINS:   return "Plains";
        case BASIC_LAND_ISLAND:   return "Island";
        case BASIC_LAND_SWAMP:    return "Swamp";
        case BASIC_LAND_MOUNTAIN: return "Mountain";
        case BASIC_LAND_FOREST:   return "Forest";
        default:                  return std::string();
    }
}

#endif
