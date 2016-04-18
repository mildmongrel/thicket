#ifndef DRAFTCHAIROBSERVER_H
#define DRAFTCHAIROBSERVER_H

#include "Draft.h"

template< typename TRoundDescriptor = std::string,
          typename TPackDescriptor  = std::string,
          typename TCardDescriptor  = std::string >
class DraftChairObserver : public Draft<TRoundDescriptor,TPackDescriptor,TCardDescriptor>::Observer {
public:

    typedef Draft<TRoundDescriptor,TPackDescriptor,TCardDescriptor> TDraft;

    DraftChairObserver( int chairIndex ) : mChairIndex( chairIndex) {}
    virtual int getChairIndex() const { return mChairIndex; }

    virtual void notifyPackQueueSizeChanged( TDraft& draft, int packQueueSize ) = 0;
    virtual void notifyNewPack( TDraft& draft, const TPackDescriptor& pack, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
    virtual void notifyCardSelected( TDraft& draft, const TPackDescriptor& pack, const TCardDescriptor& card, bool autoSelected) = 0;
    virtual void notifyCardSelectionError( TDraft& draft, const TCardDescriptor& card ) = 0;
    virtual void notifyTimeExpired( TDraft& draft, const TPackDescriptor& pack, const std::vector<TCardDescriptor>& unselectedCards ) = 0;
//    virtual void notifyNewRound( TDraft& draft, int roundIndex, const TRoundDescriptor& round ) = 0;
//    virtual void notifyDraftComplete( TDraft& draft ) = 0;

private:

    virtual void notifyPackQueueSizeChanged( TDraft& draft, int chairIndex, int packQueueSize )
    {
        if( chairIndex == mChairIndex )
        {
            notifyPackQueueSizeChanged( draft, packQueueSize );
        }
    }

    virtual void notifyNewPack( TDraft& draft, int chairIndex, const TPackDescriptor& pack, const std::vector<TCardDescriptor>& unselectedCards )
    {
        if( chairIndex == mChairIndex )
        {
            notifyNewPack( draft, pack, unselectedCards );
        }
    }

    virtual void notifyCardSelected( TDraft& draft, int chairIndex, const TPackDescriptor& pack, const TCardDescriptor& card, bool autoSelected )
    {
        if( chairIndex == mChairIndex )
        {
            notifyCardSelected( draft, pack, card, autoSelected );
        }
    }

    virtual void notifyCardSelectionError( TDraft& draft, int chairIndex, const TCardDescriptor& card )
    {
        if( chairIndex == mChairIndex )
        {
            notifyCardSelectionError( draft, card );
        }
    }
    virtual void notifyTimeExpired( TDraft& draft, int chairIndex, const TPackDescriptor& pack, const std::vector<TCardDescriptor>& unselectedCards )
    {
        if( chairIndex == mChairIndex )
        {
            notifyTimeExpired( draft, pack, unselectedCards );
        }
    }

    const int mChairIndex;
};

#endif
