#ifndef DECKLIST_H
#define DECKLIST_H

#include <map>
#include <set>
#include <vector>
#include "SimpleCardData.h"

class Decklist
{
public:

    enum ZoneType
    {
        ZONE_MAIN,
        ZONE_SIDEBOARD
    };

    // Format types.  Expandable for future program-specific file formats.
    enum FormatType
    {
        FORMAT_DEC,
        FORMAT_MWDECK,
    };

    struct ParseResult
    {
        struct Error
        {
            Error( unsigned int lnNum, const std::string& ln, const std::string& msg )
              : lineNum( lnNum ), line( ln ), message( msg ) {}

            unsigned int lineNum;
            std::string  line;
            std::string  message;
        };

        bool hasErrors() const { return !errors.empty(); }
        unsigned int errorCount() const { return errors.size(); }

        std::vector<Error> errors;
    };

    // A set of card names as a guide to place those cards at the top of
    // formatted sections.
    void setPriorityCardNames( const std::set<std::string>& names )
    {
        mPriorityCardNames = names;
    }

    bool isEmpty() const;
    void clear();

    // Add a card (or multiple of a card) to the deck.
    void addCard( std::string cardName, ZoneType zone = ZONE_MAIN, uint16_t qty = 1 );
    void addCard( const SimpleCardData& cardData, ZoneType zone = ZONE_MAIN, uint16_t qty = 1 );

    // Get a list of all cards in a zone.
    std::vector<SimpleCardData> getCards( ZoneType zone ) const;

    // Get quantity of a card in a zone.
    unsigned int getCardQuantity( const SimpleCardData& cardData, ZoneType zone ) const;

    // Get total quantity of all cards in a zone.
    unsigned int getTotalQuantity( ZoneType zone ) const;

    // Get the formatted string for a deck based on format.
    std::string getFormattedString( FormatType format = FORMAT_DEC ) const;

    // Parse a string and add all contents to the deck.
    ParseResult parse( const std::string& deckStr );

    friend bool operator==( const Decklist& a, const Decklist& b );

private:

    std::map<SimpleCardData,uint16_t> mCardQtyMainMap;
    std::map<SimpleCardData,uint16_t> mCardQtySideboardMap;
    std::set<std::string> mPriorityCardNames;

};

inline bool operator==( const Decklist& a, const Decklist& b )
{
    return (a.mCardQtyMainMap == b.mCardQtyMainMap) && (a.mCardQtySideboardMap == b.mCardQtySideboardMap);
}

inline bool operator!=( const Decklist& a, const Decklist& b )
{
    return !(a == b);
}

#endif // DECKLIST_H
