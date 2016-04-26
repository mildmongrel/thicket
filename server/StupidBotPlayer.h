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
    virtual void notifyNewPack( DraftType& draft, const DraftPackId& packId, const std::vector<DraftCard>& unselectedCards )
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
    virtual void notifyPackQueueSizeChanged( DraftType& draft, int packQueueSize ) {}
    virtual void notifyCardSelected( DraftType& draft, const DraftPackId& packId, const DraftCard& card, bool autoSelected ) {}
    virtual void notifyCardSelectionError( DraftType& draft, const DraftCard& card ) {}
    virtual void notifyTimeExpired( DraftType& draft, const DraftPackId& packId, const std::vector<DraftCard>& unselectedCards ) {}
    virtual void notifyNewRound( DraftType& draft, int roundIndex, const DraftRoundInfo& round ) {}
    virtual void notifyDraftComplete( DraftType& draft ) {}

private:
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // StupidBotPlayer.h