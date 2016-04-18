#ifndef MTGJSONALLSETSDATA_H
#define MTGJSONALLSETSDATA_H

#include "AllSetsData.h"
#include "rapidjson/document.h"
#include <string>
#include <set>
#include <cstdio>
#include "Logging.h"

class MtgJsonAllSetsData : public AllSetsData
{
public:

    MtgJsonAllSetsData( Logging::Config loggingConfig = Logging::Config() )
      : mLogger( loggingConfig.createLogger() )
    {}

    bool parse( FILE* fp );

    virtual std::vector<std::string> getSetCodes() const override;
    virtual std::string getSetName( const std::string& code, const std::string& defaultName = "") const override;
    virtual bool hasBoosterSlots( const std::string& code ) const override;
    virtual std::vector<SlotType> getBoosterSlots( const std::string& code ) const override;
    virtual std::multimap<RarityType,std::string> getCardPool( const std::string& code ) const override;
    virtual CardData* createCardData( const std::string& code, const std::string& name ) const override;
    virtual CardData* createCardData( int multiverseId ) const override;

private:

    rapidjson::Document mDoc;
    std::set<std::string> mAllSetCodes;
    std::set<std::string> mBoosterSetCodes;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // MTGJSONALLSETSDATA_H
