#ifndef MTGJSON_H
#define MTGJSON_H

#include "CardDataTypes.h"
#include <string>
#include <vector>
#include "rapidjson/document.h"

namespace MtgJson
{
    inline RarityType getRarityFromString( const std::string& rarityStr )
    {
        if( rarityStr == "Basic Land" )
            return RARITY_BASIC_LAND;
        else if( rarityStr == "Common" )
            return RARITY_COMMON;
        else if( rarityStr == "Uncommon" )
            return RARITY_UNCOMMON;
        else if( rarityStr == "Rare" )
            return RARITY_RARE;
        else if( rarityStr == "Mythic Rare" )
            return RARITY_MYTHIC_RARE;
        else
            return RARITY_UNKNOWN;
    }

    inline std::string createSplitCardName( const rapidjson::Value& namesValue )
    {
        std::string name;
        if( namesValue.IsArray() && !namesValue.Empty() )
        {
            for( unsigned int i = 0; i < namesValue.Size(); ++i )
            {
                if( i != 0 ) name += " // ";
                if( namesValue[i].IsString() ) name += namesValue[i].GetString();
            }
        }
        return name;
    }

    // Normalize a split card name from a string with any number of slashes
    // and spaces.
    // Note: this formatting must match the creation output above.
    inline std::string normalizeSplitCardName( const std::string& name )
    {
        // Find the first slash, then remove any spaces or slashes to left and right.
        std::string::size_type first = name.find( '/' );
        if( first == std::string::npos ) return name;
        std::string::size_type last = first;
        while( (first > 0) && ( (name[first-1] == ' ') || (name[first-1] == '/') ) ) first--;
        while( (last < name.length()) &&
               ( (name[last+1] == ' ') || (name[last+1] == '/') ) )
        {
            last++;
        }
        std::string n = name;
        return n.replace( first, last-first+1, " // " );
    }

};

#endif // MTGJSON_H
