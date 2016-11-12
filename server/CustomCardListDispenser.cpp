#include "CustomCardListDispenser.h"
#include "SimpleRandGen.h"


CustomCardListDispenser::CustomCardListDispenser(
        const proto::DraftConfig::CardDispenser&  dispenserSpec,
        const proto::DraftConfig::CustomCardList& customCardListSpec,
        const Logging::Config&                    loggingConfig )
  : mValid( false ),
    mLogger( loggingConfig.createLogger() )
{
    if( dispenserSpec.method() != proto::DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM )
    {
        mLogger->error( "invalid dispenser method (expected single random)" );
        return;
    }

    if( dispenserSpec.replacement() != proto::DraftConfig::CardDispenser::REPLACEMENT_UNDERFLOW_ONLY )
    {
        mLogger->error( "invalid replacement method (expected underflow only)" );
        return;
    }

    // Create pool of cards.
    for( int i = 0; i < customCardListSpec.card_quantities_size(); ++i )
    {
        const proto::DraftConfig::CustomCardList::CardQuantity& cardQty = customCardListSpec.card_quantities( i );
        DraftCard dc( cardQty.name(), cardQty.set_code() );
        mCards.insert( mCards.end(), cardQty.quantity(), dc );
    }

    if( mCards.empty() )
    {
        mLogger->error( "empty custom card list" );
        return;
    }

    mValid = true;
}


std::vector<DraftCard>
CustomCardListDispenser::dispense()
{
    std::vector<DraftCard> v;
    if( !mValid ) return v;

    // Refill pool from dispensed if pool is empty.
    if( mCards.empty() )
    {
        mLogger->debug( "refilling custom card pool" );
        mCards.swap( mCardsDispensed );
    }

    // Choose a random card
    auto iter = mCards.begin();
    SimpleRandGen rng;
    const int randAdv = rng.generateInRange( 0, mCards.size() - 1 );
    std::advance( iter, randAdv );
    DraftCard card = *iter;
    mCards.erase( iter );
    mCardsDispensed.push_back( card );

    v.push_back( card );
    return v;
}

