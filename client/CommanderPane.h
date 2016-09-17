#ifndef COMMANDERPANE_H
#define COMMANDERPANE_H

#include <QList>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QWidget>
#include <QTabWidget>
#include <QScrollArea>
#include "clienttypes.h"
#include "CommanderPaneSettings.h"
#include "BasicLandCardDataMap.h"
#include "BasicLandQuantities.h"

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QBoxLayout;
QT_END_NAMESPACE

class CardData;
class CardViewerWidget;
class CardWidget;
class ImageLoaderFactory;
class BasicLandControlWidget;
class CommanderPane_TabWidget;

// This allows our custom types to be passed around in QVariant types
// with comboboxes.
Q_DECLARE_METATYPE( CardCategorizationType );
Q_DECLARE_METATYPE( CardSortCriterionVector );

#include "Logging.h"

// "Midnight Commander" widget.  Handles a single side of the interface; i.e.
// create two of these and hook them together to make an MC-like setup.
class CommanderPane : public QWidget
{
    Q_OBJECT

public:
    explicit CommanderPane( CommanderPaneSettings            commanderPaneSettings,
                            const std::vector<CardZoneType>& cardZones,
                            ImageLoaderFactory*              imageLoaderFactory,
                            const Logging::Config&           loggingConfig = Logging::Config(),
                            QWidget*                         parent = 0 );

    void setHideIfEmpty( const CardZoneType& cardZone, bool enable );

    CardZoneType getCurrentCardZone() const { return mCurrentCardZone; }
    bool setCurrentCardZone( const CardZoneType& cardZone );

    void setBasicLandCardDataMap( const BasicLandCardDataMap& val );

signals:

    // move card requested via right-click menu or other
    void cardZoneMoveRequest( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData, const CardZoneType& destCardZone );

    // move all requested via right-click menu or other
    void cardZoneMoveAllRequest( const CardZoneType& srcCardZone, const CardZoneType& destCardZone );

    // pre-selected via single-click
    void cardPreselected( const CardDataSharedPtr& cardData );

    // selected via double-click
    void cardSelected( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData );

    // basic land quantities updated
    void basicLandQuantitiesUpdate( const CardZoneType& srcCardZone, const BasicLandQuantities& basicLandQtys );

public slots:

    // Set card list for a zone in this pane.
    void setCards( const CardZoneType& cardZone, const QList<CardDataSharedPtr>& cards );

    // Update basic land quantities for a zone in this pane.
    void setBasicLandQuantities( const CardZoneType& cardZone, const BasicLandQuantities& basicLandQtys );

    // Set true to make the pane alert the user to an urgent draft event.
    void setDraftAlert( bool alert );

private slots:

    void handleCardPreselectRequested( CardWidget* cardWidget, const CardDataSharedPtr& cardData );
    void handleCardSelectRequested( const CardDataSharedPtr& cardData );
    void handleCardMoveRequested( const CardDataSharedPtr& cardData );
    void handleCardContextMenu( CardWidget* cardWidget, const CardDataSharedPtr& cardData, const QPoint& pos );
    void handleViewerContextMenu( const QPoint& pos );
    void handleZoomComboBoxChange( int index );
    void handleCategorizationComboBoxChange( int index );
    void handleSortComboBoxChange( int index );

private:

    void evaluateHiddenTabs();
    void showHiddenTab( const CardZoneType& cardZone );
    void updateTabSettings( const CardZoneType& cardZone );
    bool isBasicLandCardData( const CardDataSharedPtr& cardData, BasicLandType& basicOut );

private:

    CommanderPaneSettings mSettings;
    QVector<CardZoneType> mCardZones;
    CardZoneType mCurrentCardZone;
    ImageLoaderFactory* mImageLoaderFactory;

    QStackedWidget* mStackedWidget;
    CommanderPane_TabWidget* mCardViewerTabWidget;
    QMap<CardZoneType,CardViewerWidget*> mCardViewerWidgetMap;
    QMap<CardZoneType,BasicLandControlWidget*> mBasicLandControlWidgetMap;

    // List of zones to be hidden if they become empty.
    QSet<CardZoneType> mHideIfEmptyCardZoneSet;

    // Currently visible zones, aligned with tab indices.
    QList<CardZoneType> mVisibleCardZoneList;

    // This map holds empty hidden widgets by card zone.
    QMap<CardZoneType,QWidget*> mHiddenCardZoneWidgetMap;

    QColor mDefaultDraftTabTextColor;
    bool mDraftAlert;

    QSize mDefaultUnloadedSize;

    BasicLandCardDataMap mBasicLandCardDataMap;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};


//
// Subclasses of internal widgets with minor overrides for behavior.
// Would be nested classes but the Q_OBJECT macro doesn't work.
//


class CommanderPane_TabWidget : public QTabWidget
{
    Q_OBJECT
public:
    CommanderPane_TabWidget( QWidget* parent = 0 ) : QTabWidget( parent ) {}

    // Publicize protected method from QTabWidget.
    QTabBar* tabBar() const
    {
        return QTabWidget::tabBar();
    }
};;


class CommanderPane_CardScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    CommanderPane_CardScrollArea( QWidget* parent = 0 ) : QScrollArea( parent ) {}
protected:
    virtual QSize sizeHint() const override
    {
        return QSize( 750, 600 );
    }
};


#endif  // COMMANDERPANE_H
