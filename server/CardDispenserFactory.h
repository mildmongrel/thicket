#ifndef CARDDISPENSERFACTORY_H
#define CARDDISPENSERFACTORY_H

#include "DraftConfig.pb.h"
#include "AllSetsData.h"
#include "CardPoolSelector.h"
#include "SimpleRandGen.h"
#include "DraftTypes.h"
#include "DraftCardDispenser.h"
#include <memory>

// Creates dispensers for a given draft configuration.
// Currently this is very limited:
//  - only does boosters, no other methods covered
//  - always replaces after booster
//  - light error checking, config should be validated beforehand

class CardDispenserFactory
{
public:

    explicit CardDispenserFactory( 
            const std::shared_ptr<const AllSetsData>& allSetsData,
            const Logging::Config&                    loggingConfig = Logging::Config() );


    // Create dispensers based on DraftConfig.  Returns an empty list if an error occurred.
    DraftCardDispenserSharedPtrVector<DraftCard> createCardDispensers(
            const proto::DraftConfig& draftConfig ) const;

private:

    std::shared_ptr<const AllSetsData>  mAllSetsData;
    Logging::Config                     mLoggingConfig;
    std::shared_ptr<spdlog::logger>     mLogger;
};

#endif
