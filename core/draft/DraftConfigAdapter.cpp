#include "DraftConfigAdapter.h"

DraftConfigAdapter::DraftConfigAdapter( const proto::DraftConfig& draftConfig )
  : mDraftConfig( draftConfig )
{}


bool
DraftConfigAdapter::isBoosterRound( unsigned int round ) const
{
    if( (int)round >= mDraftConfig.rounds_size() ) return false;
    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( round );
    if( !roundConfig.has_booster_round() ) return false;
    return true;
}


bool
DraftConfigAdapter::isSealedRound( unsigned int round ) const
{
    if( (int)round >= mDraftConfig.rounds_size() ) return false;
    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( round );
    if( !roundConfig.has_sealed_round() ) return false;
    return true;
}


bool
DraftConfigAdapter::isGridRound( unsigned int round ) const
{
    if( (int)round >= mDraftConfig.rounds_size() ) return false;
    const proto::DraftConfig::Round& roundConfig = mDraftConfig.rounds( round );
    if( !roundConfig.has_grid_round() ) return false;
    return true;
}


unsigned int
DraftConfigAdapter::getBoosterRoundSelectionTime( unsigned int round,
                                                  unsigned int defaultVal ) const
{
    if( !isBoosterRound( round ) ) return defaultVal;
    const proto::DraftConfig::BoosterRound& boosterRoundConfig =
            mDraftConfig.rounds( round ).booster_round();
    return boosterRoundConfig.has_selection_time() ?
            boosterRoundConfig.selection_time() : defaultVal;
}


proto::DraftConfig::Direction
DraftConfigAdapter::getBoosterRoundPassDirection(
        unsigned int                  round,
        proto::DraftConfig::Direction defaultVal ) const
{
    if( !isBoosterRound( round ) ) return defaultVal;
    const proto::DraftConfig::BoosterRound& boosterRoundConfig =
            mDraftConfig.rounds( round ).booster_round();
    return boosterRoundConfig.pass_direction();
}


unsigned int
DraftConfigAdapter::getGridRoundSelectionTime( unsigned int round,
                                               unsigned int defaultVal ) const
{
    if( !isGridRound( round ) ) return defaultVal;
    const proto::DraftConfig::GridRound& roundConfig =
            mDraftConfig.rounds( round ).grid_round();
    return roundConfig.has_selection_time() ?
            roundConfig.selection_time() : defaultVal;
}


