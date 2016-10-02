#include "catch.hpp"
#include "spdlog/spdlog.h"
#include "MtgJsonAllSetsData.h"
#include "CardPoolSelector.h"
#include "SimpleRandGen.h"
#include <vector>

extern const MtgJsonAllSetsData& getAllSetsDataInstance();

CATCH_TEST_CASE( "Cardpool tests", "[cardpool]" )
{
    //
    // Initialize a static MtgAllSetsData instance for use in all sections.
    //

    Logging::Config loggingConfig;
    loggingConfig.setName( "testcardpool" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::debug );

    const MtgJsonAllSetsData& allSets = getAllSetsDataInstance();

    // --------------------------------------------------------------------

    CATCH_SECTION( "Select a bunch of cards" )
    {

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
            int cardPoolSize = cps.getPoolSize();

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

            CATCH_REQUIRE( cps.getPoolSize() == cardPoolSize - 35 );

            cps.resetCardPool();

            CATCH_REQUIRE( cps.getPoolSize() == cardPoolSize );
        }

        CATCH_REQUIRE( totalRares > 0 );
        CATCH_REQUIRE( totalMythicRares > 0 );
        CATCH_REQUIRE( totalRares > totalMythicRares );
    }

    // --------------------------------------------------------------------

    CATCH_SECTION( "Names in cardpool equal to carddata" )
    {
        auto isPoolAndDataNameEqualFunc = []( const MtgJsonAllSetsData& allSets,
                                              const std::string&        set,
                                              const std::string&        name )
        {
            std::multimap<RarityType,std::string> rarityMap = allSets.getCardPool( set );

            CardData* c = allSets.createCardData( set, name );
            if( c == nullptr ) return false;

            bool found = false;
            for( auto kv : rarityMap )
            {
                if( (kv.second == name) && (kv.second == c->getName()) )
                {
                    found = true;
                    break;
                }
            }
            delete c;
            return found;
        };

        CATCH_SECTION( "Typical" )
        {
            CATCH_REQUIRE( isPoolAndDataNameEqualFunc( allSets, "APC", "Spiritmonger" ) );
        }

        CATCH_SECTION( "Split" )
        {
            CATCH_REQUIRE( isPoolAndDataNameEqualFunc( allSets, "APC", "Fire // Ice" ) );
        }

        CATCH_SECTION( "Bad name" )
        {
            CATCH_REQUIRE_FALSE( isPoolAndDataNameEqualFunc( allSets, "APC", "Fire//Ice" ) );
            CATCH_REQUIRE_FALSE( isPoolAndDataNameEqualFunc( allSets, "APC", "Fire" ) );
        }

        CATCH_SECTION( "Unknown name" )
        {
            CATCH_REQUIRE_FALSE( isPoolAndDataNameEqualFunc( allSets, "APC", "NoSuchCard" ) );
        }
    }

    // --------------------------------------------------------------------

    CATCH_SECTION( "Cardpool cards creatable as individual carddata" )
    {
        auto createAllCardsFromCardPoolFunc = [&allSets]( const std::string& set )
        {
            std::multimap<RarityType,std::string> rarityMap = allSets.getCardPool( set );
            for( auto kv : rarityMap )
            {
                CardData* c = allSets.createCardData( set, kv.second );
                CATCH_REQUIRE( c != nullptr );
                delete c;
            }
        };

        CATCH_SECTION( "Typical sets" )
        {
            createAllCardsFromCardPoolFunc( "LEA" );
            createAllCardsFromCardPoolFunc( "MIR" );
        }

        CATCH_SECTION( "Sets with split cards" )
        {
            createAllCardsFromCardPoolFunc( "INV" );
            createAllCardsFromCardPoolFunc( "APC" );
        }
    }

}

