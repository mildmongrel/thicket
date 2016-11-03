#ifndef ROOMCONFIGADAPTER_H
#define ROOMCONFIGADAPTER_H

#include "messages.pb.h"
#include "Logging.h"
#include "clienttypes.h"

class RoomConfigAdapter
{
public:

    // Always created from a protocol buffer message type.
    explicit RoomConfigAdapter( 
        uint32_t                 roomId,
        const proto::RoomConfig& roomConfig,
        const Logging::Config&   loggingConfig = Logging::Config() );

    const proto::RoomConfig& getRoomConfig() const { return mRoomConfig; }
    const proto::DraftConfig& getDraftConfig() const { return mRoomConfig.draft_config(); }

    //
    // Accessors for room configuration.
    //

    uint32_t getRoomId() const { return mRoomId; };
    std::string getName() const { return mRoomConfig.name(); };
    uint32_t getBotCount() const { return mRoomConfig.bot_count(); }
    bool isPasswordProtected() const { return mRoomConfig.password_protected(); }

    //
    // Accessors for draft configuration.
    //

    uint32_t getChairCount() const { return mRoomConfig.draft_config().chair_count(); }
    unsigned int getBoosterRoundSelectionTime( unsigned int round ) const;
    PassDirection getPassDirection( unsigned int round ) const;

    //
    // Other methods.
    //

    // Returns true if all rounds are booster.
    bool isBoosterDraft() const;

    // Returns true if all rounds are sealed.
    bool isSealedDraft() const;

    // Get sets involved in the draft, in order of appearance.
    std::vector<std::string> getSetCodes() const;

private:

    const uint32_t                         mRoomId;
    const proto::RoomConfig                mRoomConfig;
    const std::shared_ptr<spdlog::logger>  mLogger;
};

#endif
