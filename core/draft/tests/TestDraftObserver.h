#include "Draft.h"

// A do-nothing version of Draft<>::Observer

class TestDraftObserver : public Draft<>::Observer
{
public:
    virtual void notifyPackQueueSizeChanged( Draft<>& draft, int chairIndex, int packQueueSize ) override {}
    virtual void notifyNewPack( Draft<>& draft,int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override {}
    virtual void notifyCardSelected( Draft<>& draft, int chairIndex, uint32_t packId, const std::string& card, bool autoSelected ) override {}
    virtual void notifyCardSelectionError( Draft<>& draft, int chairIndex, const std::string& card ) override {}
    virtual void notifyTimeExpired( Draft<>& draft,int chairIndex, uint32_t packId, const std::vector<std::string>& unselectedCards ) override {}
    virtual void notifyNewRound( Draft<>& draft, int roundIndex ) override {}
    virtual void notifyDraftComplete( Draft<>& draft ) override {}
    virtual void notifyDraftError( Draft<>& draft ) override {}
};
