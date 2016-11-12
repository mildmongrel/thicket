#include "CardDispenserFactory.h"
#include "BoosterDispenser.h"

// Creates dispensers for a given draft configuration.
// Currently this is very limited:
//  - only does boosters, no other methods covered
//  - always replaces after booster
//  - light error checking, config should be validated beforehand

CardDispenserFactory::CardDispenserFactory( 
        const std::shared_ptr<const AllSetsData>& allSetsData,
        const Logging::Config&                    loggingConfig )
  : mAllSetsData( allSetsData ),
    mLoggingConfig( loggingConfig ),
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
        DraftCardDispenser<DraftCard>* boosterDisp = new BoosterDispenser( draftConfig.dispensers( disp ), mAllSetsData, mLoggingConfig.createChildConfig( "boosterdispenser" ) );
        auto boosterDispSptr = std::shared_ptr<DraftCardDispenser<DraftCard>>( boosterDisp );
        dispensers.push_back( boosterDispSptr );
    }
    return dispensers;
}

