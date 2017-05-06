#include "CustomCardListDispenser.h"
#include "SimpleRandGen.h"


CustomCardListDispenser::CustomCardListDispenser(
        const proto::DraftConfig::CardDispenser&  dispenserSpec,
        const proto::DraftConfig::CustomCardList& customCardListSpec,
        const Logging::Config&                    loggingConfig )
  : mValid( false ),
    mLogger( loggingConfig.createLogger() )
{
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


void
CustomCardListDispenser::reset()
{
    std::copy( mCardsDispensed.begin(), mCardsDispensed.end(), std::back_inserter( mCards ) );
    mCardsDispensed.clear();
}


std::vector<DraftCard>
CustomCardListDispenser::dispense( unsigned int qty )
{
    std::vector<DraftCard> cards;

    if( mCards.empty() )
    {
        mLogger->error( "unexpected empty cards!" );
        return cards;
    }

    for( unsigned int i = 0; i < qty; ++i )
    {
        // Choose a random card
        auto iter = mCards.begin();
        SimpleRandGen rng;
        const int randAdv = rng.generateInRange( 0, mCards.size() - 1 );
        std::advance( iter, randAdv );
        DraftCard card = *iter;
        mCards.erase( iter );
        mCardsDispensed.push_back( card );
        cards.push_back( card );

        if( mCards.empty() ) reset();
    }
    return cards;
}

std::vector<DraftCard>
CustomCardListDispenser::dispenseAll()
{
    std::vector<DraftCard> cards;
    std::copy( mCards.begin(), mCards.end(), std::back_inserter( cards ) );
    reset();
    return cards;
}
