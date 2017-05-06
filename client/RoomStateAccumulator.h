#ifndef ROOMSTATEACCUMULATOR_H
#define ROOMSTATEACCUMULATOR_H

#include <string>
#include <map>
#include <set>

#include "clienttypes.h"

// Current room occupants state information.
class RoomStateAccumulator
{
public:

    void reset()
    {
        mChairCount = 0;
        mPublicDraftType = false;
        mPassDirection = PASS_DIRECTION_NONE;
        mPostRoundTimeRemainingSecs = 0;
        mPlayerNameMap.clear();
        mPlayerStateMap.clear();
        mPlayerPackQueueSizeMap.clear();
        mPublicDraftPlayerActiveSet.clear();
        mPlayerTimeRemainingMap.clear();
        mPlayerCockatriceHashMap.clear();
    }

    int getChairCount() const { return mChairCount; }
    void setChairCount( int chairCount ) { mChairCount = chairCount; }

    bool isPublicDraftType() const { return mPublicDraftType; }
    void setPublicDraftType( bool publicDraftType ) { mPublicDraftType = publicDraftType; }

    PassDirection getPassDirection() const { return mPassDirection; }
    void setPassDirection( PassDirection passDir ) { mPassDirection = passDir; }

    bool hasPostRoundTimeRemainingSecs() const { return mPostRoundTimeRemainingSecs > 0; }
    void setPostRoundTimeRemainingSecs( int seconds ) { mPostRoundTimeRemainingSecs = seconds; }
    int getPostRoundTimeRemainingSecs() const { return mPostRoundTimeRemainingSecs; }

    bool hasPlayerName( int chair ) const { return mPlayerNameMap.count( chair ) > 0; }
    const std::string& getPlayerName( int chair ) const { return mPlayerNameMap.at( chair ); }
    void setPlayerName( int chair, const std::string& name ) { mPlayerNameMap[chair] = name; }

    bool hasPlayerState( int chair ) const { return mPlayerStateMap.count( chair ) > 0; }
    PlayerStateType getPlayerState( int chair ) const { return mPlayerStateMap.at( chair ); }
    void setPlayerState( int chair, PlayerStateType state ) { mPlayerStateMap[chair] = state; }

    bool hasPlayerPackQueueSize( int chair ) const { return mPlayerPackQueueSizeMap.count( chair ) > 0; }
    int getPlayerPackQueueSize( int chair ) const { return mPlayerPackQueueSizeMap.at( chair ); }
    void setPlayerPackQueueSize( int chair, int size ) { mPlayerPackQueueSizeMap[chair] = size; }

    bool isPublicDraftPlayerActive( int chair ) const { return mPublicDraftPlayerActiveSet.count( chair ) > 0; }
    void setPublicDraftPlayerActive( int chair ) { mPublicDraftPlayerActiveSet.insert( chair ); }
    void clearPublicDraftPlayersActive() { mPublicDraftPlayerActiveSet.clear(); }

    bool hasPlayerTimeRemaining( int chair ) const { return mPlayerTimeRemainingMap.count( chair ) > 0; }
    int getPlayerTimeRemaining( int chair ) const { return mPlayerTimeRemainingMap.at( chair ); }
    void setPlayerTimeRemaining( int chair, int time ) { mPlayerTimeRemainingMap[chair] = time; }

    bool hasPlayerCockatriceHash( int chair ) const { return mPlayerCockatriceHashMap.count( chair ) > 0; }
    const std::string& getPlayerCockatriceHash( int chair ) const { return mPlayerCockatriceHashMap.at( chair ); }
    void setPlayerCockatriceHash( int chair, const std::string& ready ) { mPlayerCockatriceHashMap[chair] = ready; }

private:

    int mChairCount;
    bool mPublicDraftType;
    PassDirection mPassDirection;
    int mPostRoundTimeRemainingSecs;

    // Maps of chair index to various player data.
    std::map<int,std::string>     mPlayerNameMap;
    std::map<int,PlayerStateType> mPlayerStateMap;
    std::map<int,int>             mPlayerPackQueueSizeMap;
    std::set<int>                 mPublicDraftPlayerActiveSet;
    std::map<int,int>             mPlayerTimeRemainingMap;
    std::map<int,std::string>     mPlayerCockatriceHashMap;
};

#endif
