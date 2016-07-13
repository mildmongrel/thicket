#include "CardDispenserFactory.h"

// Creates dispensers for a given draft configuration.
// Currently this is very limited:
//  - only does boosters, no other methods covered
//  - always replaces after booster
//  - light error checking, config should be validated beforehand

CardDispenserFactory::CardDispenserFactory( 
        const std::shared_ptr<const AllSetsData>& allSetsData,
        const Logging::Config&                    loggingConfig )
  : mAllSetsData( allSetsData ),
    mLogger( loggingConfig.createLogger() )
{}


// Create dispensers based on DraftConfig.  Returns an empty list if an error occurred.
DraftCardDispenserSharedPtrVector<DraftCard>
CardDispenserFactory::createCardDispensers(
        const proto::DraftConfig& draftConfig ) const
{
    DraftCardDispenserSharedPtrVector<DraftCard> dispensers;

    for( int disp = 0; disp < draftConfig.dispensers_size(); ++disp )
    {
        const std::string setCode = draftConfig.dispensers( disp ).set_code();
        std::vector<SlotType> boosterSlots = mAllSetsData->getBoosterSlots( setCode );
        if( boosterSlots.empty() )
        {
            mLogger->error( "set {} does not have booster slots!", setCode );
            return DraftCardDispenserSharedPtrVector<DraftCard>();
        }
        std::multimap<RarityType,std::string> rarityMap = mAllSetsData->getCardPool( setCode );
        if( rarityMap.empty() )
        {
            mLogger->error( "set {} does not have rarities!", setCode );
            return DraftCardDispenserSharedPtrVector<DraftCard>();
        }
        auto pRng = std::shared_ptr<RandGen>( new SimpleRandGen() );
        auto cps = std::make_shared<CardPoolSelector>( rarityMap, pRng );

        auto boosterDisp = std::shared_ptr<DraftCardDispenser<DraftCard>>( new BoosterDispenser( setCode, boosterSlots, cps, mLogger ) );
        dispensers.push_back( boosterDisp );
    }
    return dispensers;
}


std::vector<DraftCard>
CardDispenserFactory::BoosterDispenser::dispense()
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
