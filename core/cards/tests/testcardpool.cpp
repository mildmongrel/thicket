#include "catch.hpp"
#include "spdlog/spdlog.h"
#include "MtgJsonAllSetsData.h"
#include "CardPoolSelector.h"
#include "SimpleRandGen.h"
#include <vector>

CATCH_TEST_CASE( "Select a bunch of cards", "[cardpool]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "testcardpool" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    const std::string allSetsDataFilename = "AllSets.json";
    FILE* allSetsDataFile = fopen( allSetsDataFilename.c_str(), "r" );
    if( allSetsDataFile == NULL )
        CATCH_FAIL( "Failed to open AllSets.json: it must be co-located with the test executable." );

    MtgJsonAllSetsData allSets;
    bool parseResult = allSets.parse( allSetsDataFile );
    fclose( allSetsDataFile );
    CATCH_REQUIRE( parseResult );

    std::string setCode = "THS";

    std::vector<SlotType> boosterSlots;
    boosterSlots = allSets.getBoosterSlots( setCode );

    std::multimap<RarityType,std::string> rarityMap;
    rarityMap = allSets.getCardPool( setCode );

    std::map<std::string,RarityType> rarityLookup;
    for( auto p : rarityMap )
    {
        rarityLookup.insert( std::make_pair( p.second, p.first ) );
    }

    auto pRng = std::shared_ptr<RandGen>( new SimpleRandGen() );
    CardPoolSelector cps( rarityMap, pRng, 0.125, loggingConfig.createChildConfig( "cps" ) );

    // Select a quantity of cards, making sure there are no duplicates.
    int totalRares = 0;
    int totalMythicRares = 0;
    for( int trial = 0; trial < 100; ++trial )
    {
        std::set<std::string> selectedCards;
        std::string selectedCard;
        for( int i = 0; i < 20; ++i )
        {
            bool result = cps.selectCard( SLOT_COMMON, selectedCard );
            CATCH_REQUIRE( result );

            // The card must exist in the set and be the right rarity.
            CATCH_REQUIRE( rarityLookup.count( selectedCard ) == 1 );
            CATCH_REQUIRE( rarityLookup[selectedCard] == RARITY_COMMON );

            // The card selection must be unique.
            CATCH_REQUIRE( selectedCards.count( selectedCard ) == 0 );
            selectedCards.insert( selectedCard );
        }
        for( int i = 0; i < 10; ++i )
        {
            bool result = cps.selectCard( SLOT_UNCOMMON, selectedCard );
            CATCH_REQUIRE( result );

            // The card must exist in the set and be the right rarity.
            CATCH_REQUIRE( rarityLookup.count( selectedCard ) == 1 );
            CATCH_REQUIRE( rarityLookup[selectedCard] == RARITY_UNCOMMON );

            // The card selection must be unique.
            CATCH_REQUIRE( selectedCards.count( selectedCard ) == 0 );
            selectedCards.insert( selectedCard );
        }

        // TODO choose rare only and make sure only rares come back

        for( int i = 0; i < 5; ++i )
        {
            bool result = cps.selectCard( SLOT_RARE_OR_MYTHIC_RARE, selectedCard );
            CATCH_REQUIRE( result );

            // The card must exist.
            CATCH_REQUIRE( rarityLookup.count( selectedCard ) == 1 );

            // Tally rarities for overall check at end.
            if( rarityLookup[selectedCard] == RARITY_RARE )
                totalRares++;
            else if( rarityLookup[selectedCard] == RARITY_MYTHIC_RARE )
                totalMythicRares++;
            else
                CATCH_FAIL( "expected rare or mythic rare only" );

            // The card selection must be unique.
            CATCH_REQUIRE( selectedCards.count( selectedCard ) == 0 );
            selectedCards.insert( selectedCard );
        }

        cps.resetCardPool();
    }

    CATCH_REQUIRE( totalRares > 0 );
    CATCH_REQUIRE( totalMythicRares > 0 );
    CATCH_REQUIRE( totalRares > totalMythicRares );
}


CATCH_TEST_CASE( "Test names in cardpool vs carddata", "[cardpool]" )
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "testcardpool" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    const std::string allSetsDataFilename = "AllSets.json";
    FILE* allSetsDataFile = fopen( allSetsDataFilename.c_str(), "r" );
    if( allSetsDataFile == NULL )
        CATCH_FAIL( "Failed to open AllSets.json: it must be co-located with the test executable." );

    MtgJsonAllSetsData allSets;
    bool parseResult = allSets.parse( allSetsDataFile );
    fclose( allSetsDataFile );
    CATCH_REQUIRE( parseResult );

    // Test that cards from pool are equivalent to created card.
    {
        std::multimap<RarityType,std::string> rarityMap = allSets.getCardPool( "APC" );
        CardData* c = allSets.createCardData( "APC", "Spiritmonger" );
        bool found = false;
        for( auto kv : rarityMap )
        {
            if( kv.second == c->getName() )
            {
                found = true;
                break;
            }
        }
        CATCH_REQUIRE( found );
        delete c;
    }
    {
        std::multimap<RarityType,std::string> rarityMap = allSets.getCardPool( "APC" );
        CardData* c = allSets.createCardData( "APC", "Fire // Ice" );
        bool found = false;
        for( auto kv : rarityMap )
        {
            if( kv.second == c->getName() )
            {
                found = true;
                break;
            }
        }
        CATCH_REQUIRE( found );
        delete c;
    }
    
}
