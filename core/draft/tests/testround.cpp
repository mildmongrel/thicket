#include "catch.hpp"
#include "Draft.h"
#include "TestDraftObserver.h"
#include "testdefaults.h"

#include <functional>

using proto::DraftConfig;

static Logging::Config getLoggingConfig()
{
    Logging::Config loggingConfig;
    loggingConfig.setName( "testround" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::trace );
    return loggingConfig;
}
 
class RoundTestDraftObserver : public TestDraftObserver
{
public:
    RoundTestDraftObserver() : mNamedSelectionErrors( 0 ), mIndexedSelectionErrors( 0 ), mNewRoundNotifications( 0 ) {}

    void setRoundTypeValidator( std::function<bool(const Draft<>&)> validator ) { mValidator = validator; }

    virtual void notifyNewPack( Draft<>& draft, int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override
    {
        if( mValidator ) CATCH_REQUIRE( mValidator( draft ) );
        draft.makeNamedCardSelection( chairIndex, packId, unselectedCards[0] );
    }
    virtual void notifyPublicState( Draft<>& draft, uint32_t packId, const std::vector<PublicCardState>& cardStates, int activeChairIndex ) override
    {
        if( mValidator ) CATCH_REQUIRE( mValidator( draft ) );
        std::vector<int> picks = { 0, 1, 2 }; // chair 0 -> first row
        if (activeChairIndex % 2) picks = { 3, 4, 5 }; // chair 1 -> second row
        draft.makeIndexedCardSelection( activeChairIndex, packId, picks );
    }
    virtual void notifyNamedCardSelectionResult( Draft<>&           draft,
                                                 int                chairIndex,
                                                 uint32_t           packId,
                                                 bool               result,
                                                 const std::string& card )

    {
        if( mValidator ) CATCH_REQUIRE( mValidator( draft ) );
        if( !result ) mNamedSelectionErrors++;
    }
    virtual void notifyIndexedCardSelectionResult( Draft<>&                        draft,
                                                   int                             chairIndex,
                                                   uint32_t                        packId,
                                                   bool                            result,
                                                   const std::vector<int>&         selectionIndices,
                                                   const std::vector<std::string>& cards )
    {
        if( mValidator ) CATCH_REQUIRE( mValidator( draft ) );
        if( !result ) mIndexedSelectionErrors++;
    }

    virtual void notifyNewRound( Draft<>& draft, int roundIndex ) override
    {
        if( mValidator ) CATCH_REQUIRE( mValidator( draft ) );
        mNewRoundNotifications++;
    }

    int mNamedSelectionErrors;
    int mIndexedSelectionErrors;
    int mNewRoundNotifications;
    std::function<bool(const Draft<>&)> mValidator;
};
 

CATCH_TEST_CASE( "Rounds: simple booster", "[draft][round]" )
{
    const int NUM_ROUNDS  = 3;
    const int NUM_PLAYERS = 8;

    DraftConfig dc = TestDefaults::getSimpleBoosterDraftConfig( NUM_ROUNDS, NUM_PLAYERS, 30 );
    auto dispensers = TestDefaults::getDispensers();
    Draft<> d( dc, dispensers, getLoggingConfig() );

    RoundTestDraftObserver obs;
    obs.setRoundTypeValidator( [](const Draft<>& draft) { return draft.isBoosterRound(); } );
    d.addObserver( &obs );

    d.start();

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );
    CATCH_REQUIRE( obs.mNewRoundNotifications == dc.rounds_size() );
    CATCH_REQUIRE( obs.mNamedSelectionErrors == 0 );
    CATCH_REQUIRE( obs.mIndexedSelectionErrors == 0 );
}

CATCH_TEST_CASE( "Rounds: simple sealed", "[draft][round]" )
{
    const int NUM_PLAYERS = 8;

    DraftConfig dc = TestDefaults::getSimpleSealedDraftConfig( NUM_PLAYERS );
    auto dispensers = TestDefaults::getDispensers( 6 );
    Draft<> d( dc, dispensers, getLoggingConfig() );

    RoundTestDraftObserver obs;
    obs.setRoundTypeValidator( [](const Draft<>& draft) { return draft.isSealedRound(); } );
    d.addObserver( &obs );

    d.start();

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );
    CATCH_REQUIRE( obs.mNewRoundNotifications == dc.rounds_size() );
    CATCH_REQUIRE( obs.mNamedSelectionErrors == 0 );
    CATCH_REQUIRE( obs.mIndexedSelectionErrors == 0 );
}

CATCH_TEST_CASE( "Rounds: simple grid", "[draft][round]" )
{
    DraftConfig dc = TestDefaults::getSimpleGridDraftConfig();
    auto dispensers = TestDefaults::getDispensers( 1 );
    Draft<> d( dc, dispensers, getLoggingConfig() );

    RoundTestDraftObserver obs;
    obs.setRoundTypeValidator( [](const Draft<>& draft) { return draft.isGridRound(); } );
    d.addObserver( &obs );

    d.start();

    CATCH_REQUIRE( d.getState() == Draft<>::STATE_COMPLETE );
    CATCH_REQUIRE( obs.mNewRoundNotifications == dc.rounds_size() );
    CATCH_REQUIRE( obs.mNamedSelectionErrors == 0 );
    CATCH_REQUIRE( obs.mIndexedSelectionErrors == 0 );
}
