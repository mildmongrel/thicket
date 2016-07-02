#ifndef DRAFTCONFIGADAPTER_H
#define DRAFTCONFIGADAPTER_H

#include "DraftConfig.pb.h"

class DraftConfigAdapter
{
public:
    DraftConfigAdapter( const DraftConfig& draftConfig )
      : mDraftConfig( draftConfig )
    {}

    bool isBoosterRound( unsigned int round ) const
    {
        if( (int)round >= mDraftConfig.rounds_size() ) return false;
        const DraftConfig::Round& roundConfig = mDraftConfig.rounds( round );
        if( !roundConfig.has_booster_round() ) return false;
        return true;
    }

    unsigned int getBoosterRoundSelectionTime( unsigned int round,
                                               unsigned int defaultVal = 0 ) const
    {
        if( !isBoosterRound( round ) ) return defaultVal;
        const DraftConfig::BoosterRound& boosterRoundConfig =
                mDraftConfig.rounds( round ).booster_round();
        return boosterRoundConfig.has_selection_time() ?
                boosterRoundConfig.selection_time() : defaultVal;
    }

    DraftConfig::Direction getBoosterRoundPassDirection(
            unsigned int           round,
            DraftConfig::Direction defaultVal = DraftConfig::DIRECTION_CLOCKWISE ) const
    {
        if( !isBoosterRound( round ) ) return defaultVal;
        const DraftConfig::BoosterRound& boosterRoundConfig =
                mDraftConfig.rounds( round ).booster_round();
        return boosterRoundConfig.pass_direction();
    }

private:
    const DraftConfig& mDraftConfig;
};

#endif
