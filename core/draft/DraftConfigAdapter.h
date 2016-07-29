#ifndef DRAFTCONFIGADAPTER_H
#define DRAFTCONFIGADAPTER_H

#include "DraftConfig.pb.h"

class DraftConfigAdapter
{
public:

    DraftConfigAdapter( const proto::DraftConfig& draftConfig );

    bool isBoosterRound( unsigned int round ) const;
    bool isSealedRound( unsigned int round ) const;

    unsigned int getBoosterRoundSelectionTime( unsigned int round,
                                               unsigned int defaultVal = 0 ) const;

    proto::DraftConfig::Direction getBoosterRoundPassDirection(
            unsigned int                  round,
            proto::DraftConfig::Direction defaultVal = proto::DraftConfig::DIRECTION_CLOCKWISE ) const;

private:
    const proto::DraftConfig& mDraftConfig;
};

#endif
