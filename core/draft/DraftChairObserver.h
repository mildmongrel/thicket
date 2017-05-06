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
    virtual void notifyNamedCardSelectionResult( TDraft&                draft,
                                                 uint32_t               packId,
                                                 bool                   result,
                                                 const TCardDescriptor& card ) = 0;
    virtual void notifyIndexedCardSelectionResult( TDraft&                             draft,
                                                   uint32_t                            packId,
                                                   bool                                result,
                                                   const std::vector<int>&             selectionIndices,
                                                   const std::vector<TCardDescriptor>& cards ) = 0;
    virtual void notifyCardAutoselection( TDraft&                draft,
                                          uint32_t               packId,
                                          const TCardDescriptor& card ) = 0;
    virtual void notifyTimeExpired( TDraft& draft, uint32_t packId ) = 0;

    // NOTE: Remaining non-chair-specific Observer methods are not overridden here.

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

    virtual void notifyNamedCardSelectionResult( TDraft&                draft,
                                                 int                    chairIndex,
                                                 uint32_t               packId,
                                                 bool                   result,
                                                 const TCardDescriptor& card ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyNamedCardSelectionResult( draft, packId, result, card );
        }
    }
    virtual void notifyIndexedCardSelectionResult( TDraft&                             draft,
                                                   int                                 chairIndex,
                                                   uint32_t                            packId,
                                                   bool                                result,
                                                   const std::vector<int>&             selectionIndices,
                                                   const std::vector<TCardDescriptor>& cards ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyIndexedCardSelectionResult( draft, packId, result, selectionIndices, cards );
        }
    }
    virtual void notifyCardAutoselection( TDraft&                draft,
                                          int                    chairIndex,
                                          uint32_t               packId,
                                          const TCardDescriptor& card ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyCardAutoselection( draft, packId, card );
        }
    }

    virtual void notifyTimeExpired( TDraft& draft, int chairIndex, uint32_t packId ) override
    {
        if( chairIndex == mChairIndex )
        {
            notifyTimeExpired( draft, packId );
        }
    }

    const int mChairIndex;
};

#endif
