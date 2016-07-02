#ifndef DRAFTCHAIROBSERVER_H
#define DRAFTCHAIROBSERVER_H

#include "Draft.h"

template< typename TCardDescriptor = std::string >
class DraftChairObserver : public Draft<TCardDescriptor>::Observer {
public:

    typedef Draft<TCardDescriptor> TDraft;

    DraftChairObserver( int chairIndex ) : mChairIndex( chairIndex) {}
    virtual int getChairIndex() const { return mChairIndex; }

    virtual void notifyPackQueueSizeChanged( TDraft& draft, int packQueueSize ) = 0;
    virtual void notifyNewPack( TDraft& draft, uint32_t packId, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
    virtual void notifyCardSelected( TDraft& draft, uint32_t packId, const TCardDescriptor& card, bool autoSelected) = 0;
    virtual void notifyCardSelectionError( TDraft& draft, const TCardDescriptor& card ) = 0;
    virtual void notifyTimeExpired( TDraft& draft, uint32_t packId, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
//    virtual void notifyNewRound( TDraft& draft, int roundIndex, const TRoundDescriptor& round ) = 0;
//    virtual void notifyDraftComplete( TDraft& draft ) = 0;

private:

    virtual void notifyPackQueueSizeChanged( TDraft& draft, int chairIndex, int packQueueSize ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyPackQueueSizeChanged( draft, packQueueSize );
        }
    }

    virtual void notifyNewPack( TDraft& draft, int chairIndex, uint32_t packId, const std::vector<TCardDescriptor>& unselectedCards ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyNewPack( draft, packId, unselectedCards );
        }
    }

    virtual void notifyCardSelected( TDraft& draft, int chairIndex, uint32_t packId, const TCardDescriptor& card, bool autoSelected ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyCardSelected( draft, packId, card, autoSelected );
        }
    }

    virtual void notifyCardSelectionError( TDraft& draft, int chairIndex, const TCardDescriptor& card ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyCardSelectionError( draft, card );
        }
    }
    virtual void notifyTimeExpired( TDraft& draft, int chairIndex, uint32_t packId, const std::vector<TCardDescriptor>& unselectedCards ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyTimeExpired( draft, packId, unselectedCards );
        }
    }

    const int mChairIndex;
};

#endif
