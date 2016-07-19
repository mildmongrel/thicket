#include "RoomConfigValidator.h"
#include "DraftConfig.pb.h"
#include <algorithm>


RoomConfigValidator::RoomConfigValidator(
        const std::shared_ptr<const AllSetsData>& allSetsData,
        const Logging::Config&                    loggingConfig )
  : mAllSetsData( allSetsData ),
    mLogger( loggingConfig.createLogger() )
{}


bool
RoomConfigValidator::validate( const proto::RoomConfig& roomConfig, ResultType& failureResult )
{
    const proto::DraftConfig& draftConfig = roomConfig.draft_config();

    //
    // Must have at least two chairs.
    //

    if( draftConfig.chair_count() < 2 )
    {
        mLogger->warn( "Invalid chair count {}", draftConfig.chair_count() );
        failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_CHAIR_COUNT;
        return false;
    }

    //
    // Must have bots fewer bots than chairs.
    //

    if( roomConfig.bot_count() >= draftConfig.chair_count() )
    {
        mLogger->warn( "Invalid bot count {} (chair count {})", roomConfig.bot_count(), draftConfig.chair_count() );
        failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_BOT_COUNT;
        return false;
    }

    //
    // Must have at least one round.
    //

    if( draftConfig.rounds_size() <= 0 )
    {
        mLogger->warn( "Invalid round count {}", draftConfig.rounds_size() );
        failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_ROUND_COUNT;
        return false;
    }

    //
    // Must have at least one dispenser.
    //

    if( draftConfig.dispensers_size() < 1 )
    {
        mLogger->warn( "Invalid card dispensers count {}", draftConfig.dispensers_size() );
        failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_COUNT;
        return false;
    } 
    //
    // All dispensers must have recognizable set codes.
    // Booster method dispensers must use a set with booster specs.
    //

    std::vector<std::string> allSetCodes = mAllSetsData->getSetCodes();
    for( int i = 0; i < draftConfig.dispensers_size(); ++i )
    {
        // Check for valid set code.
        const std::string setCode = draftConfig.dispensers( i ).set_code();

        // OPTIMIZATION: Why isn't AllSetsData returning a set instead of
        // a vector?  Would be a much faster search here.
        if( std::find( allSetCodes.begin(), allSetCodes.end(), setCode ) == allSetCodes.end() )
        {
            mLogger->warn( "Card dispenser {} uses invalid set code {}", i, setCode );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_SET_CODE;
            return false;
        }

        // If booster method, check for support.
        if( draftConfig.dispensers( i ).method() == proto::DraftConfig::CardDispenser::METHOD_BOOSTER )
        {
            if( !mAllSetsData->hasBoosterSlots( setCode ) )
            {
                mLogger->warn( "Card dispenser {} uses non-booster set code {} with booster method", i, setCode );
                failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_CONFIG;
                return false;
            }
        }
    }

    //
    // Currently all rounds must be booster.
    // Each round must have at least one dispensation.
    // All dispensations must point to a valid dispenser index.
    //

    for( int i = 0; i < draftConfig.rounds_size(); ++i )
    {
        if( !draftConfig.rounds( i ).has_booster_round() )
        {
            mLogger->warn( "Draft contains a non-booster round" );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DRAFT_TYPE;
            return false;
        }

        const proto::DraftConfig::BoosterRound boosterRound =
                draftConfig.rounds( i ).booster_round();
        if( boosterRound.dispensations_size() <= 0 )
        {
            mLogger->warn( "Draft round has no dispensers" );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_ROUND_CONFIG;
            return false;
        }

        for( auto d : boosterRound.dispensations() )
        {
            if( (int) d.dispenser_index() >= draftConfig.dispensers_size() )
            {
                mLogger->warn( "Draft round dispensation has an invalid dispenser index {}", d.dispenser_index() );
                failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_ROUND_CONFIG;
                return false;
            }
        }
    }

    return true;
}
