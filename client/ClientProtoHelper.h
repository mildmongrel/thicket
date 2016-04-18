#ifndef CLIENTPROTOHELPER_H
#define CLIENTPROTOHELPER_H

#include "messages.pb.h"
#include "clienttypes.h"


//
// For these conversion functions, ignore return-type warnings.  Returns
// happen from within each case of a switch statement; there should not
// be a default value that is returned if a case isn't hit.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"


inline thicket::Zone convertCardZone( const CardZoneType& z )
{
    switch( z )
    {
        case CARD_ZONE_MAIN:      return thicket::ZONE_MAIN;
        case CARD_ZONE_SIDEBOARD: return thicket::ZONE_SIDEBOARD;
        case CARD_ZONE_JUNK:      return thicket::ZONE_JUNK;

        // These zones should not be converted, but if they are somehow,
        // map them to junk.
        case CARD_ZONE_DRAFT:     return thicket::ZONE_JUNK;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};

inline CardZoneType convertCardZone( const thicket::Zone& z )
{
    switch( z )
    {
        case thicket::ZONE_MAIN:      return CARD_ZONE_MAIN;
        case thicket::ZONE_SIDEBOARD: return CARD_ZONE_SIDEBOARD;
        case thicket::ZONE_JUNK:      return CARD_ZONE_JUNK;

        // Intentionally no default; compile-time error if enum omitted in switch.
    }
};


#pragma GCC diagnostic pop


#endif
