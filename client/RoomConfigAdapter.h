#ifndef ROOMCONFIGADAPTER_H
#define ROOMCONFIGADAPTER_H

#include "messages.pb.h"
#include "Logging.h"

class RoomConfigAdapter
{
public:

    // The type of draft based on parsed configuration.
    enum DraftType
    {
        DRAFT_BASIC_BOOSTER,    // 3 rounds of single-pack booster draft
        DRAFT_BASIC_SEALED,     // 1 round of 6-pack sealed draft
        DRAFT_OTHER             // anything else
    };

    // Always created from a protocol buffer message type.
    explicit RoomConfigAdapter( 
        uint32_t                          roomId,
        const thicket::RoomConfiguration& protoBufConfig,
        const Logging::Config&            loggingConfig = Logging::Config() );

    const thicket::RoomConfiguration& getProtoBufConfig() const { return mProtoBufConfig; }

    DraftType getDraftType() const { return mDraftType; }

    uint32_t getRoomId() const { return mRoomId; };
    std::string getName() const { return mProtoBufConfig.name(); };
    uint32_t getChairCount() const { return mProtoBufConfig.chair_count(); }
    uint32_t getBotCount() const { return mProtoBufConfig.bot_count(); }
    bool isPasswordProtected() const { return mProtoBufConfig.password_protected(); }

    uint32_t getRoundCount() const { return mProtoBufConfig.rounds_size(); }

    // Returns three set codes if draft is DRAFT_BASIC_BOOSTER.
    // Returns six set codes if draft is DRAFT_BASIC_SEALED.
    // Return empty otherwise.
    std::vector<std::string> getBasicSetCodes() const;

    // Returns true if valid booster round and clockwise, false otherwise.
    bool isRoundClockwise( unsigned int roundIndex ) const;

    // Returns round time if valid round, 0 otherwise.
    uint32_t getRoundTime( unsigned int roundIndex ) const;

private:

    enum RoundType
    {
        ROUND_BASIC_BOOSTER,    // Booster round with a single booster pack
        ROUND_BASIC_SEALED,     // Sealed round with 6 booster packs
        ROUND_OTHER             // anything else
    };

    RoundType getRoundType( unsigned int roundIndex ) const;

    const uint32_t                         mRoomId;
    const thicket::RoomConfiguration       mProtoBufConfig;
    DraftType                              mDraftType;
    const std::shared_ptr<spdlog::logger>  mLogger;
};

#endif
