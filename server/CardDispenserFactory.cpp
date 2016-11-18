#include "CardDispenserFactory.h"
#include "BoosterDispenser.h"
#include "CustomCardListDispenser.h"

// Creates dispensers for a given draft configuration.
// The configuration should be validated before creating dispensers.

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

    for( int d = 0; d < draftConfig.dispensers_size(); ++d )
    {
        const proto::DraftConfig::CardDispenser& disp = draftConfig.dispensers( d );
        if( disp.has_set_code() )
        {
            BoosterDispenser* boosterDisp = new BoosterDispenser( disp, mAllSetsData, mLoggingConfig.createChildConfig( "boosterdispenser" ) );
            if( boosterDisp->isValid() )
            {
                auto sptr = std::shared_ptr<DraftCardDispenser<DraftCard>>( boosterDisp );
                dispensers.push_back( sptr );
            }
            else
            {
                mLogger->error( "booster dispenser invalid!" );
                return DraftCardDispenserSharedPtrVector<DraftCard>();
            }
        }
        else if( disp.has_custom_card_list_index() )
        {
            const int cclIndex = disp.custom_card_list_index();
            if( cclIndex < draftConfig.custom_card_lists_size() )
            {
                const proto::DraftConfig::CustomCardList& ccl = draftConfig.custom_card_lists( cclIndex );
                CustomCardListDispenser* cclDisp = new CustomCardListDispenser( disp, ccl, mLoggingConfig.createChildConfig( "ccldispenser" ) );
                if( cclDisp->isValid() )
                {
                    auto sptr = std::shared_ptr<DraftCardDispenser<DraftCard>>( cclDisp );
                    dispensers.push_back( sptr );
                }
                else
                {
                    mLogger->error( "custom card list dispenser invalid!" );
                    return DraftCardDispenserSharedPtrVector<DraftCard>();
                }
            }
            else
            {
                mLogger->error( "invalid custom card list index! ({})", cclIndex );
                return DraftCardDispenserSharedPtrVector<DraftCard>();
            }
        }
        else
        {
            mLogger->error( "unknown dispenser type!" );
            return DraftCardDispenserSharedPtrVector<DraftCard>();
        }
    }
    return dispensers;
}

