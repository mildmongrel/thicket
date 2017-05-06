#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

using proto::DraftConfig;

static Logging::Config getLoggingConfig()
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "testgriddraft" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::trace );
    return loggingConfig;
}
 
class GridTestDraftObserver : public TestDraftObserver
{
public:
    GridTestDraftObserver() : mPublicStateUpdates( 0 ), mPublicSelectionErrors( 0 ), mNewRounds( 0 ) {}

    virtual void notifyPublicState( Draft<>& draft, uint32_t packId, const std::vector<PublicCardState>& cardStates, int activeChairIndex ) override
    {
        mPackId = packId;
        mCardStates.clear();
        std::copy( cardStates.begin(), cardStates.end(), std::back_inserter( mCardStates ) );
        mActiveChairIndex = activeChairIndex;
        mPublicStateUpdates++;
    }
    virtual void notifyIndexedCardSelectionResult( Draft<>& draft, int chairIndex, uint32_t packId, bool result, const std::vector<int>& selectionIndices, const std::vector<std::string>& cards ) override
    {
        if( !result ) mPublicSelectionErrors++;
    }
    virtual void notifyNewRound( Draft<>& draft, int roundIndex ) override
    {
        mNewRounds++;
    }

    uint32_t                     mPackId;
    std::vector<PublicCardState> mCardStates;
    int                          mActiveChairIndex;

    int mPublicStateUpdates;
    int mPublicSelectionErrors;
    int mNewRounds;
};
 

CATCH_TEST_CASE( "Grid draft", "[draft][grid]" )
{
    DraftConfig dc = TestDefaults::getSimpleGridDraftConfig();
    auto dispensers = TestDefaults::getDispensers( 1 );
    Draft<> d( dc, dispensers, getLoggingConfig() );

    GridTestDraftObserver obs;
    d.addObserver( &obs );

    d.start();

    CATCH_SECTION( "Initial assumptions" )
    {
        CATCH_REQUIRE( obs.mPublicStateUpdates > 0 );
        CATCH_REQUIRE( obs.mActiveChairIndex == 0 );
        CATCH_REQUIRE( obs.mCardStates.size() == 9 );
        for( auto state : obs.mCardStates )
        {
            CATCH_REQUIRE( state.getSelectedChairIndex() == -1 );
            CATCH_REQUIRE( state.getSelectedOrder() == -1 );
        }
        CATCH_REQUIRE( obs.mPublicSelectionErrors == 0 );
        CATCH_REQUIRE( obs.mNewRounds == 1 );
    }

    CATCH_SECTION( "Invalid selecting chair" )
    {
        std::vector<int> picks;

        CATCH_SECTION( "1st pick invalid" )
        {
            // rnd0pick0: wrong chair 1
            picks = { 0, 1, 2 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );
            CATCH_REQUIRE( obs.mPublicSelectionErrors == 1 );

            // rnd0pick0: correct chair 0
            picks = { 0, 1, 2 };
            d.makeIndexedCardSelection( 0, obs.mPackId, picks );
            CATCH_REQUIRE( obs.mPublicSelectionErrors == 1 );

            // rnd0pick1: correct chair 1
            picks = { 3, 4, 5 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );
            CATCH_REQUIRE( obs.mPublicSelectionErrors == 1 );
        }

        CATCH_SECTION( "2nd pick invalid" )
        {
            // rnd1pick0: correct chair 0
            picks = { 0, 1, 2 };
            d.makeIndexedCardSelection( 0, obs.mPackId, picks );
            CATCH_REQUIRE( obs.mPublicSelectionErrors == 0 );

            // rnd1pick1: wrong chair 0
            picks = { 3, 4, 5 };
            d.makeIndexedCardSelection( 0, obs.mPackId, picks );
            CATCH_REQUIRE( obs.mPublicSelectionErrors == 1 );

            // rnd1pick1: correct chair 1
            picks = { 3, 4, 5 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );
            CATCH_REQUIRE( obs.mPublicSelectionErrors == 1 );
        }
    }

    CATCH_SECTION( "valid 3-card selections" )
    {
        std::vector<int> picks;

        picks = { 0, 1, 2 }; // player0rnd0: row0
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );
        picks = { 3, 4, 5 }; // player1rnd0: row1
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );

        picks = { 3, 4, 5 }; // player1rnd1: row1
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );
        picks = { 6, 7, 8 }; // player0rnd1: row2
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        picks = { 6, 7, 8 }; // player0rnd2: row2
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );
        picks = { 0, 1, 2 }; // player1rnd2: row0
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );

        picks = { 0, 3, 6 }; // player1rnd3: col0
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );
        picks = { 1, 4, 7 }; // player0rnd3: col1
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        picks = { 1, 4, 7 }; // player0rnd4: col1
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );
        picks = { 2, 5, 8 }; // player1rnd4: col2
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );

        picks = { 2, 5, 8 }; // player1rnd5: col2
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );
        picks = { 0, 3, 6 }; // player0rnd5: col0
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        CATCH_REQUIRE( obs.mPublicSelectionErrors == 0 );
        CATCH_REQUIRE( obs.mNewRounds == 7 );
    }

    CATCH_SECTION( "valid 3-card selection then 2-card selection" )
    {
        std::vector<int> picks;

        picks = { 0, 1, 2 }; // player0rnd0: row0
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );
        picks = { 3, 6 };    // player1rnd0: col0
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );

        picks = { 3, 4, 5 }; // player0rnd1: row1
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );
        picks = { 1, 7 };    // player1rnd1: col1 
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        picks = { 6, 7, 8 }; // player0rnd2: row2
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );
        picks = { 2, 5 };    // player1rnd2: col2
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );

        picks = { 0, 3, 6 }; // player0rnd3: col0
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );
        picks = { 1, 2 };    // player1rnd3: row0
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        picks = { 1, 4, 7 }; // player0rnd4: col1
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );
        picks = { 3, 5    }; // player1rnd4: row1
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );

        picks = { 2, 5, 8 }; // player0rnd5: col2
        d.makeIndexedCardSelection( 1, obs.mPackId, picks );
        picks = { 6, 7 };    // player1rnd5: row2
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        CATCH_REQUIRE( obs.mPublicSelectionErrors == 0 );
        CATCH_REQUIRE( obs.mNewRounds == 7 );
    }

    CATCH_SECTION( "Invalid first selections" )
    {
        std::vector<int> picks;

        // invalid card
        picks = { 9 };
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        // valid and invalid cards
        picks = { 0, 9 };
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        // not a row/col
        picks = { 0, 1, 3 };
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        // diagonal
        picks = { 0, 4, 8 };
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        // more than a row
        picks = { 0, 1, 2, 3 };
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        // partial
        picks = { 5 };
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        // partial row
        picks = { 5, 6 };
        d.makeIndexedCardSelection( 0, obs.mPackId, picks );

        CATCH_REQUIRE( obs.mPublicSelectionErrors == 7 );
        CATCH_REQUIRE( obs.mNewRounds == 1 );
    }

    CATCH_SECTION( "Invalid second selections" )
    {
        std::vector<int> picks;

        CATCH_SECTION( "First pick row" )
        {
            picks = { 3, 4, 5 };
            d.makeIndexedCardSelection( 0, obs.mPackId, picks );

            // insufficient
            picks = { 8 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            // partial row
            picks = { 1, 2 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            // total overlap
            picks = { 3, 4, 5 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            // column with overlap
            picks = { 2, 5, 8 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            CATCH_REQUIRE( obs.mPublicSelectionErrors == 4 );
            CATCH_REQUIRE( obs.mNewRounds == 1 );
        }

        CATCH_SECTION( "First pick col" )
        {
            picks = { 1, 4, 7 };
            d.makeIndexedCardSelection( 0, obs.mPackId, picks );

            // insufficient
            picks = { 8 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            // partial col
            picks = { 0, 3 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            // total overlap
            picks = { 1, 4, 7 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            // row with overlap
            picks = { 6, 7, 8 };
            d.makeIndexedCardSelection( 1, obs.mPackId, picks );

            CATCH_REQUIRE( obs.mPublicSelectionErrors == 4 );
            CATCH_REQUIRE( obs.mNewRounds == 1 );
        }
    }
}
