#ifndef CLIENTPROTOHELPER_H
#define CLIENTPROTOHELPER_H

#include "messages.pb.h"
#include "clienttypes.h"

const int INVENTORY_ZONE_TYPE_COUNT = 4;
const std::array<proto::Zone,INVENTORY_ZONE_TYPE_COUNT> gInventoryZoneArray = {
    proto::ZONE_AUTO, proto::ZONE_MAIN, proto::ZONE_SIDEBOARD, proto::ZONE_JUNK };

//
// For these conversion functions, ignore return-type warnings.  Returns
// happen from within each case of a switch statement; there should not
// be a default value that is returned if a case isn't hit.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"


inline proto::Zone convertCardZone( const CardZoneType& z )
{
    switch( z )
    {
        case CARD_ZONE_AUTO:      return proto::ZONE_AUTO;
        case CARD_ZONE_MAIN:      return proto::ZONE_MAIN;
        case CARD_ZONE_SIDEBOARD: return proto::ZONE_SIDEBOARD;
        case CARD_ZONE_JUNK:      return proto::ZONE_JUNK;

        // These zones should not be converted, but if they are somehow,
        // map them to junk.
        case CARD_ZONE_BOOSTER_DRAFT: return proto::ZONE_JUNK;
        case CARD_ZONE_GRID_DRAFT:    return proto::ZONE_JUNK;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline CardZoneType convertCardZone( const proto::Zone& z )
{
    switch( z )
    {
        case proto::ZONE_AUTO:      return CARD_ZONE_AUTO;
        case proto::ZONE_MAIN:      return CARD_ZONE_MAIN;
        case proto::ZONE_SIDEBOARD: return CARD_ZONE_SIDEBOARD;
        case proto::ZONE_JUNK:      return CARD_ZONE_JUNK;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


inline PlayerStateType convertPlayerState( const proto::RoomOccupantsInfoInd::Player::StateType state )
{
    switch( state )
    {
        case proto::RoomOccupantsInfoInd::Player::STATE_STANDBY: return PLAYER_STATE_STANDBY;
        case proto::RoomOccupantsInfoInd::Player::STATE_READY: return PLAYER_STATE_READY;
        case proto::RoomOccupantsInfoInd::Player::STATE_ACTIVE: return PLAYER_STATE_ACTIVE;
        case proto::RoomOccupantsInfoInd::Player::STATE_DEPARTED: return PLAYER_STATE_DEPARTED;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


#pragma GCC diagnostic pop


#endif
