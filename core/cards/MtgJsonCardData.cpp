#include "MtgJsonCardData.h"

MtgJsonCardData::MtgJsonCardData( const std::string&      setCode,
                                  const rapidjson::Value& cardValue )
  : mSetCode( setCode ),
    mName( parseName( cardValue ) ),
    mMultiverseId( parseMultiverseId( cardValue ) ),
    mCMC( parseCMC( cardValue ) ),
    mRarity( parseRarity( cardValue ) ),
    mSplit( parseSplit( cardValue ) ),
    mColorSet( parseColors( cardValue ) ),
    mTypesSet( parseTypes( cardValue ) )
{
}


std::string
MtgJsonCardData::parseName( const rapidjson::Value& cardValue )
{
    rapidjson::Value::ConstMemberIterator itr;

    itr = cardValue.FindMember( "names" );
    if( itr != cardValue.MemberEnd() && MtgJson::isSplitCard( cardValue ) )
    {
        return MtgJson::createSplitCardName( itr->value );
    }

    itr = cardValue.FindMember( "name" );
    if( itr != cardValue.MemberEnd() )
    {
        if( itr->value.IsString() ) return itr->value.GetString();
    }
    return "";
}


int
MtgJsonCardData::parseMultiverseId( const rapidjson::Value& cardValue )
{
    rapidjson::Value::ConstMemberIterator itr = cardValue.FindMember( "multiverseid" );
    if( itr != cardValue.MemberEnd() )
    {
        if( itr->value.IsInt() ) return itr->value.GetInt();
    }
    return -1;
}


int
MtgJsonCardData::parseCMC( const rapidjson::Value& cardValue )
{ 
    rapidjson::Value::ConstMemberIterator itr = cardValue.FindMember( "cmc" );
    if( itr != cardValue.MemberEnd() )
    {
        return itr->value.IsInt() ? itr->value.GetInt() : -1;
    }
    else
    {
        // MtgJSON docs: "Cards without this field have an implied CMC of zero..."
        return 0;
    }
}


RarityType
MtgJsonCardData::parseRarity( const rapidjson::Value& cardValue )
{
    rapidjson::Value::ConstMemberIterator itr = cardValue.FindMember( "rarity" );
    if( itr != cardValue.MemberEnd() )
    {
        if( itr->value.IsString() )
        {
            std::string rarityStr( itr->value.GetString() );
            return MtgJson::getRarityFromString( rarityStr );
        }
    }
    return RARITY_UNKNOWN;
}


bool
MtgJsonCardData::parseSplit( const rapidjson::Value& cardValue )
{
    return MtgJson::isSplitCard( cardValue );
}


std::set<ColorType>
MtgJsonCardData::parseColors( const rapidjson::Value& cardValue )
{
    std::set<ColorType> colors;

    // Return empty set if no "colors" member.
    if( !cardValue.HasMember("colors") ) return colors;

    const rapidjson::Value& colorsVal = cardValue["colors"];
    if( colorsVal.IsArray() )
    {
        // Add colors from JSON array
        for( rapidjson::Value::ConstValueIterator iter = colorsVal.Begin();
                iter != colorsVal.End(); ++iter )
        {
            if( iter->IsString() )
            {
                std::string colorStr( iter->GetString() );
                if( colorStr == "White" )
                    colors.insert( COLOR_WHITE );
                if( colorStr == "Blue" )
                    colors.insert( COLOR_BLUE );
                if( colorStr == "Black" )
                    colors.insert( COLOR_BLACK );
                if( colorStr == "Red" )
                    colors.insert( COLOR_RED );
                if( colorStr == "Green" )
                    colors.insert( COLOR_GREEN );
                else
                {
                    // Unrecognized color string
                }
            }
            else
            {
                // JSON array element wasn't a string
            }
        }
    }

    return colors;
}


std::set<std::string>
MtgJsonCardData::parseTypes( const rapidjson::Value& cardValue )
{
    std::set<std::string> types;

    // Return empty set if no "types" member.
    if( !cardValue.HasMember("types") ) return types;

    const rapidjson::Value& typesVal = cardValue["types"];
    if( typesVal.IsArray() )
    {
        // Add types from JSON array
        for( rapidjson::Value::ConstValueIterator iter = typesVal.Begin();
                iter != typesVal.End(); ++iter )
        {
            if( iter->IsString() )
            {
                types.insert( iter->GetString() );
            }
            else
            {
                // JSON array element wasn't a string
            }
        }
    }

    return types;
}
