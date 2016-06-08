#ifndef DECKLIST_H
#define DECKLIST_H

#include <map>
#include <set>

class Decklist
{
public:

    enum ZoneType
    {
        ZONE_MAIN,
        ZONE_SIDEBOARD
    };

    // Format types.  Could be expanded to use other program-specific file formats.
    enum FormatType
    {
        FORMAT_DEFAULT
    };

    // A set of card names as a guide to place those cards at the top of
    // formatted sections.
    void setPriorityCardNames( const std::set<std::string>& names )
    {
        mPriorityCardNames = names;
    }

    // Add a card (or multiple of a card) to the deck.
    void addCard( std::string cardName, ZoneType zone = ZONE_MAIN, uint16_t qty = 1 );

    // Get the formatted string for a deck based on format.
    std::string getFormattedString( FormatType format = FORMAT_DEFAULT ) const;

private:

    std::map<std::string,uint16_t> mCardQtyMainMap;
    std::map<std::string,uint16_t> mCardQtySideboardMap;
    std::set<std::string> mPriorityCardNames;

};

#endif // DECKLIST_H
