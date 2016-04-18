#ifndef ROOMCONFIGPROTOTYPE_H
#define ROOMCONFIGPROTOTYPE_H

#include "messages.pb.h"
#include "DraftTypes.h"
#include "AllSetsData.h"

class RoomConfigPrototype
{
public:

    // Always created from a protocol buffer message type.
    explicit RoomConfigPrototype( 
            const std::shared_ptr<const AllSetsData>& allSetsData,
            const thicket::RoomConfiguration&         protoBufConfig,
            const std::string&                        password,
            const Logging::Config&                    loggingConfig = Logging::Config() );

    const thicket::RoomConfiguration& getProtoBufConfig() const { return mProtoBufConfig; }

    enum StatusType
    {
        STATUS_OK,
        STATUS_BAD_CHAIR_COUNT,
        STATUS_BAD_BOT_COUNT,
        STATUS_BAD_ROUND_COUNT,
        STATUS_BAD_DRAFT_TYPE,
        STATUS_BAD_ROUND_CONFIG,
        STATUS_BAD_SET_CODE
    };

    StatusType getStatus() const { return mStatus; }

    std::string getRoomName() const { return mProtoBufConfig.name(); };
    std::string getPassword() const { return mPassword; }
    int getChairCount() const { return mProtoBufConfig.chair_count(); }
    int getBotCount() const { return mProtoBufConfig.bot_count(); }

    // Returns empty vector if not valid.
    std::vector<DraftRoundConfigurationType> generateDraftRoundConfigs() const;

private:

    StatusType checkStatus() const;

    std::vector<DraftRoundConfigurationType> createRoundConfigurations(
            const std::vector<std::string>& setCodes,
            int                             chairs,
            int                             roundTimeoutTicks ) const;

    std::shared_ptr<const AllSetsData>  mAllSetsData;
    const thicket::RoomConfiguration    mProtoBufConfig;
    const std::string                   mPassword;
    const StatusType                    mStatus;

    std::shared_ptr<spdlog::logger>     mLogger;
};

#endif
