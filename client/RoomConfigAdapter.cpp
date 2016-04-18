#include "RoomConfigAdapter.h"


RoomConfigAdapter::RoomConfigAdapter( 
    uint32_t                          roomId,
    const thicket::RoomConfiguration& protoBufConfig,
    const Logging::Config&            loggingConfig )
  : mRoomId( roomId ),
    mProtoBufConfig( protoBufConfig ),
    mLogger( loggingConfig.createLogger() )
{
    if( (mProtoBufConfig.rounds_size() == 3) &&
        (getRoundType( 0 ) == ROUND_BASIC_BOOSTER) &&
        (getRoundType( 1 ) == ROUND_BASIC_BOOSTER) &&
        (getRoundType( 2 ) == ROUND_BASIC_BOOSTER) )
    {
        mDraftType = DRAFT_BASIC_BOOSTER;
    }
    else if( (mProtoBufConfig.rounds_size() == 1) &&
             (getRoundType( 0 ) == ROUND_BASIC_SEALED) )
    {
        mDraftType = DRAFT_BASIC_SEALED;
    }
    else
    {
        mDraftType = DRAFT_OTHER;
    }
}


// Returns three set codes if draft is DRAFT_BASIC_BOOSTER.
// Returns six set codes if draft is DRAFT_BASIC_SEALED.
// Return empty otherwise.
std::vector<std::string>
RoomConfigAdapter::getBasicSetCodes() const
{
    std::vector<std::string> setCodes;
    if( mDraftType == DRAFT_BASIC_BOOSTER )
    {
        setCodes.push_back( mProtoBufConfig.rounds( 0 ).booster_round_config().card_bundles( 0 ).set_code() );
        setCodes.push_back( mProtoBufConfig.rounds( 1 ).booster_round_config().card_bundles( 0 ).set_code() );
        setCodes.push_back( mProtoBufConfig.rounds( 2 ).booster_round_config().card_bundles( 0 ).set_code() );
    }
    else if( mDraftType == DRAFT_BASIC_SEALED )
    {
        setCodes.push_back( mProtoBufConfig.rounds( 0 ).sealed_round_config().card_bundles( 0 ).set_code() );
        setCodes.push_back( mProtoBufConfig.rounds( 0 ).sealed_round_config().card_bundles( 1 ).set_code() );
        setCodes.push_back( mProtoBufConfig.rounds( 0 ).sealed_round_config().card_bundles( 2 ).set_code() );
        setCodes.push_back( mProtoBufConfig.rounds( 0 ).sealed_round_config().card_bundles( 3 ).set_code() );
        setCodes.push_back( mProtoBufConfig.rounds( 0 ).sealed_round_config().card_bundles( 4 ).set_code() );
        setCodes.push_back( mProtoBufConfig.rounds( 0 ).sealed_round_config().card_bundles( 5 ).set_code() );
    }
    return setCodes;
}


// Returns true if valid booster round and clockwise, false otherwise.
bool
RoomConfigAdapter::isRoundClockwise( unsigned int roundIndex ) const
{
    if( (int)roundIndex < mProtoBufConfig.rounds_size() )
    {
        const thicket::RoomConfiguration::Round& round = mProtoBufConfig.rounds( roundIndex );
        if( round.has_booster_round_config() )
        {
            const thicket::RoomConfiguration::BoosterRoundConfiguration& boosterRoundConfig = round.booster_round_config();
            return boosterRoundConfig.clockwise();
        }
        else
        {
            mLogger->warn( "unsupported draft type in round configuration" );
        }
    }
    else
    {
        mLogger->warn( "invalid round index {}", roundIndex );
    }
    return false;
}


// Returns round time if valid round, 0 otherwise.
uint32_t
RoomConfigAdapter::getRoundTime( unsigned int roundIndex ) const
{
    if( (int)roundIndex < mProtoBufConfig.rounds_size() )
    {
        const thicket::RoomConfiguration::Round& round = mProtoBufConfig.rounds( roundIndex );
        if( round.has_booster_round_config() )
        {
            const thicket::RoomConfiguration::BoosterRoundConfiguration& boosterRoundConfig = round.booster_round_config();
            return boosterRoundConfig.time();
        }
        else if( round.has_sealed_round_config() )
        {
            const thicket::RoomConfiguration::SealedRoundConfiguration& sealedRoundConfig = round.sealed_round_config();
            return sealedRoundConfig.time();
        }
        else
        {
            mLogger->warn( "unsupported draft type in round configuration" );
        }
    }
    else
    {
        mLogger->warn( "invalid round index {}", roundIndex );
    }
    return 0;
}


RoomConfigAdapter::RoundType
RoomConfigAdapter::getRoundType( unsigned int roundIndex ) const
{
    if( (int)roundIndex < mProtoBufConfig.rounds_size() )
    {
        const thicket::RoomConfiguration::Round& round = mProtoBufConfig.rounds( roundIndex );
        if( round.has_booster_round_config() )
        {
            const thicket::RoomConfiguration::BoosterRoundConfiguration& boosterRoundConfig = round.booster_round_config();
            if( boosterRoundConfig.card_bundles_size() == 1 )
            {
                const thicket::RoomConfiguration::CardBundle& cardBundle = boosterRoundConfig.card_bundles( 0 );
                if( cardBundle.method() == cardBundle.METHOD_BOOSTER )
                {
                    return ROUND_BASIC_BOOSTER;
                }
            }
        }
        else if( round.has_sealed_round_config() )
        {
            const thicket::RoomConfiguration::SealedRoundConfiguration& sealedRoundConfig = round.sealed_round_config();
            if( sealedRoundConfig.card_bundles_size() == 6 )
            {
                for( int b = 0; b < sealedRoundConfig.card_bundles_size(); ++b )
                {
                    const thicket::RoomConfiguration::CardBundle& cardBundle = sealedRoundConfig.card_bundles( b );
                    if( cardBundle.method() != cardBundle.METHOD_BOOSTER )
                    {
                        return ROUND_OTHER;
                    }
                }
                return ROUND_BASIC_SEALED;
            }
        }
        else
        {
            mLogger->warn( "unsupported draft type in round configuration" );
        }
    }
    else
    {
        mLogger->warn( "invalid round index {}", roundIndex );
    }
    return ROUND_OTHER;
}
