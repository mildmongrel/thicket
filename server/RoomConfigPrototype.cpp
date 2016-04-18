#include "RoomConfigPrototype.h"

#include "CardPoolSelector.h"
#include "SimpleRandGen.h"

RoomConfigPrototype::RoomConfigPrototype( 
        const std::shared_ptr<const AllSetsData>& allSetsData,
        const thicket::RoomConfiguration&         protoBufConfig,
        const std::string&                        password,
        const Logging::Config&                    loggingConfig )
  : mAllSetsData( allSetsData ),
    mProtoBufConfig( protoBufConfig ),
    mPassword( password ),
    mStatus( checkStatus() ),
    mLogger( loggingConfig.createLogger() )
{
}


// This entire method uses a very narrow scope of the room configuration
// protocol, assuming the same time for every round, 1 booster per pack,
// ignoring sealed configuration.
std::vector<DraftRoundConfigurationType>
RoomConfigPrototype::generateDraftRoundConfigs() const
{
    std::vector<DraftRoundConfigurationType> roundConfigs;
    uint32_t roundTimer = 0;

    std::vector<std::string> setCodes;
    for( int r = 0; r < mProtoBufConfig.rounds_size(); ++r )
    {
        const thicket::RoomConfiguration::Round& round = mProtoBufConfig.rounds( r );

        if( round.has_booster_round_config() )
        {
            const thicket::RoomConfiguration::BoosterRoundConfiguration& boosterRoundConfig =
                    round.booster_round_config();

            // Protocol allows for more but for now just use the first cardpoolgen element.
            // Also ignore any other settings in the element - createRoundConfigurations()
            // assumes its a booster.
            const thicket::RoomConfiguration::CardBundle& bundle =
                    boosterRoundConfig.card_bundles( 0 );
            setCodes.push_back( bundle.set_code() );

            roundTimer = boosterRoundConfig.time();
        }
    }

    // Create round configurations.
    roundConfigs = createRoundConfigurations( setCodes, getChairCount(), roundTimer );

    return roundConfigs;
}


RoomConfigPrototype::StatusType
RoomConfigPrototype::checkStatus() const
{
    if( mProtoBufConfig.chair_count() <= 0 )
    {
        return STATUS_BAD_CHAIR_COUNT;
    }

    if( mProtoBufConfig.bot_count() >= mProtoBufConfig.chair_count() )
    {
        return STATUS_BAD_BOT_COUNT;
    }

    if( mProtoBufConfig.rounds_size() <= 0 )
    {
        return STATUS_BAD_ROUND_COUNT;
    }

    for( int r = 0; r < mProtoBufConfig.rounds_size(); ++r )
    {
        const thicket::RoomConfiguration::Round& round = mProtoBufConfig.rounds( r );

        if( round.has_booster_round_config() )
        {
            const thicket::RoomConfiguration::BoosterRoundConfiguration& boosterRoundConfig =
                    round.booster_round_config();

            if( boosterRoundConfig.card_bundles_size() <= 0 )
            {
                return STATUS_BAD_ROUND_CONFIG;
            }

            // Validate set codes.
            const std::vector<std::string> allSetCodes = mAllSetsData->getSetCodes();
            for( int b = 0; b < boosterRoundConfig.card_bundles_size(); ++b )
            {
                const thicket::RoomConfiguration::CardBundle& bundle =
                        boosterRoundConfig.card_bundles( b );
                if( !mAllSetsData->hasBoosterSlots( bundle.set_code() ) )
                {
                    return STATUS_BAD_SET_CODE;
                }
            }
        }
        else
        {
            return STATUS_BAD_DRAFT_TYPE;
        }
    }

    return STATUS_OK;
}


std::vector<DraftRoundConfigurationType>
RoomConfigPrototype::createRoundConfigurations(
        const std::vector<std::string>& setCodes,
        int                             chairs,
        int                             roundTimeoutTicks ) const
{
    std::vector<DraftType::RoundConfiguration> roundConfigs;
    int packId = 0;

    for( std::size_t round = 0; round < setCodes.size(); ++round )
    {
        std::vector<SlotType> boosterSlots = mAllSetsData->getBoosterSlots( setCodes[round] );
        std::multimap<RarityType,std::string> rarityMap;
        rarityMap = mAllSetsData->getCardPool( setCodes[round] );
        auto pRng = std::shared_ptr<RandGen>( new SimpleRandGen() );
        CardPoolSelector cps( rarityMap, pRng );

        DraftRoundConfigurationType roundConfig( (DraftRoundInfo()) );
        roundConfig.setTimeoutTicks( roundTimeoutTicks );
        roundConfig.setPassDirection( (round % 2 == 0) ? DraftType::CLOCKWISE : DraftType::COUNTERCLOCKWISE );
        for( int chair = 0; chair < chairs; ++chair )
        {
            mLogger->debug( "generating pack: round={} chair={}", round, chair );
            std::vector<DraftCard> packCards;
            for( std::vector<SlotType>::size_type i = 0; i < boosterSlots.size(); ++i )
            {
                std::string selectedCard;
                bool result = cps.selectCard( boosterSlots[i], selectedCard );
                if( result )
                {
                    packCards.push_back( DraftCard( selectedCard, setCodes[round] ) );
                }
                else
                {
                    mLogger->warn( "error selecting card: round={} chair={} setCode={} slotIndex={}",
                            round, chair, setCodes[round], i );
                }
            }
            cps.resetCardPool();

            roundConfig.setPack( chair, packId++, packCards );
        }
        roundConfigs.push_back( roundConfig );
    }
    return roundConfigs;
}
