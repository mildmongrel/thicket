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
    // Must have at least one chair.
    //

    if( draftConfig.chair_count() < 1 )
    {
        mLogger->warn( "Invalid chair count {}", draftConfig.chair_count() );
        failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_CHAIR_COUNT;
        return false;
    }

    //
    // Must have fewer bots than chairs.
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
    // Check dispensers.
    // Set dispensers must have recognizable set codes.
    // Booster method dispensers must use a set with booster specs.
    // Custom card list dispensers must have a nonzero amount of cards
    //

    std::vector<std::string> allSetCodes = mAllSetsData->getSetCodes();
    for( int i = 0; i < draftConfig.dispensers_size(); ++i )
    {
        if( draftConfig.dispensers( i ).has_set_code() )
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
        else if( draftConfig.dispensers( i ).has_custom_card_list_index() )
        {
            const int cclIndex = draftConfig.dispensers( i ).custom_card_list_index();
            if( cclIndex >= draftConfig.custom_card_lists_size() )
            {
                mLogger->warn( "Card dispenser {} uses invalid custom card list index", i );
                failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_CONFIG;
                return false;
            }

            if( draftConfig.dispensers( i ).method() != proto::DraftConfig::CardDispenser::METHOD_SINGLE_RANDOM )
            {
                mLogger->warn( "Card dispenser {} uses invalid replacement", i );
                failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_CONFIG;
                return false;
            }

            if( draftConfig.dispensers( i ).replacement() != proto::DraftConfig::CardDispenser::REPLACEMENT_UNDERFLOW_ONLY )
            {
                mLogger->warn( "Card dispenser {} uses invalid replacement", i );
                failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_CONFIG;
                return false;
            }
        }
        else
        {
            mLogger->warn( "Card dispenser {} uses unknown source", i );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DISPENSER_CONFIG;
            return false;
        }
    }

    //
    // Check custom card lists.  Must have non-zero amount of cards.
    //

    for( int i = 0; i < draftConfig.custom_card_lists_size(); ++i )
    {
        const proto::DraftConfig::CustomCardList& ccl = draftConfig.custom_card_lists( i );
        if( ccl.card_quantities_size() == 0 )
        {
            mLogger->warn( "Custom card list {} has no card quantity entries", i );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_CUSTOM_CARD_LIST;
            return false;
        }

        int qty = 0;
        for( int j = 0; j < ccl.card_quantities_size(); ++j )
        {
            const proto::DraftConfig::CustomCardList::CardQuantity& cq = ccl.card_quantities( i );
            qty += cq.quantity();
        }
        if( qty <= 0 )
        {
            mLogger->warn( "Custom card list {} has no cards", i );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_CUSTOM_CARD_LIST;
            return false;
        }
    }

    //
    // Currently all rounds must be booster or sealed, and must be of the same type.
    // Each round must have at least one dispensation.
    // All dispensations must point to a valid dispenser index.
    //

    bool booster = false;
    for( int i = 0; i < draftConfig.rounds_size(); ++i )
    {
        const proto::DraftConfig::Round& round = draftConfig.rounds( i );

        using CardDispensationsType = google::protobuf::RepeatedPtrField<proto::DraftConfig::CardDispensation>;
        CardDispensationsType dispensations;

        if( round.has_booster_round() )
        {
            if( (i > 0) && !booster )
            {
                mLogger->warn( "Booster draft contains a non-booster round" );
                failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DRAFT_TYPE;
                return false;
            }
            else
            {
                booster = true;
            }
            dispensations = round.booster_round().dispensations();
        }
        else if( round.has_sealed_round() )
        {
            if( (i > 0) )
            {
                mLogger->warn( "Sealed draft may only have one round" );
                failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DRAFT_TYPE;
                return false;
            }
            dispensations = round.sealed_round().dispensations();
        }
        else
        {
            mLogger->warn( "Draft contains a non-booster round" );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_DRAFT_TYPE;
            return false;
        }

        if( dispensations.size() <= 0 )
        {
            mLogger->warn( "Draft round has no dispensers" );
            failureResult = proto::CreateRoomFailureRsp::RESULT_INVALID_ROUND_CONFIG;
            return false;
        }

        for( auto d : dispensations )
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
