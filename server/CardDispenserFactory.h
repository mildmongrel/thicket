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

    class BoosterDispenser : public DraftCardDispenser<DraftCard>
    {
    public:

        BoosterDispenser( const std::string&                       setCode,
                          const std::vector<SlotType>&             boosterSlots,
                          const std::shared_ptr<CardPoolSelector>& cardPoolSelector,
                          const std::shared_ptr<spdlog::logger>&   logger )
          : mSetCode( setCode ),
            mBoosterSlots( boosterSlots ),
            mCardPoolSelector( cardPoolSelector ),
            mLogger( logger )
        {}

        virtual std::vector<DraftCard> dispense() override;

    private:
        std::string                       mSetCode;
        std::vector<SlotType>             mBoosterSlots;
        std::shared_ptr<CardPoolSelector> mCardPoolSelector;
        std::shared_ptr<spdlog::logger>   mLogger;
    };


    std::shared_ptr<const AllSetsData>  mAllSetsData;
    std::shared_ptr<spdlog::logger>     mLogger;
};

#endif
