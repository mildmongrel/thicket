#include "MtgJsonCardData.h"

MtgJsonCardData::MtgJsonCardData( const std::string& setCode,
                                  const rapidjson::Value& cardValue )
  : mSetCode( setCode ),
    mCardValue( cardValue ),
    mColorSet( parseColors() ),
    mTypesSet( parseTypes() )
{
}


std::string
MtgJsonCardData::getName() const
{
    rapidjson::Value::ConstMemberIterator itr;

    itr = mCardValue.FindMember( "names" );
    if( itr != mCardValue.MemberEnd() && isSplit() )
    {
        return MtgJson::createSplitCardName( itr->value );
    }

    itr = mCardValue.FindMember( "name" );
    if( itr != mCardValue.MemberEnd() )
    {
        if( itr->value.IsString() ) return itr->value.GetString();
    }
    return "";
}


int
MtgJsonCardData::getMultiverseId() const
{
    rapidjson::Value::ConstMemberIterator itr = mCardValue.FindMember( "multiverseid" );
    if( itr != mCardValue.MemberEnd() )
    {
        if( itr->value.IsInt() ) return itr->value.GetInt();
    }
    return -1;
}


int
MtgJsonCardData::getCMC() const
{ 
    rapidjson::Value::ConstMemberIterator itr = mCardValue.FindMember( "cmc" );
    if( itr != mCardValue.MemberEnd() )
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
MtgJsonCardData::getRarity() const
{
    rapidjson::Value::ConstMemberIterator itr = mCardValue.FindMember( "rarity" );
    if( itr != mCardValue.MemberEnd() )
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
MtgJsonCardData::isSplit() const
{
    return MtgJson::isSplitCard( mCardValue );
}


std::set<ColorType>
MtgJsonCardData::parseColors() const
{
    std::set<ColorType> colors;

    // Return empty set if no "colors" member.
    if( !mCardValue.HasMember("colors") ) return colors;

    const rapidjson::Value& colorsVal = mCardValue["colors"];
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
MtgJsonCardData::parseTypes() const
{
    std::set<std::string> types;

    // Return empty set if no "types" member.
    if( !mCardValue.HasMember("types") ) return types;

    const rapidjson::Value& typesVal = mCardValue["types"];
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
