#include "BoosterDispenser.h"
#include "SimpleRandGen.h"


BoosterDispenser::BoosterDispenser( const proto::DraftConfig::CardDispenser&  dispenserSpec,
                                    const std::shared_ptr<const AllSetsData>& allSetsData,
                                    const Logging::Config&                    loggingConfig )
  : mValid( false ),
    mLogger( loggingConfig.createLogger() )
{
    if( dispenserSpec.method() != proto::DraftConfig::CardDispenser::METHOD_BOOSTER )
    {
        mLogger->error( "invalid dispenser method (expected booster)" );
        return;
    }

    if( dispenserSpec.replacement() != proto::DraftConfig::CardDispenser::REPLACEMENT_ALWAYS )
    {
        mLogger->error( "invalid replacement method (expected always)" );
        return;
    }

    mSetCode = dispenserSpec.set_code();
    mBoosterSlots = allSetsData->getBoosterSlots( mSetCode );

    if( mBoosterSlots.empty() )
    {
        mLogger->error( "set {} does not have booster slots!", mSetCode );
        return;
    }

    std::multimap<RarityType,std::string> rarityMap = allSetsData->getCardPool( mSetCode );
    if( rarityMap.empty() )
    {
        mLogger->error( "set {} does not have rarities!", mSetCode );
        return;
    }

    auto rng = std::shared_ptr<RandGen>( new SimpleRandGen() );
    mCardPoolSelector = std::make_shared<CardPoolSelector>( rarityMap, rng );

    mValid = true;
}


std::vector<DraftCard>
BoosterDispenser::dispense()
{
    std::vector<DraftCard> boosterCards;
    for( std::vector<SlotType>::size_type i = 0; i < mBoosterSlots.size(); ++i )
    {
        std::string selectedCard;
        bool result = mCardPoolSelector->selectCard( mBoosterSlots[i], selectedCard );
        if( result )
        {
            boosterCards.push_back( DraftCard( selectedCard, mSetCode ) );
        }
        else
        {
            mLogger->error( "error selecting card! setCode={}", mSetCode );
        }
    }
    mCardPoolSelector->resetCardPool();
    return boosterCards;
}

