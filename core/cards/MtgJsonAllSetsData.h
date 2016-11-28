#ifndef MTGJSONALLSETSDATA_H
#define MTGJSONALLSETSDATA_H

#include "AllSetsData.h"
#include "rapidjson/document.h"
#include <string>
#include <set>
#include <vector>
#include "Logging.h"

class MtgJsonAllSetsData : public AllSetsData
{
public:

    MtgJsonAllSetsData( Logging::Config loggingConfig = Logging::Config() )
      : mLogger( loggingConfig.createLogger() )
    {}
    virtual ~MtgJsonAllSetsData() {}

    bool parse( FILE* fp );

    virtual std::vector<std::string> getSetCodes() const override;
    virtual std::string getSetName( const std::string& code, const std::string& defaultName = "") const override;
    virtual bool hasBoosterSlots( const std::string& code ) const override;
    virtual std::vector<SlotType> getBoosterSlots( const std::string& code ) const override;
    virtual std::multimap<RarityType,std::string> getCardPool( const std::string& code ) const override;

    // Create a CardData object given set code and name.  If the set code is empty, search
    // through known sets in reverse chronological order for a name match.  Returns nullptr
    // if no match.
    virtual CardData* createCardData( const std::string& code, const std::string& name ) const override;

    virtual CardData* createCardData( int multiverseId ) const override;

private:

    rapidjson::Document mDoc;
    std::set<std::string> mAllSetCodes;
    std::vector<std::string> mSearchPrioritizedAllSetCodes;
    std::set<std::string> mBoosterSetCodes;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // MTGJSONALLSETSDATA_H
