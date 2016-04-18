#ifndef CARDVIEWERWIDGET_H
#define CARDVIEWERWIDGET_H

#include <functional>
#include <QWidget>
#include <QList>
#include "clienttypes.h"
#include "BasicLandCardDataMap.h"
#include "BasicLandQuantities.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QLabel;
QT_END_NAMESPACE

class CardData;
class CardWidget;
class ImageLoaderFactory;
class FlowLayout;

#include "Logging.h"

// Card Viewer widget - hold a layout of card widgets and manages sorting,
// zooming, forwarding card signals, etc.
class CardViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CardViewerWidget( ImageLoaderFactory*    imageLoaderFactory,
                               const Logging::Config& loggingConfig = Logging::Config(),
                               QWidget*               parent = 0 );
    virtual ~CardViewerWidget();

    void setDefaultUnloadedSize( const QSize& size ) { mDefaultUnloadedSize = size; }
    void setBasicLandCardDataMap( const BasicLandCardDataMap& val ) { mBasicLandCardDataMap = val; }

    // Total number of cards in the widget, including basic lands.
    int getTotalCardCount() const { return mCardsList.size() + mBasicLandQtys.getTotalQuantity(); }

signals:

    void cardSingleClicked( const CardDataSharedPtr& cardData );
    void cardDoubleClicked( const CardDataSharedPtr& cardData );
    void cardShiftClicked( const CardDataSharedPtr& cardData );

    // card context menu requested
    void cardContextMenuRequested( const CardDataSharedPtr& cardData, const QPoint& pos );

public slots:

    // Set card list for a zone.  (Does not include basic lands.)
    void setCards( const QList<CardDataSharedPtr>& cards );

    void setBasicLandQuantities( const BasicLandQuantities& basicLandQtys );

    void setZoomFactor( float zoomFactor );

    void setCategorization( const CardCategorizationType& categorization );

    // Enable/configure sorting.  Cards are stable-sorted in reverse order
    // of the criteria.
    void setSortCriteria( const CardSortCriterionVector& sortCriteria );

    // Alter appearance for an alerted state.
    void setAlert( bool alert );

protected:

    // Overridden to ensure proper stylesheet handling.
    virtual void paintEvent( QPaintEvent *pe ) override;

private slots:

    void handleCardDoubleClicked();
    void handleCardShiftClicked();
    void handleCardContextMenu( const QPoint& pos );

private:

    typedef std::function<bool(CardDataSharedPtr,CardDataSharedPtr)> SortFunctionType;
    typedef std::vector<SortFunctionType> SortFunctionVectorType;

    // This filtering functionality could eventually be externally exposed.
    typedef std::function<bool(const CardDataSharedPtr&)> FilterFunctionType;
    struct Filter
    {
        explicit Filter( const std::string& nameParam, const FilterFunctionType& filterFuncParam )
          : name( nameParam ), filterFunc( filterFuncParam ) {}

        std::string        name;
        FilterFunctionType filterFunc;
    };
    typedef std::vector<Filter> FilterVectorType;

    void setFilters( const FilterVectorType& filters );

private:

    // Sorting functions for CardDataSharedPtr types.
    static bool compareCardDataName( CardDataSharedPtr a, CardDataSharedPtr b ) { return a->getName() < b->getName(); }
    static bool compareCardDataCMCs( CardDataSharedPtr a, CardDataSharedPtr b ) { return a->getCMC() < b->getCMC(); }
    static bool compareCardDataColors( CardDataSharedPtr a, CardDataSharedPtr b );
    static bool compareCardDataRarity( CardDataSharedPtr a, CardDataSharedPtr b ) { return a->getRarity() < b->getRarity(); }
    static bool compareCardDataTypes( CardDataSharedPtr a, CardDataSharedPtr b );

private:

    ImageLoaderFactory* const mImageLoaderFactory;

    QVBoxLayout* mLayout;

    QList<FlowLayout*> mFilteredCardsLayouts;
    QList<QLabel*> mFilteredCardsLabels;
    QWidget *mBasicLandWidget;

    QList<CardDataSharedPtr> mCardsList;
    QList<CardWidget*> mCardWidgetsList;
    FilterVectorType mFilters;

    SortFunctionVectorType mSortFunctions;
    QSize mDefaultUnloadedSize;
    float mZoomFactor;

    BasicLandQuantities mBasicLandQtys;
    BasicLandCardDataMap mBasicLandCardDataMap;

    bool mAlerted;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;

};

#endif  // CARDVIEWERWIDGET_H
