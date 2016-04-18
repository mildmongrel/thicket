#include "Draft.h"

// A do-nothing version of Draft<>::Observer

class TestDraftObserver : public Draft<>::Observer
{
public:
    virtual void notifyPackQueueSizeChanged( Draft<>& draft, int chairIndex, int packQueueSize ) {}
    virtual void notifyNewPack( Draft<>& draft,int chairIndex, const std::string& pack, const std::vector<std::string>& unselectedCards ) {}
    virtual void notifyCardSelected( Draft<>& draft, int chairIndex, const std::string& pack, const std::string& card, bool autoSelected ) {}
    virtual void notifyCardSelectionError( Draft<>& draft, int chairIndex, const std::string& card ) {}
    virtual void notifyTimeExpired( Draft<>& draft,int chairIndex, const std::string& pack, const std::vector<std::string>& unselectedCards ) {}
    virtual void notifyNewRound( Draft<>& draft, int roundIndex, const std::string& round ) {}
    virtual void notifyDraftComplete( Draft<>& draft ) {}
};
