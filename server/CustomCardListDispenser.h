#ifndef CUSTOMCARDLISTDISPENSER_H
#define CUSTOMCARDLISTDISPENSER_H

#include "DraftConfig.pb.h"
#include "DraftTypes.h"
#include "DraftCardDispenser.h"

class CustomCardListDispenser : public DraftCardDispenser<DraftCard>
{
public:

    CustomCardListDispenser( const proto::DraftConfig::CardDispenser&  dispenserSpec,
                             const proto::DraftConfig::CustomCardList& customCardListSpec,
                             const Logging::Config&                    loggingConfig = Logging::Config() );

    bool isValid() const { return mValid; }

    unsigned int getPoolSize() const { return mCards.size() + mCardsDispensed.size(); }

    virtual std::vector<DraftCard> dispense() override;

private:
    bool                              mValid;
    std::vector<DraftCard>            mCards;
    std::vector<DraftCard>            mCardsDispensed;
    std::shared_ptr<spdlog::logger>   mLogger;
};

#endif
