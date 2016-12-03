#include "catch.hpp"
#include "Logging.h"
#include "MtgJsonAllSetsData.h"
#include <vector>


void initAllSetsData( MtgJsonAllSetsData& allSets )
{
    const std::string allSetsDataFilename = "AllSets.json";
    FILE* allSetsDataFile = fopen( allSetsDataFilename.c_str(), "r" );
    if( allSetsDataFile == NULL )
    {
        CATCH_FAIL( "Failed to open AllSets.json: it must be co-located with the test executable." );
    }

    bool parseResult = allSets.parse( allSetsDataFile );
    fclose( allSetsDataFile );

    CATCH_REQUIRE( parseResult );
}


const MtgJsonAllSetsData& getAllSetsDataInstance()
{
    static MtgJsonAllSetsData allSets;
    static bool initialized = false;
    if( !initialized )
    {
        initAllSetsData( allSets );
        initialized = true;
    }

    return allSets;
}


// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check set codes", "[mtgjson]" )
{
    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();
    std::vector<std::string> setCodes = allSets.getSetCodes();

    // As of Oct 2015 there are 187 sets.
    CATCH_REQUIRE( setCodes.size() > 185 );
    CATCH_REQUIRE( setCodes.size() < 300 );
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check set names", "[mtgjson]" )
{
    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();

    CATCH_REQUIRE( allSets.getSetName( "" ) == "" );
    CATCH_REQUIRE( allSets.getSetName( "LEA" ) == "Limited Edition Alpha" );
    CATCH_REQUIRE( allSets.getSetName( "10E" ) == "Tenth Edition" );
    CATCH_REQUIRE( allSets.getSetName( "INV" ) == "Invasion" );
    CATCH_REQUIRE( allSets.getSetName( "THS" ) == "Theros" );
    CATCH_REQUIRE( allSets.getSetName( "KLD" ) == "Kaladesh" );
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check booster slots", "[mtgjson]" )
{
    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();
    std::vector<SlotType> boosterSlots;

    CATCH_REQUIRE_FALSE( allSets.hasBoosterSlots( "" ) );
    boosterSlots = allSets.getBoosterSlots( "" );
    CATCH_REQUIRE( boosterSlots.size() == 0 );

    CATCH_REQUIRE( allSets.hasBoosterSlots( "LEA" ) );
    boosterSlots = allSets.getBoosterSlots( "LEA" );
    CATCH_REQUIRE( boosterSlots.size() == 15 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(),SLOT_RARE ) == 1 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_UNCOMMON ) == 3 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_COMMON ) == 11 );

    CATCH_REQUIRE( allSets.hasBoosterSlots( "INV" ) );
    boosterSlots = allSets.getBoosterSlots( "INV" );
    CATCH_REQUIRE( boosterSlots.size() == 15 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_RARE ) == 1 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_UNCOMMON ) == 3 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_COMMON ) == 11 );

    CATCH_REQUIRE( allSets.hasBoosterSlots( "THS" ) );
    boosterSlots = allSets.getBoosterSlots( "THS" );
    CATCH_REQUIRE( boosterSlots.size() == 14 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_RARE_OR_MYTHIC_RARE ) == 1 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_UNCOMMON ) == 3 );
    CATCH_REQUIRE( std::count( boosterSlots.begin(), boosterSlots.end(), SLOT_COMMON ) == 10 );
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check rarity maps", "[mtgjson]" )
{
    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();
    std::multimap<RarityType,std::string> rarityMap;

    rarityMap = allSets.getCardPool( "LEA" );
    CATCH_REQUIRE( rarityMap.size() == 295 );
    CATCH_REQUIRE( rarityMap.count( RARITY_BASIC_LAND ) == 10 );
    CATCH_REQUIRE( rarityMap.count( RARITY_COMMON ) == 74 );
    CATCH_REQUIRE( rarityMap.count( RARITY_UNCOMMON ) == 95 );
    CATCH_REQUIRE( rarityMap.count( RARITY_RARE ) == 116 );

    // FEM has multiple-art commons.
    rarityMap = allSets.getCardPool( "FEM" );
    CATCH_REQUIRE( rarityMap.size() == 187 );
    CATCH_REQUIRE( rarityMap.count( RARITY_COMMON ) == 121 );
    CATCH_REQUIRE( rarityMap.count( RARITY_UNCOMMON ) == 30 );
    CATCH_REQUIRE( rarityMap.count( RARITY_RARE ) == 36 );

    // INV has split cards.
    rarityMap = allSets.getCardPool( "INV" );
    CATCH_REQUIRE( rarityMap.size() == 350 );
    CATCH_REQUIRE( rarityMap.count( RARITY_BASIC_LAND ) == 20 );
    CATCH_REQUIRE( rarityMap.count( RARITY_COMMON ) == 110 );
    CATCH_REQUIRE( rarityMap.count( RARITY_UNCOMMON ) == 110 );
    CATCH_REQUIRE( rarityMap.count( RARITY_RARE ) == 110 );

    // CHK has flip cards and Brothers Yamazaki, which has two variants.
    rarityMap = allSets.getCardPool( "CHK" );
    CATCH_REQUIRE( rarityMap.size() == 306 );
    CATCH_REQUIRE( rarityMap.count( RARITY_BASIC_LAND ) == 20 );
    CATCH_REQUIRE( rarityMap.count( RARITY_COMMON ) == 110 );
    CATCH_REQUIRE( rarityMap.count( RARITY_UNCOMMON ) == 88 );
    CATCH_REQUIRE( rarityMap.count( RARITY_RARE ) == 88 );

    rarityMap = allSets.getCardPool( "THS" );
    CATCH_REQUIRE( rarityMap.size() == 249 );
    CATCH_REQUIRE( rarityMap.count( RARITY_BASIC_LAND ) == 20 );
    CATCH_REQUIRE( rarityMap.count( RARITY_COMMON ) == 101 );
    CATCH_REQUIRE( rarityMap.count( RARITY_UNCOMMON ) == 60 );
    CATCH_REQUIRE( rarityMap.count( RARITY_RARE ) == 53 );
    CATCH_REQUIRE( rarityMap.count( RARITY_MYTHIC_RARE ) == 15 );

    rarityMap = allSets.getCardPool( "BNG" );
    CATCH_REQUIRE( rarityMap.size() == 165 );
    CATCH_REQUIRE( rarityMap.count( RARITY_COMMON ) == 60 );
    CATCH_REQUIRE( rarityMap.count( RARITY_UNCOMMON ) == 60 );
    CATCH_REQUIRE( rarityMap.count( RARITY_RARE ) == 35 );
    CATCH_REQUIRE( rarityMap.count( RARITY_MYTHIC_RARE ) == 10 );
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Ensure every card can be created", "[mtgjson]" )
{
    std::cout << "Creating every card is every set:" << std::endl;

    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();
    const std::vector<std::string> setCodes = allSets.getSetCodes();
    for( auto set : allSets.getSetCodes() )
    {
        std::multimap<RarityType,std::string> rarityMap = allSets.getCardPool( set );
        std::cout << "  " << set << std::endl;
        for( const auto& kv : rarityMap )
        {
            CardData* c = allSets.createCardData( set, kv.second );
            CATCH_REQUIRE( c != 0 );
            delete c;
        }
    }
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check set code lookup by card name", "[mtgjson]" )
{
    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();

    CATCH_REQUIRE( allSets.findSetCode( "Black Lotus" ) == "2ED" );
    CATCH_REQUIRE( allSets.findSetCode( "No Such Card" ) == "" );
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check CardData created from set and name", "[mtgjson]" )
{
    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();
    CardData* c;
    std::set<ColorType> colors;
    std::set<std::string> types;

    CATCH_SECTION( "Invalid parameters" )
    {
        CATCH_REQUIRE( allSets.createCardData( "", "" ) == nullptr );
        CATCH_REQUIRE( allSets.createCardData( "LEA", "" ) == nullptr );
    }

    CATCH_SECTION( "Same name, different set" )
    {
        c = allSets.createCardData( "LEA", "Lightning Bolt" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "LEA" );
        CATCH_REQUIRE( c->getName() == "Lightning Bolt" );
        CATCH_REQUIRE( c->getMultiverseId() == 209 );
        CATCH_REQUIRE( c->getCMC() == 1 );
        CATCH_REQUIRE( c->getRarity() == RARITY_COMMON );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 1 );
        CATCH_REQUIRE( colors.count( COLOR_RED ) );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Instant" ) );
        delete c;

        c = allSets.createCardData( "3ED", "Lightning Bolt" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "3ED" );
        CATCH_REQUIRE( c->getName() == "Lightning Bolt" );
        CATCH_REQUIRE( c->getMultiverseId() == 1303 );
        CATCH_REQUIRE( c->getCMC() == 1 );
        CATCH_REQUIRE( c->getRarity() == RARITY_COMMON );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 1 );
        CATCH_REQUIRE( colors.count( COLOR_RED ) );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Instant" ) );
        delete c;
    }

    CATCH_SECTION( "Empty set (find version by priority)" )
    {
        c = allSets.createCardData( "", "Black Lotus" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "2ED" );
        CATCH_REQUIRE( c->getName() == "Black Lotus" );
        delete c;

        CATCH_REQUIRE( allSets.createCardData( "", "No Such Card" ) == nullptr );
    }

    CATCH_SECTION( "Color/Rarity/Type coverage" )
    {
        c = allSets.createCardData( "INV", "Fact or Fiction" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "INV" );
        CATCH_REQUIRE( c->getName() == "Fact or Fiction" );
        CATCH_REQUIRE( c->getMultiverseId() == 22998 );
        CATCH_REQUIRE( c->getCMC() == 4 );
        CATCH_REQUIRE( c->getRarity() == RARITY_UNCOMMON );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 1 );
        CATCH_REQUIRE( colors.count( COLOR_BLUE ) );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Instant" ) );
        delete c;

        c = allSets.createCardData( "RAV", "Putrefy" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "RAV" );
        CATCH_REQUIRE( c->getName() == "Putrefy" );
        CATCH_REQUIRE( c->getMultiverseId() == 89063 );
        CATCH_REQUIRE( c->getCMC() == 3 );
        CATCH_REQUIRE( c->getRarity() == RARITY_UNCOMMON );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == true );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 2 );
        CATCH_REQUIRE( colors.count( COLOR_BLACK ) );
        CATCH_REQUIRE( colors.count( COLOR_GREEN ) );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Instant" ) );
        delete c;

        c = allSets.createCardData( "LEA", "Black Lotus" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "LEA" );
        CATCH_REQUIRE( c->getName() == "Black Lotus" );
        CATCH_REQUIRE( c->getMultiverseId() == 3 );
        CATCH_REQUIRE( c->getCMC() == 0 );
        CATCH_REQUIRE( c->getRarity() == RARITY_RARE );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 0 );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Artifact" ) );
        delete c;

        c = allSets.createCardData( "APC", "Lightning Angel" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "APC" );
        CATCH_REQUIRE( c->getName() == "Lightning Angel" );
        CATCH_REQUIRE( c->getMultiverseId() == 27650 );
        CATCH_REQUIRE( c->getCMC() == 4 );
        CATCH_REQUIRE( c->getRarity() == RARITY_RARE );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == true );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 3 );
        CATCH_REQUIRE( colors.count( COLOR_RED ) );
        CATCH_REQUIRE( colors.count( COLOR_WHITE ) );
        CATCH_REQUIRE( colors.count( COLOR_BLUE ) );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Creature" ) );
        delete c;

        c = allSets.createCardData( "DST", "Darksteel Colossus" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "DST" );
        CATCH_REQUIRE( c->getName() == "Darksteel Colossus" );
        CATCH_REQUIRE( c->getMultiverseId() == 48158 );
        CATCH_REQUIRE( c->getCMC() == 11 );
        CATCH_REQUIRE( c->getRarity() == RARITY_RARE );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 0 );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 2 );
        CATCH_REQUIRE( types.count( "Artifact" ) );
        CATCH_REQUIRE( types.count( "Creature" ) );
        delete c;

        c = allSets.createCardData( "THS", "Unknown Shores" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "THS" );
        CATCH_REQUIRE( c->getName() == "Unknown Shores" );
        CATCH_REQUIRE( c->getMultiverseId() == 373743 );
        CATCH_REQUIRE( c->getCMC() == 0 );
        CATCH_REQUIRE( c->getRarity() == RARITY_COMMON );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 0 );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Land" ) );
        delete c;
    }

    CATCH_SECTION( "Split cards" )
    {
        c = allSets.createCardData( "APC", "Fire" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "APC" );
        CATCH_REQUIRE( c->getName() == "Fire // Ice" );
        CATCH_REQUIRE( c->getMultiverseId() == 27166 );
        CATCH_REQUIRE( c->getCMC() == 2 );
        CATCH_REQUIRE( c->getRarity() == RARITY_UNCOMMON );
        CATCH_REQUIRE( c->isSplit() == true );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 1 );
        CATCH_REQUIRE( colors.count( COLOR_RED ) );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Instant" ) );

        // Create variant names and ensure they produce equivalent objects.

        auto checkVariantEqualFunc = [&allSets,c]( const std::string& set,
                                                   const std::string& name )
        {
            CardData* variant = allSets.createCardData( set, name );
            CATCH_REQUIRE( variant != nullptr );
            CATCH_REQUIRE( *variant == *c );
            delete variant;
        };

        checkVariantEqualFunc( "APC", "Fire // Ice" );
        checkVariantEqualFunc( "APC", "Fire/Ice" );
        checkVariantEqualFunc( "APC", "Ice" );

        delete c;
    }

    CATCH_SECTION( "Flip cards" )
    {
        c = allSets.createCardData( "CHK", "Bushi Tenderfoot" );
        CATCH_REQUIRE( c != 0 );
        CATCH_REQUIRE( c->getSetCode() == "CHK" );
        CATCH_REQUIRE( c->getName() == "Bushi Tenderfoot" );
        CATCH_REQUIRE( c->getMultiverseId() == 78600 );
        CATCH_REQUIRE( c->getCMC() == 1 );
        CATCH_REQUIRE( c->getRarity() == RARITY_UNCOMMON );
        CATCH_REQUIRE( c->isSplit() == false );
        CATCH_REQUIRE( c->isMulticolor() == false );
        colors = c->getColors();
        CATCH_REQUIRE( colors.size() == 1 );
        CATCH_REQUIRE( colors.count( COLOR_WHITE ) );
        types = c->getTypes();
        CATCH_REQUIRE( types.size() == 1 );
        CATCH_REQUIRE( types.count( "Creature" ) );
        delete c;
    }
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check caching", "[mtgjson][cache]" )
{
    MtgJsonAllSetsData allSetsSmallCache( 2 );
    initAllSetsData( allSetsSmallCache );

    // 
    // CARD LOOKUPS
    //

    CardData* c;

    // Miss on two new entries.
    c = allSetsSmallCache.createCardData( "LEA", "Lightning Bolt" );
    delete c;
    c = allSetsSmallCache.createCardData( "3ED", "Lightning Bolt" );
    delete c;

    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheHits() == 0 );
    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheMisses() == 2 );

    // Hit on two existing entries.
    c = allSetsSmallCache.createCardData( "LEA", "Lightning Bolt" );
    delete c;
    c = allSetsSmallCache.createCardData( "3ED", "Lightning Bolt" );
    delete c;

    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheHits() == 2 );
    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheMisses() == 2 );

    // Miss on new entry, then miss because LEA entry should be evicted.
    c = allSetsSmallCache.createCardData( "LEB", "Lightning Bolt" );
    delete c;
    c = allSetsSmallCache.createCardData( "LEA", "Lightning Bolt" );
    delete c;

    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheHits() == 2 );
    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheMisses() == 4 );

    // Miss on setless new entry.
    c = allSetsSmallCache.createCardData( "", "Lightning Bolt" );
    delete c;

    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheHits() == 2 );
    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheMisses() == 5 );

    // Hit on same setless entry.
    c = allSetsSmallCache.createCardData( "", "Lightning Bolt" );
    delete c;

    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheHits() == 3 );
    CATCH_REQUIRE( allSetsSmallCache.getSetNameLookupCacheMisses() == 5 );

    // 
    // SET CODE LOOKUPS
    //

    // Miss on two new entries.
    CATCH_REQUIRE( allSetsSmallCache.findSetCode( "Black Lotus" ) == "2ED" );
    CATCH_REQUIRE( allSetsSmallCache.findSetCode( "No Such Card" ) == "" );
    CATCH_REQUIRE( allSetsSmallCache.getSetCodeLookupCacheHits() == 0 );
    CATCH_REQUIRE( allSetsSmallCache.getSetCodeLookupCacheMisses() == 2 );

    // Hit on two existing entries.
    CATCH_REQUIRE( allSetsSmallCache.findSetCode( "Black Lotus" ) == "2ED" );
    CATCH_REQUIRE( allSetsSmallCache.findSetCode( "No Such Card" ) == "" );
    CATCH_REQUIRE( allSetsSmallCache.getSetCodeLookupCacheHits() == 2 );
    CATCH_REQUIRE( allSetsSmallCache.getSetCodeLookupCacheMisses() == 2 );

    // Miss on a new entry.
    CATCH_REQUIRE( allSetsSmallCache.findSetCode( "Savannah" ) == "3ED" );
    CATCH_REQUIRE( allSetsSmallCache.getSetCodeLookupCacheHits() == 2 );
    CATCH_REQUIRE( allSetsSmallCache.getSetCodeLookupCacheMisses() == 3 );
}

// ------------------------------------------------------------------------

CATCH_TEST_CASE( "Spot-check CardCata created from multiverse id", "[mtgjson]" )
{
    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();

    CATCH_REQUIRE( allSets.createCardData( -1 ) == nullptr );
    CATCH_REQUIRE( allSets.createCardData( 4000000000 ) == nullptr );

    CardData* c;
    std::set<ColorType> colors;
    std::set<std::string> types;

    c = allSets.createCardData( 209 );
    CATCH_REQUIRE( c != 0 );
    CATCH_REQUIRE( c->getSetCode() == "LEA" );
    CATCH_REQUIRE( c->getName() == "Lightning Bolt" );
    CATCH_REQUIRE( c->getMultiverseId() == 209 );
    CATCH_REQUIRE( c->getCMC() == 1 );
    CATCH_REQUIRE( c->getRarity() == RARITY_COMMON );
    CATCH_REQUIRE( c->isSplit() == false );
    CATCH_REQUIRE( c->isMulticolor() == false );
    colors = c->getColors();
    CATCH_REQUIRE( colors.size() == 1 );
    CATCH_REQUIRE( colors.count( COLOR_RED ) );
    types = c->getTypes();
    CATCH_REQUIRE( types.size() == 1 );
    CATCH_REQUIRE( types.count( "Instant" ) );
    delete c;

    c = allSets.createCardData( 1303 );
    CATCH_REQUIRE( c != 0 );
    CATCH_REQUIRE( c->getSetCode() == "3ED" );
    CATCH_REQUIRE( c->getName() == "Lightning Bolt" );
    CATCH_REQUIRE( c->getMultiverseId() == 1303 );
    CATCH_REQUIRE( c->getCMC() == 1 );
    CATCH_REQUIRE( c->getRarity() == RARITY_COMMON );
    CATCH_REQUIRE( c->isSplit() == false );
    CATCH_REQUIRE( c->isMulticolor() == false );
    colors = c->getColors();
    CATCH_REQUIRE( colors.size() == 1 );
    CATCH_REQUIRE( colors.count( COLOR_RED ) );
    types = c->getTypes();
    CATCH_REQUIRE( types.size() == 1 );
    CATCH_REQUIRE( types.count( "Instant" ) );
    delete c;
}

