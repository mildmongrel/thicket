#include "RoomConfigAdapter.h"

#include "DraftConfigAdapter.h"

RoomConfigAdapter::RoomConfigAdapter( 
    uint32_t                 roomId,
    const proto::RoomConfig& roomConfig,
    const Logging::Config&   loggingConfig )
  : mRoomId( roomId ),
    mRoomConfig( roomConfig ),
    mLogger( loggingConfig.createLogger() )
{
}


PassDirection
RoomConfigAdapter::getPassDirection( unsigned int round ) const
{
    DraftConfigAdapter draftConfigAdapter( mRoomConfig.draft_config() );
    if( draftConfigAdapter.isBoosterRound( round ) )
    {
        const proto::DraftConfig::Direction dir = draftConfigAdapter.getBoosterRoundPassDirection( round );
        return dir == proto::DraftConfig::DIRECTION_CLOCKWISE ?  PASS_DIRECTION_CW : PASS_DIRECTION_CCW;
    }
    return PASS_DIRECTION_NONE;
}


unsigned int 
RoomConfigAdapter::getBoosterRoundSelectionTime( unsigned int round ) const
{
    DraftConfigAdapter draftConfigAdapter( mRoomConfig.draft_config() );
    return draftConfigAdapter.getBoosterRoundSelectionTime( round );
}


// Returns true if all rounds are booster.
bool
RoomConfigAdapter::isBoosterDraft() const
{
    int round = 0;
    while( round < mRoomConfig.draft_config().rounds_size() )
    {
        const proto::DraftConfig::Round& roundConfig = mRoomConfig.draft_config().rounds( round );
        if( !roundConfig.has_booster_round() ) return false;
        ++round;
    }
    return true;
}


// Returns true if all rounds are sealed.
bool
RoomConfigAdapter::isSealedDraft() const
{
    int round = 0;
    while( round < mRoomConfig.draft_config().rounds_size() )
    {
        const proto::DraftConfig::Round& roundConfig = mRoomConfig.draft_config().rounds( round );
        if( !roundConfig.has_sealed_round() ) return false;
        ++round;
    }
    return true;
}


// Get sets involved in the draft, in order of appearance.
//
// NOTE: Right now this uses a dumb approach of simply adding set codes
// from dispensers, which is not a foolproof way of getting all set codes
// in order since different dispensations may demand different dispensers.
std::vector<std::string>
RoomConfigAdapter::getSetCodes() const
{
    std::vector<std::string> setCodes;

    auto pushBackDispenserCodeFn = [&setCodes,this]( int dispIdx ) {
            if( dispIdx < mRoomConfig.draft_config().dispensers_size() )
            {
                const proto::DraftConfig::CardDispenser& cardDispenser = mRoomConfig.draft_config().dispensers( dispIdx );
                if( cardDispenser.has_set_code() )
                {
                    setCodes.push_back( cardDispenser.set_code() );
                }
                else if( cardDispenser.has_custom_card_list_index() )
                {
                    // Push back a "Superscript 3", AKA cube.
                    setCodes.push_back( "\u00B3" );
                }
            }
        };

    for( int round = 0; round < mRoomConfig.draft_config().rounds_size(); ++round )
    {
        const proto::DraftConfig::Round& roundConfig = mRoomConfig.draft_config().rounds( round );
        if( roundConfig.has_booster_round() )
        {
            const proto::DraftConfig::BoosterRound& boosterRoundConfig =
                    mRoomConfig.draft_config().rounds( round ).booster_round();
            for( int d = 0; d < boosterRoundConfig.dispensations_size(); ++d )
            {
                pushBackDispenserCodeFn( boosterRoundConfig.dispensations( d ).dispenser_index() );
            }
        }
        if( roundConfig.has_sealed_round() )
        {
            const proto::DraftConfig::SealedRound& sealedRoundConfig =
                    mRoomConfig.draft_config().rounds( round ).sealed_round();
            for( int d = 0; d < sealedRoundConfig.dispensations_size(); ++d )
            {
                pushBackDispenserCodeFn( sealedRoundConfig.dispensations( d ).dispenser_index() );
            }
        }
    }

    return setCodes;
}
