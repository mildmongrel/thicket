#ifndef CARDVIEWERWIDGET_H
#define CARDVIEWERWIDGET_H

#include <functional>
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSet>
#include "clienttypes.h"
#include "BasicLandCardDataMap.h"
#include "BasicLandQuantities.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QLabel;
class QGridLayout;
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

    // Set data for basic lands.  This can change if new card data is
    // created, e.g. for a different image set.
    void setBasicLandCardDataMap( const BasicLandCardDataMap& val );

    // Set additional space below cards.
    void setFooterSpacing( int spacing ) { mFooterSpacing = spacing; }

    // Set card list.  (Does not include basic lands.)
    virtual void setCards( const QList<CardDataSharedPtr>& cards );

    // Set cards that are selected.  (Use empty map to reset.)
    virtual void setSelectedCards( const QMap<CardDataSharedPtr,SelectedCardData>& selectedCards );

    // Total number of cards in the widget, including basic lands.
    int getTotalCardCount() const { return mCardsList.size() + mBasicLandQtys.getTotalQuantity(); }

    // Set a preselected card, all others will be un-preselected.
    void setPreselected( CardWidget* cardWidget );

    // Set the widget to reflect waiting for player turn (applies to some draft types)
    virtual void setWaitingForTurn( bool waiting ) {}

signals:

    void cardPreselectRequested( CardWidget* cardWidget, const CardDataSharedPtr& cardData );
    void cardSelectRequested( const CardDataSharedPtr& cardData );
    void cardMoveRequested( const CardDataSharedPtr& cardData );
    void cardIndicesSelectRequested( const QList<int>& indices );

    // card context menu requested
    void cardContextMenuRequested( CardWidget* cardWidget, const CardDataSharedPtr& cardData, const QPoint& pos );

public slots:

    void setBasicLandQuantities( const BasicLandQuantities& basicLandQtys );

    void setZoomFactor( float zoomFactor );

    void setCategorization( const CardCategorizationType& categorization );

    // Enable/configure sorting.  Cards are stable-sorted in reverse order
    // of the criteria.
    void setSortCriteria( const CardSortCriterionVector& sortCriteria );

    // Alter appearance for an alerted state.
    void setAlert( bool alert );

    // Set cards in this widget to be preselectable.
    void setCardsPreselectable( bool preselectable );

protected:

    void addAlertableSubwidget( QWidget* w );
    void clearAlertableSubwidgets();

    // Overridden to ensure proper stylesheet handling.
    virtual void paintEvent( QPaintEvent *pe ) override;

    QVBoxLayout* mLayout;

    QList<CardDataSharedPtr> mCardsList;
    QList<CardWidget*>       mCardWidgetsList;

    QMap<CardDataSharedPtr,SelectedCardData> mSelectedCards;

    // REFACTOR NOTE: These members may be private once a protected
    // function is in place for taking/creating a cardwidget.
    ImageLoaderFactory* const mImageLoaderFactory;
    bool mCardsPreselectable;

    int mFooterSpacing;
    float mZoomFactor;

    bool mAlerted;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;

private slots:

    void handleCardPreselectRequested();
    void handleCardSelectRequested();
    void handleCardMoveRequested();
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

    virtual void selectedCardsUpdateHandler() {};

private:

    // Sorting functions for CardDataSharedPtr types.
    static bool compareCardDataName( CardDataSharedPtr a, CardDataSharedPtr b ) { return a->getName() < b->getName(); }
    static bool compareCardDataCMCs( CardDataSharedPtr a, CardDataSharedPtr b ) { return a->getCMC() < b->getCMC(); }
    static bool compareCardDataColors( CardDataSharedPtr a, CardDataSharedPtr b );
    static bool compareCardDataRarity( CardDataSharedPtr a, CardDataSharedPtr b ) { return a->getRarity() < b->getRarity(); }
    static bool compareCardDataTypes( CardDataSharedPtr a, CardDataSharedPtr b );

private:

    QList<FlowLayout*> mFilteredCardsLayouts;
    QList<QLabel*> mFilteredCardsLabels;
    QWidget *mBasicLandWidget;

    FilterVectorType mFilters;

    SortFunctionVectorType mSortFunctions;

    QSet<QWidget*>           mAlertableSubwidgets;

    BasicLandQuantities mBasicLandQtys;
    BasicLandCardDataMap mBasicLandCardDataMap;
};

#endif  // CARDVIEWERWIDGET_H
