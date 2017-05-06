#include "BoosterDispenser.h"
#include "SimpleRandGen.h"


BoosterDispenser::BoosterDispenser( const proto::DraftConfig::CardDispenser&  dispenserSpec,
                                    const std::shared_ptr<const AllSetsData>& allSetsData,
                                    const Logging::Config&                    loggingConfig )
  : mValid( false ),
    mLogger( loggingConfig.createLogger() )
{
    // Must have at least a single booster
    if( dispenserSpec.source_booster_set_codes_size() < 1 )
    {
        mLogger->error( "dispenser does not have booster slots!" );
        return;
    }
   
    // Currently only handle a single booster
    if( dispenserSpec.source_booster_set_codes_size() > 1 )
    {
        mLogger->error( "dispenser only supports a single booster!" );
        return;
    }

    mSetCode = dispenserSpec.source_booster_set_codes( 0 );
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

    reset();
}


void
BoosterDispenser::reset()
{
    mCards.clear();
    mCardPoolSelector->resetCardPool();

    for( std::vector<SlotType>::size_type i = 0; i < mBoosterSlots.size(); ++i )
    {
        std::string selectedCard;
        bool result = mCardPoolSelector->selectCard( mBoosterSlots[i], selectedCard );
        if( result )
        {
            mCards.push_back( DraftCard( selectedCard, mSetCode ) );
        }
        else
        {
            mLogger->error( "error generating card! setCode={}", mSetCode );
        }
    }

}


std::vector<DraftCard>
BoosterDispenser::dispense( unsigned int qty )
{
    std::vector<DraftCard> cards;

    if( !mValid || mCards.empty() )
    {
        mLogger->error( "unexpected empty cards!", mSetCode );
        return cards;
    }

    for( unsigned int i = 0; i < qty; ++i )
    {
        cards.push_back( mCards.front() );
        mCards.pop_front();
        if( mCards.empty() ) reset();
    }
    return cards;
}


std::vector<DraftCard>
BoosterDispenser::dispenseAll()
{
    std::vector<DraftCard> cards;
    std::copy( mCards.begin(), mCards.end(), std::back_inserter( cards ) );
    reset();
    return cards;
}
