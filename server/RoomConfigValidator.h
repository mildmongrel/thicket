#ifndef ROOMCONFIGVALIDATOR_H
#define ROOMCONFIGVALIDATOR_H

#include "messages.pb.h"
#include "AllSetsData.h"
#include "Logging.h"
#include <memory>

//
// This class validates a room configuration for compatibility with
// the current capabilities of the server.  More configurations may
// be *legal*, but if the server and draft won't work, validate()
// should fail.
//

class RoomConfigValidator
{
public:

    using ResultType = thicket::CreateRoomFailureRsp_ResultType;

    // Always created from a protocol buffer message type.
    explicit RoomConfigValidator( 
            const std::shared_ptr<const AllSetsData>& allSetsData,
            const Logging::Config&                    loggingConfig = Logging::Config() );

    bool validate( const thicket::RoomConfig& roomConfig, ResultType& failureResult );

private:

    std::shared_ptr<const AllSetsData>  mAllSetsData;
    std::shared_ptr<spdlog::logger>     mLogger;
};

#endif
