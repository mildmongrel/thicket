#ifndef SETDATATYPES_H
#define SETDATATYPES_H

#include <string>

enum SlotType
{
    SLOT_COMMON,
    SLOT_UNCOMMON,
    SLOT_RARE,
    SLOT_RARE_OR_MYTHIC_RARE,
    SLOT_TIMESHIFTED_PURPLE
};

inline std::string stringify( const SlotType& slot ) {
    return "unknown";
}

#endif  // SETDATATYPES_H
