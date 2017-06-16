#ifndef ALLSETSDATA_H
#define ALLSETSDATA_H

#include "SetDataTypes.h"
#include "CardDataTypes.h"
#include "CardData.h"
#include <string>
#include <vector>
#include <map>

class AllSetsData
{
public:

    virtual ~AllSetsData() {}

    virtual std::vector<std::string> getSetCodes() const = 0;

    // returns default string if no set found or no name value
    virtual std::string getSetName( const std::string& code, const std::string& defaultName ="" ) const = 0;
    virtual std::string getSetGathererCode( const std::string& code, const std::string& defaultVal ="" ) const = 0;

    virtual bool hasBoosterSlots( const std::string& code ) const = 0;
    virtual std::vector<SlotType> getBoosterSlots( const std::string& code ) const = 0;

    virtual std::multimap<RarityType,std::string> getCardPool( const std::string& code ) const = 0;

    virtual CardData* createCardData( const std::string& code, const std::string& name ) const = 0;
    virtual CardData* createCardData( int multiverseId ) const = 0;

    virtual std::string findSetCode( const std::string& name ) const = 0;
};

#endif  // ALLSETSDATA_H
