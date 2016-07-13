#ifndef STUPIDBOTPLAYER_H
#define STUPDIBOTPLAYER_H

#include "BotPlayer.h"
#include "Logging.h"

#include "SimpleRandGen.h"

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
        bool result = draft.makeCardSelection( getChairIndex(), stupidCardToSelect );
        if( !result )
        {
            mLogger->warn( "error selecting card {}", stupidCardToSelect.name );
        }
    }
    // No-ops everywhere else.
    virtual void notifyPackQueueSizeChanged( DraftType& draft, int packQueueSize ) override {}
    virtual void notifyCardSelected( DraftType& draft, uint32_t packId, const DraftCard& card, bool autoSelected ) override {}
    virtual void notifyCardSelectionError( DraftType& draft, const DraftCard& card ) override {}
    virtual void notifyTimeExpired( DraftType& draft, uint32_t packId, const std::vector<DraftCard>& unselectedCards ) override {}
    virtual void notifyNewRound( DraftType& draft, int roundIndex ) override {}
    virtual void notifyDraftComplete( DraftType& draft ) override {}
    virtual void notifyDraftError( DraftType& draft ) override {}

private:
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // StupidBotPlayer.h
