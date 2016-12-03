#include "MtgJsonAllSetsData.h"
#include "MtgJsonCardData.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h"

#include <map>

using namespace rapidjson;

MtgJsonAllSetsData::MtgJsonAllSetsData( unsigned int    cacheSize,
                                        Logging::Config loggingConfig )
  : mCardLookupLRUCache( cacheSize ),
    mCardLookupLRUCacheHits( 0 ),
    mCardLookupLRUCacheMisses( 0 ),
    mSetCodeLookupLRUCache( cacheSize ),
    mSetCodeLookupLRUCacheHits( 0 ),
    mSetCodeLookupLRUCacheMisses( 0 ),
    mLogger( loggingConfig.createLogger() )
{}


bool
MtgJsonAllSetsData::parse( FILE* fp )
{
    char readBuffer[65536];
    FileReadStream is( fp, readBuffer, sizeof(readBuffer) );
    mLogger->debug( "parsing mtgjson file" );
    mDoc.ParseStream( is );

    if( mDoc.HasParseError() )
    {
        mLogger->error( "json parsing error (offset {}): {}",
                mDoc.GetErrorOffset(), GetParseError_En( mDoc.GetParseError() ) );
        return false;
    }

    // Temporary multimaps to store set codes by release date.
    using SetCodesByReleaseDateMap = std::multimap<std::string,std::string>;
    SetCodesByReleaseDateMap scrdMapHi;
    SetCodesByReleaseDateMap scrdMapLo;
    std::vector<std::string> setCodesNoReleaseDate;

    // Do all JSON verification up front so future calls are easy.
    for( Value::ConstMemberIterator setIter = mDoc.MemberBegin(); setIter != mDoc.MemberEnd(); ++setIter )
    {
        const std::string setCode = setIter->name.GetString();
        const Value& setValue = setIter->value;
        if( !setValue.IsObject() )
        {
            mLogger->warn( "set value for {} is not an object", setCode );
            continue;
        }

        if( !setValue.HasMember("name") )
        {
            mLogger->warn( "set value for {} has no 'name' member, ignoring set", setCode );
            continue;
        }
        if( !setValue["name"].IsString() )
        {
            mLogger->warn( "'name' member for {} is not a string", setCode );
            continue;
        }

        if( !setValue.HasMember("cards") )
        {
            mLogger->warn( "set value for {} has no 'cards' member, ignoring set", setCode );
            continue;
        }
        if( !setValue["cards"].IsArray() )
        {
            mLogger->warn( "'cards' member for {} is not an array", setCode );
            continue;
        }

        // Note that inserting the code into the set will lose file ordering of the sets;
        // the set codes will be ordered alphabetically.
        mAllSetCodes.insert( setCode );

        // Accumulate sets in the release date multimap.
        if( setValue.HasMember("releaseDate") && setValue["releaseDate"].IsString() )
        {
            const std::string& relDateStr = setValue["releaseDate"].GetString();
            if( setValue.HasMember("type") && setValue["type"].IsString() )
            {
                // Type "expansion" or "core" sets are higher-priority.
                const std::string& typeStr = setValue["type"].GetString();
                if( (typeStr == "expansion") || (typeStr == "core") )
                {
                    scrdMapHi.insert( std::make_pair( relDateStr, setCode ) );
                }
                else
                {
                    scrdMapLo.insert( std::make_pair( relDateStr, setCode ) );
                }
            }
            else
            {
                // No type for set - treat as low priority.
                mLogger->notice( "'type' member not present or invalid for {}", setCode );
                scrdMapLo.insert( std::make_pair( relDateStr, setCode ) );
            }
        }
        else
        {
            mLogger->notice( "'releaseDate' member not present or invalid for {}", setCode );
            setCodesNoReleaseDate.push_back( setCode );
        }

        if( !setValue.HasMember("booster") )
        {
            // This is expected for some sets so it's not a warning.
            mLogger->debug( "set value for {} has no 'booster' member", setCode );
            continue;
        }
        if( !setValue["booster"].IsArray() )
        {
            mLogger->warn( "'booster' member for {} is not an array", setCode );
            continue;
        }

        // Note that inserting the code into the set will lose file ordering of the sets;
        // the set codes will be ordered alphabetically.
        mBoosterSetCodes.insert( setCode );
    }

    // Assemble the prioritized set code vector.  This is the high priority
    // sets in reverse-chron order, then the low priority sets in
    // reverse-chron order, then any sets without release dates.
    mSearchPrioritizedAllSetCodes.reserve(
            scrdMapHi.size() + scrdMapLo.size() + setCodesNoReleaseDate.size() );
    for( auto kv : scrdMapLo )
    {
        mSearchPrioritizedAllSetCodes.insert(
                mSearchPrioritizedAllSetCodes.begin(), kv.second );
    }
    for( auto kv : scrdMapHi )
    {
        mSearchPrioritizedAllSetCodes.insert(
                mSearchPrioritizedAllSetCodes.begin(), kv.second );
    }
    for( auto sc : setCodesNoReleaseDate )
    {
        mSearchPrioritizedAllSetCodes.push_back( sc );
    }

    return true;
}


std::vector<std::string>
MtgJsonAllSetsData::getSetCodes() const
{
    std::vector<std::string> setCodes;
    for( auto setCode : mAllSetCodes )
    {
        setCodes.push_back( setCode );
    }
    return setCodes;
}


std::string
MtgJsonAllSetsData::getSetName( const std::string& code, const std::string& defaultName ) const
{
    if( mAllSetCodes.count(code) == 0 )
    {
        mLogger->warn( "Unable to find set {}, returning default name", code );
        return defaultName;
    }
    const Value& nameValue = mDoc[code]["name"];
    return nameValue.GetString();
}


bool
MtgJsonAllSetsData::hasBoosterSlots( const std::string& code ) const
{
    return (mBoosterSetCodes.count(code) > 0);
}


std::vector<SlotType>
MtgJsonAllSetsData::getBoosterSlots( const std::string& code ) const
{
    std::vector<SlotType> boosterSlots;

    if( mBoosterSetCodes.count(code) == 0 )
    {
        mLogger->warn( "No booster member in set {}, returning empty booster slots", code );
        return boosterSlots;
    }

    // In parse() this was vetted to be safe and yield an Array-type value.
    const Value& boosterValue = mDoc[code]["booster"];

    mLogger->debug( "{:-^40}", "assembling booster slots" );
    for( Value::ConstValueIterator iter = boosterValue.Begin(); iter != boosterValue.End(); ++iter )
    {
        if( iter->IsArray() )
        {
            // create a set of any strings in the array
            std::set<std::string> slotArraySet;
            for( unsigned i = 0; i < iter->Size(); ++i )
            {
                const Value& val = (*iter)[i];
                if( val.IsString() )
                {
                    slotArraySet.insert( val.GetString() );
                    mLogger->debug( "booster slot array[{}]: {}", i, val.GetString() );
                }
                else
                {
                    mLogger->warn( "Non-string in booster slot array, ignoring!" );
                }
            }
            const std::set<std::string> rareMythicRareSlot { "rare", "mythic rare" };
            if( slotArraySet == rareMythicRareSlot )
                boosterSlots.push_back( SLOT_RARE_OR_MYTHIC_RARE );
            else
                mLogger->warn( "Unrecognized booster slot array, ignoring!" );
        }
        else if( iter->IsString() )
        {
            std::string slotStr( iter->GetString() );
            mLogger->debug( "booster slot string: {}", slotStr );
            if( slotStr == "common" )
                boosterSlots.push_back( SLOT_COMMON );
            else if( slotStr == "uncommon" )
                boosterSlots.push_back( SLOT_UNCOMMON );
            else if( slotStr == "rare" )
                boosterSlots.push_back( SLOT_RARE );
            else if( slotStr == "timeshifted purple" )
                boosterSlots.push_back( SLOT_TIMESHIFTED_PURPLE );
            else if( slotStr == "land" )      { /* do nothing */ }
            else if( slotStr == "marketing" ) { /* do nothing */ }
            else
                mLogger->warn( "Unrecognized booster slot type {}, ignoring!", slotStr );
        }
        else
        {
            mLogger->warn( "Non-string booster slot type, ignoring!" );
        }
    }
    return boosterSlots;
}


std::multimap<RarityType,std::string>
MtgJsonAllSetsData::getCardPool( const std::string& code ) const
{
    std::multimap<RarityType,std::string> rarityMap;

    if( mAllSetCodes.count(code) == 0 )
    {
        mLogger->warn( "Unable to find set {}, returning empty card pool", code );
        return rarityMap;
    }

    // In parse() this was vetted to be safe and yield an Array-type value.
    const Value& cardsValue = mDoc[code]["cards"];

    mLogger->debug( "{:-^40}", "assembling card pool" );
    for( Value::ConstValueIterator iter = cardsValue.Begin(); iter != cardsValue.End(); ++iter )
    {
        Value::ConstMemberIterator nameIter = iter->FindMember( "name" );
        Value::ConstMemberIterator rarityIter = iter->FindMember( "rarity" );

        if( nameIter != iter->MemberEnd() && nameIter->value.IsString() &&
            rarityIter != iter->MemberEnd() && rarityIter->value.IsString() )
        {
            std::string nameStr( nameIter->value.GetString() );
            std::string rarityStr( rarityIter->value.GetString() );
            mLogger->debug( "json: {} : {}", nameStr, rarityStr );

            // Some cards have multiple entries (i.e. split/flip/double-sided),
            // so make sure they are only represented once.  Done by skipping
            // over cards whose name doesn't match the first entry of the
            // 'names' array (if it exists).
            if( iter->HasMember("names") )
            {
                const Value& names = (*iter)["names"];
                if( names.IsArray() && !names.Empty() && (nameStr != names[0]) )
                {
                    continue;
                }

                // Modify the name for split cards.
                if( MtgJson::isSplitCard( *iter ) )
                {
                    nameStr = MtgJson::createSplitCardName( names );
                }

            }

            // Some cards have variations with multiple entries that should
            // only be counted once.  Done by checking if there are
            // variations and checking a card's number for a non-digit,
            // non-'a' ending.
            if( iter->HasMember("variations") && iter->HasMember("number") )
            {
                const Value& numberValue = (*iter)["number"];
                if( numberValue.IsString() )
                {
                    const std::string numberStr( numberValue.GetString() );
                    const char c = numberStr.back();
                    if( !std::isdigit( c ) && (c != 'a') )
                    {
                        continue;
                    }
                }
            }

            RarityType rarity = MtgJson::getRarityFromString( rarityStr );
            if( rarity != RARITY_UNKNOWN )
                rarityMap.insert( std::make_pair( rarity, nameStr ) );
            else
                mLogger->warn( "Unknown rarity type {}, ignoring!", rarityStr );
        }
        else
        {
            mLogger->notice( "card entry without name or rarity in set {}", code );
        }
    }

    return rarityMap;
}


CardData*
MtgJsonAllSetsData::createCardData( const std::string& code, const std::string& name ) const
{
    // See if the card is in the cache.
    const SimpleCardData cardLookupCacheKey( name, code );
    if( mCardLookupLRUCache.exists( cardLookupCacheKey ) )
    {
        mCardLookupLRUCacheHits++;
        Value::ConstValueIterator iter = mCardLookupLRUCache.get( cardLookupCacheKey );
        return new MtgJsonCardData( code, *iter );
    }
    mCardLookupLRUCacheMisses++;

    if( mAllSetCodes.count(code) == 0 )
    {
        mLogger->warn( "Unable to find set {}", code );
        return nullptr;
    }

    // In parse() this was vetted to be safe and yield an Array-type value.
    const Value& cardsValue = mDoc[code]["cards"];

    Value::ConstValueIterator iter = findCardValueByName(
            cardsValue.Begin(), cardsValue.End(), name );
    if( iter != cardsValue.End() )
    {
        mLogger->debug( "found name {}", name );
        mCardLookupLRUCache.put( cardLookupCacheKey, iter );
        return new MtgJsonCardData( code, *iter );
    }

    mLogger->debug( "unable to find card name {}", name );
    return nullptr;
}


CardData*
MtgJsonAllSetsData::createCardData( int multiverseId ) const
{
    // Unfortunately this is a slow linear search.
    for( const std::string& setCode : mAllSetCodes )
    {
        // In parse() this was vetted to be safe and yield an Array-type value.
        const Value& cardsValue = mDoc[setCode]["cards"];

        // This is a linear search looking for the multiverse id.
        for( Value::ConstValueIterator iter = cardsValue.Begin();
                iter != cardsValue.End(); ++iter )
        {
            if( iter->HasMember("multiverseid") )
            {
                const Value& multiverseIdValue = (*iter)["multiverseid"];
                if( multiverseIdValue.IsInt() )
                {
                    int muid = multiverseIdValue.GetInt();
                    if( muid == multiverseId )
                    {
                        mLogger->debug( "found muid ", muid );
                        return new MtgJsonCardData( setCode, *iter );
                    }
                }
            }
        }
    }

    mLogger->warn( "unable to find card multiverseId {}", multiverseId );
    return nullptr;
}


std::string
MtgJsonAllSetsData::findSetCode( const std::string& name ) const
{
    // See if the value is in the cache.
    if( mSetCodeLookupLRUCache.exists( name ) )
    {
        mSetCodeLookupLRUCacheHits++;
        return mSetCodeLookupLRUCache.get( name );
    }
    mSetCodeLookupLRUCacheMisses++;

    std::string retSetCode;
    for( const std::string& setCode : mSearchPrioritizedAllSetCodes )
    {
        // In parse() this was vetted to be safe and yield an Array-type value.
        const Value& cardsValue = mDoc[setCode]["cards"];

        Value::ConstValueIterator iter = findCardValueByName(
                cardsValue.Begin(), cardsValue.End(), name );
        if( iter != cardsValue.End() )
        {
            retSetCode = setCode;
            break;
        }
    }

    // Cache the search result and return.
    mSetCodeLookupLRUCache.put( name, retSetCode );
    return retSetCode;
}


Value::ConstValueIterator
MtgJsonAllSetsData::findCardValueByName( Value::ConstValueIterator first,
                                         Value::ConstValueIterator last,
                                         const std::string&        name ) const
{
    Value::ConstValueIterator iter = first;

    for( Value::ConstValueIterator iter = first; iter != last; ++iter )
    {
        std::string nameStr( (*iter)["name"].GetString() );
        if( nameStr == name )
        {
            mLogger->debug( "found name {}", name );
            return iter;
        }
        else if( iter->HasMember("names") )
        {
            // Here we check if the card has multiple names, i.e. split
            // cards.  If so, create a split card name and normalize the
            // name parameter and see if they match.

            std::string splitCardName = MtgJson::createSplitCardName( (*iter)["names"] );
            std::string nameNormalized = MtgJson::normalizeSplitCardName( name );

            if( nameNormalized == splitCardName )
            {
                mLogger->debug( "found split name {}", name );
                return iter;
            }
        }
    }

    return last;
}
