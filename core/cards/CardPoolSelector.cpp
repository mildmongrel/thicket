#include "CardPoolSelector.h"

#include <map>
#include <set>

CardPoolSelector::CardPoolSelector( const SetRarityToCardMap& cardPool,
                                    std::shared_ptr<RandGen>& rng,
                                    float                     mythicRareProbability,
                                    const Logging::Config&    loggingConfig )
      : mMythicRareProbability( mythicRareProbability ), 
        mCardPool( cardPool ),
        mRng( rng ),
        mLogger( loggingConfig.createLogger() )
{}


void
CardPoolSelector::resetCardPool()
{
    for( auto item : mCardsRemovedFromPool )
    {
        mLogger->debug( "resetCardPool(): putting back {}", item.second );
        mCardPool.insert( item );
    }
    mCardsRemovedFromPool.clear();
}


bool
CardPoolSelector::selectCard( const SlotType& slot, std::string& selectedCard )
{
    bool result;
    RarityType rarity;
    result = getRarityForSlot( slot, rarity );
    if( !result )
    {
        return false;
    }

    // Find range of items that match desired rarity.
    auto iterPair = mCardPool.equal_range( rarity );

    // If the range is empty then return the default.
    if( iterPair.first == iterPair.second )
    {
        return false;
    }

    // Otherwise return a random selection from the range.
    auto itemIter = iterPair.first;
    const int randAdv = mRng->generateInRange(
            0, std::distance( iterPair.first, iterPair.second ) - 1 );
    std::advance( itemIter, randAdv );
    auto mapItem = *itemIter;
    mCardPool.erase( itemIter );
    mCardsRemovedFromPool.insert( mapItem );
    selectedCard = mapItem.second;

    return true;
}


bool
CardPoolSelector::getRarityForSlot( const SlotType& slot, RarityType& rarity ) const
{
    switch( slot )
    {
        case SLOT_COMMON:
            rarity = RARITY_COMMON;
            break;

        case SLOT_UNCOMMON:
            rarity = RARITY_UNCOMMON;
            break;

        case SLOT_RARE:
            rarity = RARITY_RARE;
            break;

        case SLOT_RARE_OR_MYTHIC_RARE:
            {
            // Roll a die and return the right rarity.
            //float x = std::generate_canonical<float,std::numeric_limits<float>::digits>( mRandEng );
            float x = mRng->generateCanonical();
            mLogger->debug( "getRarityForSlot(): rare/myth rolled {}", x );
            rarity = ( x < mMythicRareProbability ) ? RARITY_MYTHIC_RARE : RARITY_RARE;
            break;
            }

        default:
            return false;
    }

    return true;
}
