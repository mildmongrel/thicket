#ifndef BOTPLAYER_H
#define BOTPLAYER_H

#include "Player.h"

class BotPlayer : public Player
{
public:
    BotPlayer( int chairIndex )
      : Player( chairIndex )
    {
        setName( "bot " + std::to_string( chairIndex ) );
    }
    virtual ~BotPlayer() {}
};

#endif  // BOTPLAYER_H
