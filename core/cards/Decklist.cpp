#include "Decklist.h"
#include "StringUtil.h"
#include <sstream>
#include <regex>


bool
Decklist::isEmpty() const
{
    return mCardQtyMainMap.empty() && mCardQtySideboardMap.empty();
}


void
Decklist::clear()
{
    mCardQtyMainMap.clear();
    mCardQtySideboardMap.clear();
}


void
Decklist::addCard( std::string cardName, Decklist::ZoneType zone, uint16_t qty )
{
    if( qty == 0 ) return;
    SimpleCardData cardData( cardName, "" );

    std::map<SimpleCardData,uint16_t>& cardQtyMap =
            (zone == ZONE_MAIN) ? mCardQtyMainMap : mCardQtySideboardMap;
    cardQtyMap[cardName] += qty;
}


void
Decklist::addCard( const SimpleCardData& cardData, ZoneType zone, uint16_t qty )
{
    if( qty == 0 ) return;

    std::map<SimpleCardData,uint16_t>& cardQtyMap =
            (zone == ZONE_MAIN) ? mCardQtyMainMap : mCardQtySideboardMap;
    cardQtyMap[cardData] += qty;
}


std::vector<SimpleCardData>
Decklist::getCards( ZoneType zone ) const
{
    std::vector<SimpleCardData> v;
    const std::map<SimpleCardData,uint16_t>& cardQtyMap =
            (zone == ZONE_MAIN) ? mCardQtyMainMap : mCardQtySideboardMap;
    for( auto kv : cardQtyMap )
    {
        v.push_back( kv.first );
    }
    return v;
}


unsigned int
Decklist::getCardQuantity( const SimpleCardData& cardData, ZoneType zone ) const
{
    const std::map<SimpleCardData,uint16_t>& cardQtyMap =
            (zone == ZONE_MAIN) ? mCardQtyMainMap : mCardQtySideboardMap;
    auto iter = cardQtyMap.find( cardData );
    return (iter != cardQtyMap.end()) ? iter->second : 0;
}


unsigned int
Decklist::getTotalQuantity( ZoneType zone ) const
{
    const std::map<SimpleCardData,uint16_t>& cardQtyMap =
            (zone == ZONE_MAIN) ? mCardQtyMainMap : mCardQtySideboardMap;
    unsigned int total = 0;
    for( auto kv : cardQtyMap )
    {
        total += kv.second;
    }
    return total;
}


std::string
Decklist::getFormattedString( Decklist::FormatType format ) const
{
    std::stringstream ss;

    auto printFn = [&ss,format]( const std::pair<SimpleCardData,int> kv, bool sb ) {
        if( sb ) ss << "SB: ";
        ss << kv.second << " ";
        if( (format == FORMAT_MWDECK) && !kv.first.getSetCode().empty() )
        {
            ss << "[" << kv.first.getSetCode() << "] ";
        }
        ss << kv.first.getName();
        ss << std::endl;
    };

    // Priority main cards.
    for( auto& kv : mCardQtyMainMap )
    {
        if( mPriorityCardNames.count( kv.first.getName() ) > 0 )
        {
            printFn( kv, false );
        }
    }

    // Non-priority main cards.
    for( auto& kv : mCardQtyMainMap )
    {
        if( mPriorityCardNames.count( kv.first.getName() ) == 0 )
        {
            printFn( kv, false );
        }
    }

    ss << std::endl;

    // Priority sideboard cards.
    for( auto& kv : mCardQtySideboardMap )
    {
        if( mPriorityCardNames.count( kv.first.getName() ) > 0 )
        {
            printFn( kv, true );
        }
    }

    // Non-priority sideboard cards.
    for( auto& kv : mCardQtySideboardMap )
    {
        if( mPriorityCardNames.count( kv.first.getName() ) == 0 )
        {
            printFn( kv, true );
        }
    }

    return ss.str();
}


Decklist::ParseResult
Decklist::parse( const std::string& deckStr )
{
    ParseResult result;

    std::stringstream ss( deckStr );
    std::string rawLine;
    std::string line;

    // Printable regex: All characters printable.
    std::regex printableRegex( "^[[:print:]]*$" );

    // Comment regex: Two forward slashes followed by anything
    std::regex commentRegex( "^//.*$" );

    // MWDECK regex
    std::regex mwdeckRegex( // "SB", case-insensitive with optional colon
                            "^([sS][bB]:?[[:space:]]*)?"

                            // qty with optional case-insensitive 'x'
                            "(([[:digit:]]+)[xX]?[[:space:]]*)?"

                            // set in brackets
                            "\\[([[:alnum:]]*)\\]?[[:space:]]*"

                            // name
                            "(.*)$" );

    // DEC regex
    std::regex decRegex( // "SB", case-insensitive with optional colon
                         "^([sS][bB]:?[[:space:]]*)?"

                         // qty with optional case-insensitive 'x'
                         "(([[:digit:]]+)[xX]?[[:space:]]*)?"

                         // name
                         "(.*)$" );

    unsigned int lineNum = 0;
    while( std::getline( ss, rawLine ) )
    {
        lineNum++;
        std::smatch match;

        line = StringUtil::trim( rawLine );

        bool sb = false;
        std::string qtyStr;
        std::string nameStr;
        std::string setStr;

        if( line.empty() )
        {
            continue;
        }
        else if( !std::regex_match( line, match, printableRegex ) )
        {
            result.errors.push_back( ParseResult::Error( lineNum, "<binary data>", "Contains binary data" ) );
            continue;
        }
        else if( std::regex_match( line, match, commentRegex ) )
        {
            continue;
        }
        else if( std::regex_match( line, match, mwdeckRegex ) )
        {
            sb = !match[1].str().empty();
            qtyStr = match[3].str();
            setStr = match[4].str();
            nameStr = match[5].str();
        }
        else if( std::regex_match( line, match, decRegex ) )
        {
            sb = !match[1].str().empty();
            qtyStr = match[3].str();
            nameStr = match[4].str();
        }
        else
        {
            result.errors.push_back( ParseResult::Error( lineNum, rawLine, "Unrecognized format" ) );
            continue;
        }

        if( nameStr.empty() )
        {
            result.errors.push_back( ParseResult::Error( lineNum, rawLine, "Missing name field" ) );
            continue;
        }

        SimpleCardData c( nameStr, setStr );
        uint16_t qty = 1;
        if( !qtyStr.empty() )
        {
            std::istringstream( qtyStr ) >> qty;
        }

        if( sb )
        {
            mCardQtySideboardMap[c] = qty;
        }
        else
        {
            mCardQtyMainMap[c] = qty;
        }
    }

    return result;
}

