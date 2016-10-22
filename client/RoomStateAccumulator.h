#ifndef ROOMSTATEACCUMULATOR_H
#define ROOMSTATEACCUMULATOR_H

#include <string>
#include <map>

// Current room occupants state information.
class RoomStateAccumulator
{
public:
    void reset()
    {
        mChairCount = 0;
        mPlayerNameMap.clear();
        mPlayerReadyMap.clear();
    }

    int getChairCount() const { return mChairCount; }
    void setChairCount( int chairCount ) { mChairCount = chairCount; }

    bool hasPlayerName( int chair ) const { return mPlayerNameMap.count( chair ) > 0; }
    const std::string& getPlayerName( int chair ) const { return mPlayerNameMap.at( chair ); }
    void setPlayerName( int chair, const std::string& name ) { mPlayerNameMap[chair] = name; }

    bool hasPlayerReady( int chair ) const { return mPlayerReadyMap.count( chair ) > 0; }
    bool getPlayerReady( int chair ) const { return mPlayerReadyMap.at( chair ); }
    void setPlayerReady( int chair, bool ready ) { mPlayerReadyMap[chair] = ready; }

    bool hasPlayerCockatriceHash( int chair ) const { return mPlayerCockatriceHashMap.count( chair ) > 0; }
    const std::string& getPlayerCockatriceHash( int chair ) const { return mPlayerCockatriceHashMap.at( chair ); }
    void setPlayerCockatriceHash( int chair, const std::string& ready ) { mPlayerCockatriceHashMap[chair] = ready; }


private:

    int mChairCount;

    // Maps of chair index to various player data.
    std::map<int,std::string> mPlayerNameMap;
    std::map<int,bool>        mPlayerReadyMap;
    std::map<int,std::string> mPlayerCockatriceHashMap;
};

#endif
