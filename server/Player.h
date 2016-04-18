#ifndef PLAYER_H
#define PLAYER_H

#include "DraftTypes.h"

class Player : public DraftChairObserverType
{
public:

    Player( int chairIndex, const std::string& name = "" )
      : DraftChairObserverType( chairIndex ), mName( name ) {}
    virtual ~Player() {}

    virtual std::string getName() { return mName; }
    virtual void setName( const std::string& name ) { mName = name; }

protected:

    std::string mName;

};

#endif  // PLAYER_H
