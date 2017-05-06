#ifndef STUPIDBOTPLAYER_H
#define STUPDIBOTPLAYER_H

#include "BotPlayer.h"
#include "Logging.h"

#include "SimpleRandGen.h"
#include "GridHelper.h"

class StupidBotPlayer : public BotPlayer
{
public:
    StupidBotPlayer( int chairIndex, const Logging::Config& loggingConfig = Logging::Config() )
      : BotPlayer( chairIndex ),
        mLogger( loggingConfig.createLogger() )
        {}

    // Always pick a random card.
    virtual void notifyNewPack( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards ) override
    {
        SimpleRandGen rng;
        const int index = rng.generateInRange( 0, unselectedCards.size() - 1 );
        DraftCard stupidCardToSelect = unselectedCards[index];
        mLogger->info( "StupidBot<{}> selecting card {} ({})", getChairIndex(), stupidCardToSelect.name, index );
        bool result = draft.makeNamedCardSelection( getChairIndex(), packId, stupidCardToSelect );
        if( !result )
        {
            mLogger->warn( "error selecting card {}", stupidCardToSelect.name );
        }
    }

    // Currently this is only received in grid draft.  If the active player, pick
    // row zero if available, otherwise pick row 1.
    virtual void notifyPublicState( DraftType& draft, uint32_t packId, const std::vector<PublicCardState>& cardStates, int activeChairIndex) override
    {
        if( !draft.isGridRound() )
        {
            mLogger->warn( "not a grid round!" );
            return;
        }

        GridHelper gh;
        if( cardStates.size() < gh.getIndexCount() )
        {
            mLogger->warn( "less than {} cards ({}) in grid pack!", gh.getIndexCount(), cardStates.size() );
            return;
        }

        if( getChairIndex() == activeChairIndex )
        {
            // Build set of unavailable indices.
            GridHelper::IndexSet unavailableIndices;
            for( std::size_t i = 0; i < cardStates.size(); ++i )
            {
                if( cardStates[i].getSelectedChairIndex() != -1 ) unavailableIndices.insert( i );
            }

            // Get available selections.
            GridHelper gridHelper;
            auto availableSelectionsMap = gridHelper.getAvailableSelectionsMap( unavailableIndices );
            if( availableSelectionsMap.empty() )
            {
                mLogger->error( "no available selections in grid pack!" );
                return;
            }

            // Randomly pick a selection.
            SimpleRandGen rng;
            const int adv = rng.generateInRange( 0, availableSelectionsMap.size() - 1 );
            auto iter = availableSelectionsMap.begin();
            std::advance( iter, adv );

            mLogger->debug( "selecting slice {}", iter->second );
            std::vector<int> indices( iter->first.begin(), iter->first.end() );
            bool result = draft.makeIndexedCardSelection( getChairIndex(), packId, indices );

            if( !result )
            {
                mLogger->warn( "error selecting public cards" );
            }
        }
    }

    // No-ops everywhere else.
    virtual void notifyPackQueueSizeChanged( DraftType& draft, int packQueueSize ) override {}
    virtual void notifyNamedCardSelectionResult( DraftType& draft, uint32_t packId, bool result, const DraftCard& card ) override {}
    virtual void notifyIndexedCardSelectionResult( DraftType& draft, uint32_t packId, bool result, const std::vector<int>& selectionIndices, const std::vector<DraftCard>& cards ) override {}
    virtual void notifyCardAutoselection( DraftType& draft, uint32_t packId, const DraftCard& card ) override {}
    virtual void notifyTimeExpired( DraftType& draft, uint32_t packId ) override {}
    virtual void notifyPostRoundTimerStarted( DraftType& draft, int roundIndex, int ticksRemaining ) override {}
    virtual void notifyNewRound( DraftType& draft, int roundIndex ) override {}
    virtual void notifyDraftComplete( DraftType& draft ) override {}
    virtual void notifyDraftError( DraftType& draft ) override {}

private:
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // StupidBotPlayer.h
