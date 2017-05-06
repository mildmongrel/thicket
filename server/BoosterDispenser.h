#ifndef BOOSTERDISPENSER_H
#define BOOSTERDISPENSER_H

#include "DraftCardDispenser.h"
#include "AllSetsData.h"
#include "CardPoolSelector.h"
#include "DraftTypes.h"

class BoosterDispenser : public DraftCardDispenser<DraftCard>
{
public:

    BoosterDispenser( const proto::DraftConfig::CardDispenser&  dispenserSpec,
                      const std::shared_ptr<const AllSetsData>& allSetsData,
                      const Logging::Config&                    loggingConfig = Logging::Config() );

    bool isValid() const { return mValid; }

    virtual std::vector<DraftCard> dispenseAll() override;
    virtual std::vector<DraftCard> dispense( unsigned int quantity ) override;

private:

    void reset();

    bool                              mValid;
    std::string                       mSetCode;
    std::vector<SlotType>             mBoosterSlots;
    std::deque<DraftCard>             mCards;
    std::shared_ptr<CardPoolSelector> mCardPoolSelector;
    std::shared_ptr<spdlog::logger>   mLogger;
};

#endif
