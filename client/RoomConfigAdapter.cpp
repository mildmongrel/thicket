#include "RoomConfigAdapter.h"

#include "DraftConfigAdapter.h"

RoomConfigAdapter::RoomConfigAdapter( 
    uint32_t                          roomId,
    const thicket::RoomConfig&        roomConfig,
    const Logging::Config&            loggingConfig )
  : mRoomId( roomId ),
    mRoomConfig( roomConfig ),
    mLogger( loggingConfig.createLogger() )
{
}


bool
RoomConfigAdapter::isBoosterRoundClockwise( unsigned int round ) const
{
    DraftConfigAdapter draftConfigAdapter( mRoomConfig.draft_config() );
    if( draftConfigAdapter.isBoosterRound( round ) )
    {
        proto::DraftConfig::Direction dir = draftConfigAdapter.getBoosterRoundPassDirection( round );
        return (dir == proto::DraftConfig::DIRECTION_CLOCKWISE);
    }
    return false;
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

    for( int i = 0; i < mRoomConfig.draft_config().dispensers_size(); ++i )
    {
        const proto::DraftConfig::CardDispenser& cardDispenser = mRoomConfig.draft_config().dispensers( i );
        setCodes.push_back( cardDispenser.set_code() );
    }

    return setCodes;
}
