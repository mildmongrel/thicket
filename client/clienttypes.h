#ifndef CLIENTTYPES_H
#define CLIENTTYPES_H

#include <string>
#include <memory>
#include <map>

class AllSetsData;
class CardData;

// Areas cards are moved between.
enum CardZoneType
{
    CARD_ZONE_DRAFT,
    CARD_ZONE_AUTO,
    CARD_ZONE_MAIN,
    CARD_ZONE_SIDEBOARD,
    CARD_ZONE_JUNK,
};

const int CARD_ZONE_TYPE_COUNT = 5;
const std::array<CardZoneType,CARD_ZONE_TYPE_COUNT> gCardZoneTypeArray = {
    CARD_ZONE_DRAFT, CARD_ZONE_AUTO, CARD_ZONE_MAIN, CARD_ZONE_SIDEBOARD, CARD_ZONE_JUNK };

inline std::string
stringify( const CardZoneType& zone )
{
    switch( zone )
    {
        case CARD_ZONE_DRAFT:     return "Draft";
        case CARD_ZONE_AUTO:      return "Auto";
        case CARD_ZONE_MAIN:      return "Main";
        case CARD_ZONE_SIDEBOARD: return "Sideboard";
        case CARD_ZONE_JUNK:      return "Junk";
        default:                  return std::string();
    }
}

// Sort criterion for cards.
enum CardSortCriterionType
{
    CARD_SORT_CRITERION_NONE,
    CARD_SORT_CRITERION_CMC,
    CARD_SORT_CRITERION_COLOR,
    CARD_SORT_CRITERION_NAME,
    CARD_SORT_CRITERION_RARITY,
    CARD_SORT_CRITERION_TYPE
};

// Vector of card sorting criteria.
typedef std::vector<CardSortCriterionType> CardSortCriterionVector;

// Categorization for cards.
enum CardCategorizationType
{
    CARD_CATEGORIZATION_NONE,
    CARD_CATEGORIZATION_CMC,
    CARD_CATEGORIZATION_COLOR,
    CARD_CATEGORIZATION_TYPE,
    CARD_CATEGORIZATION_RARITY
};

// Player states. (partially translates from proto: RoomOccupantsInfoInd::Player::StateType)
enum PlayerStateType
{
    PLAYER_STATE_STANDBY,  // connected, not ready to draft
    PLAYER_STATE_READY,    // connected, ready to draft
    PLAYER_STATE_ACTIVE,   // connected, drafting or finished drafting
    PLAYER_STATE_DEPARTED, // departed (may return)
    PLAYER_STATE_STALE     // player data has gone stale, no longer updated
};

// Pass direction.
enum PassDirection
{
    PASS_DIRECTION_NONE,
    PASS_DIRECTION_CW,
    PASS_DIRECTION_CCW
};

// Card data shared pointers.
typedef std::shared_ptr<CardData> CardDataSharedPtr;

// Set data for room capabilities.
struct RoomCapabilitySetItem
{
    std::string code;
    std::string name;
    bool        boosterGen;
};

// AllSetsData shared pointers.
typedef std::shared_ptr<AllSetsData> AllSetsDataSharedPtr;

#endif
