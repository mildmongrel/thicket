#ifndef CARDDATATYPES_H
#define CARDDATATYPES_H

#include <string>
#include <array>

// Colors in WUBRG order.
enum ColorType
{
    COLOR_WHITE,
    COLOR_BLUE,
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN
};

const std::array<ColorType,5> gColorTypeArray = {
    COLOR_WHITE, COLOR_BLUE, COLOR_BLACK, COLOR_RED, COLOR_GREEN };

inline std::string stringify( const ColorType& color )
{
    switch( color )
    {
        case COLOR_WHITE:  return "White";
        case COLOR_BLUE:   return "Blue";
        case COLOR_BLACK:  return "Black";
        case COLOR_RED:    return "Red";
        case COLOR_GREEN:  return "Green";
        default:           return std::string();
    }
}

enum RarityType
{
    RARITY_BASIC_LAND,
    RARITY_COMMON,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_MYTHIC_RARE,
    RARITY_SPECIAL,
    RARITY_UNKNOWN
};

const std::array<RarityType,7> gRarityTypeArray = {
    RARITY_BASIC_LAND, RARITY_COMMON, RARITY_UNCOMMON, RARITY_RARE, RARITY_MYTHIC_RARE, RARITY_SPECIAL, RARITY_UNKNOWN };

inline std::string stringify( const RarityType& rarity )
{
    switch( rarity )
    {
        case RARITY_BASIC_LAND:  return "Basic Land";
        case RARITY_COMMON:      return "Common";
        case RARITY_UNCOMMON:    return "Uncommon";
        case RARITY_RARE:        return "Rare";
        case RARITY_MYTHIC_RARE: return "Mythic Rare";
        case RARITY_SPECIAL:     return "Special";
        case RARITY_UNKNOWN:     return "Unknown";
        default:                 return std::string();
    }
}

#endif  // CARDDATATYPES_H
