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

    enum HashType
    {
        HASH_COCKATRICE,
        HASH_MWS
    };

    // Add a card (or multiple of a card) to the deck.
    void addCard( std::string cardName, ZoneType zone = ZONE_MAIN, uint16_t qty = 1 );

    // Get the formatted string for a deck based on format.  A set of
    // card names can be given as a parameter as a guide to place those
    // cards at the top of formatted sections.
    std::string getFormattedString( FormatType                   format = FORMAT_DEFAULT,
                                    const std::set<std::string>& priorityCardNames = std::set<std::string>() );

    // Compute a hash and return it as a string.
    std::string computeHash( HashType hash );

private:

    std::map<std::string,uint16_t> mCardQtyMainMap;
    std::map<std::string,uint16_t> mCardQtySideboardMap;

};

#endif // DECKLIST_H
