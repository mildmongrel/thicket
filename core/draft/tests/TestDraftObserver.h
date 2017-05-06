#include "Draft.h"

// A do-nothing version of Draft<>::Observer

class TestDraftObserver : public Draft<>::Observer
{
public:
    virtual void notifyPackQueueSizeChanged( Draft<>& draft, int chairIndex, int packQueueSize ) override {}
    virtual void notifyNewPack( Draft<>& draft,int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override {}
    virtual void notifyPublicState( Draft<>& draft, uint32_t packId, const std::vector<PublicCardState>& cardStates, int activeChairIndex) override {}
    virtual void notifyNamedCardSelectionResult( Draft<>&           draft,
                                                 int                chairIndex,
                                                 uint32_t           packId,
                                                 bool               result,
                                                 const std::string& card ) override {}
    virtual void notifyIndexedCardSelectionResult( Draft<>&                        draft,
                                                   int                             chairIndex,
                                                   uint32_t                        packId,
                                                   bool                            result,
                                                   const std::vector<int>&         selectionIndices,
                                                   const std::vector<std::string>& cards ) override {}
    virtual void notifyCardAutoselection( Draft<>&           draft,
                                          int                chairIndex,
                                          uint32_t           packId,
                                          const std::string& card ) override {}
    virtual void notifyTimeExpired( Draft<>& draft,int chairIndex, uint32_t packId ) override {}
    virtual void notifyPostRoundTimerStarted( Draft<>& draft, int roundIndex, int ticksRemaining ) override {};
    virtual void notifyNewRound( Draft<>& draft, int roundIndex ) override {}
    virtual void notifyDraftComplete( Draft<>& draft ) override {}
    virtual void notifyDraftError( Draft<>& draft ) override {}
};
