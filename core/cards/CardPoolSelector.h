#ifndef CARDPOOLSELECTOR_H
#define CARDPOOLSELECTOR_H

#include "SetDataTypes.h"
#include "CardDataTypes.h"

#include <map>
#include <random>
#include <memory>

#include "RandGen.h"
#include "Logging.h"

class CardPoolSelector
{
public:
    typedef std::multimap<RarityType,std::string> SetRarityToCardMap;

    CardPoolSelector( const SetRarityToCardMap& cardPool,
                      std::shared_ptr<RandGen>& rng,
                      float                     mythicRareProbability = 0.125,
                      const Logging::Config&    loggingConfig = Logging::Config() );

    void setMythicRareProbability( const float& prob ) { mMythicRareProbability = prob; }
    float getMythicRareProbability() const { return mMythicRareProbability; }

    // Reset the card pool.
    void resetCardPool();

    // Select a card randomly based on slot type and remove it from the card pool.
    bool selectCard( const SlotType& slot, std::string& selectedCard );

    int getPoolSize() { return mCardPool.size(); }

private:

    bool getRarityForSlot( const SlotType& slot, RarityType& rarity ) const;

    float mMythicRareProbability;
    SetRarityToCardMap mCardPool;
    SetRarityToCardMap mCardsRemovedFromPool;
    std::shared_ptr<RandGen> mRng;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CARDPOOLSELECTOR_H
