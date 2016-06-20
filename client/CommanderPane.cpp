#include "CommanderPane.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QScrollArea>
#include <QLabel>
#include <QMenu>
#include <QWidgetAction>
#include <QPushButton>
#include <QTabWidget>
#include <QStackedWidget>

#include "CardData.h"
#include "qtutils_widget.h"

#include "CardViewerWidget.h"
#include "SizedSvgWidget.h"
#include "BasicLandControlWidget.h"
#include "BasicLandQuantities.h"
#include "DraftTimerWidget.h"

static const QString RESOURCE_SVG_CARD_BACK( ":/card-back-portrait.svg" );

CommanderPane::CommanderPane( CommanderPaneSettings            commanderPaneSettings,
                              const std::vector<CardZoneType>& cardZones,
                              ImageLoaderFactory*              imageLoaderFactory,
                              const Logging::Config&           loggingConfig,
                              QWidget*                         parent )
  : QWidget( parent ),
    mSettings( commanderPaneSettings ),
    mCardZones( QVector<CardZoneType>::fromStdVector( cardZones ) ),
    mImageLoaderFactory( imageLoaderFactory ),
    mDraftTimerWidget( nullptr ),
    mDraftPackQueueLayout( nullptr ),
    mDraftAlert( false ),
    mDefaultUnloadedSize( QSize( 150, 225 ) ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    QGridLayout *outerLayout = new QGridLayout();
    outerLayout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( outerLayout );

    mStackedWidget = new QStackedWidget();
    mCardViewerTabWidget = new CommanderPane_TabWidget();

    // Set up all CardViewerWidgets in tabs for each zone.
    for( auto cardZone : cardZones )
    {
        // Widget to hold the cards.  Make the background white to hide
        // the white corners on JPG cards returned by gatherer. 
        CardViewerWidget *cardViewerWidget = new CardViewerWidget( mImageLoaderFactory, mLoggingConfig.createChildConfig("cardviewerwidget"), this );
        cardViewerWidget->setContextMenuPolicy( Qt::CustomContextMenu );
        cardViewerWidget->setDefaultUnloadedSize( mDefaultUnloadedSize );
        cardViewerWidget->setSortCriteria( { CARD_SORT_CRITERION_NAME } );
        connect(cardViewerWidget, SIGNAL(cardDoubleClicked(const CardDataSharedPtr&)),
                this, SLOT(handleCardDoubleClicked(const CardDataSharedPtr&)));
        connect(cardViewerWidget, SIGNAL(cardShiftClicked(const CardDataSharedPtr&)),
                this, SLOT(handleCardShiftClicked(const CardDataSharedPtr&)));
        connect(cardViewerWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
                this, SLOT(handleViewerContextMenu(const QPoint&)));
        connect(cardViewerWidget, SIGNAL(cardContextMenuRequested(const CardDataSharedPtr&,const QPoint&)),
                this, SLOT(handleCardContextMenu(const CardDataSharedPtr&,const QPoint&)));

        mCardViewerWidgetMap[cardZone] = cardViewerWidget;

        CommanderPane_CardScrollArea *cardScrollArea = new CommanderPane_CardScrollArea();
        // Important or else the widget won't expand to the size of the
        // QScrollArea, resulting in the FlowLayout showing up as a vertical
        // list of items rather than a flow layout.
        cardScrollArea->setWidgetResizable(true);
        cardScrollArea->setWidget( cardViewerWidget );
        cardScrollArea->setMinimumWidth( 300 );
        cardScrollArea->setMinimumHeight( 200 );

        // Add the tab.  Title will be updated after internal maps are set up.
        int tabIndex = mCardViewerTabWidget->addTab( cardScrollArea, QString() );
        mVisibleCardZoneList.append( cardZone );

        // Create a widget for a stack widget that corresponds to custom
        // controls based on card zone.  Currently the tab indices MUST match
        // the stack indices, so a widget shoudl be added here even if it's
        // an empty one.
        if( (cardZone == CARD_ZONE_MAIN) || (cardZone == CARD_ZONE_SIDEBOARD) )
        {
            // Only main and sideboard allow adding/removing basic lands.
            QWidget *containerWidget = new QWidget();
            QHBoxLayout *containerLayout = new QHBoxLayout();
            containerLayout->setContentsMargins( 0, 0, 0, 0 );
            containerWidget->setLayout( containerLayout );

            BasicLandControlWidget *basicLandControlWidget = new BasicLandControlWidget();

            // Right-justify the items within the stacked widget.
            containerLayout->addStretch( 1 );

            containerLayout->addWidget( basicLandControlWidget );
            containerLayout->addSpacing( 10 );
            mStackedWidget->addWidget( containerWidget );

            // wire the basic land qtys signal to our cardviewer widget
            connect(basicLandControlWidget, SIGNAL(basicLandQuantitiesUpdate(const BasicLandQuantities&)),
                    cardViewerWidget, SLOT(setBasicLandQuantities(const BasicLandQuantities&)));

            // forward the basic land qtys signal from from the widget to
            // this class's signal, adding our zone
            connect( basicLandControlWidget, &BasicLandControlWidget::basicLandQuantitiesUpdate,
                     [this,cardZone] (const BasicLandQuantities& qtys)
                     {
                         mLogger->debug( "forwarding basic land qtys signal, zone={}", cardZone );
                         emit basicLandQuantitiesUpdate( cardZone, qtys );
                     });

            mBasicLandControlWidgetMap[cardZone] = basicLandControlWidget;
        }
        else if( cardZone == CARD_ZONE_DRAFT )
        {
            // One-off: grab the default draft tab text color here.
            mDefaultDraftTabTextColor = mCardViewerTabWidget->tabBar()->tabTextColor( tabIndex );

            QWidget *containerWidget = new QWidget();
            QHBoxLayout *containerLayout = new QHBoxLayout();
            containerLayout->setContentsMargins( 0, 0, 0, 0 );
            containerWidget->setLayout( containerLayout );

            // Right-justify the items within the stacked widget.
            containerLayout->addStretch( 1 );

            mDraftPackQueueLayout = new QBoxLayout( QBoxLayout::RightToLeft );
            mDraftPackQueueLayout->setContentsMargins( 0, 0, 0, 0 );
            mDraftPackQueueLayout->setSpacing( 10 );
            containerLayout->addLayout( mDraftPackQueueLayout );

            containerLayout->addSpacing( 15 );

            mDraftTimerWidget = new DraftTimerWidget( DraftTimerWidget::SIZE_LARGE, 10 );
            containerLayout->addWidget( mDraftTimerWidget );

            containerLayout->addSpacing( 10 );

            mStackedWidget->addWidget( containerWidget );
        }
        else
        {
            // Nothing for this zone, add an empty widget.
            mStackedWidget->addWidget( new QWidget() );
        }

        updateTabSettings( cardZone );
    }

    connect( mCardViewerTabWidget, &QTabWidget::currentChanged,
             [this] (int index)
             {
                 mCurrentCardZone = mVisibleCardZoneList[index];
                 mLogger->debug( "current zone changed to {}", mCurrentCardZone );
                 mStackedWidget->setCurrentIndex( index );
                 evaluateHiddenTabs();
             });

    outerLayout->addWidget( mCardViewerTabWidget, 0, 0, 1, 2 );
    outerLayout->setRowStretch( 0, 1 );

    // Set active tab and current card zone to the first tab.
    mCardViewerTabWidget->setCurrentIndex( 0 );
    mCurrentCardZone = mVisibleCardZoneList.value( 0, CARD_ZONE_MAIN );

    QHBoxLayout* controlLayout = new QHBoxLayout();

    // A little space to the left.
    controlLayout->addSpacing( 10 );

    // Add a zoom combobox to the control area.
    QComboBox* zoomComboBox = new QComboBox();
    zoomComboBox->addItem( "25%", 0.25f );
    zoomComboBox->addItem( "50%", 0.5f );
    zoomComboBox->addItem( "75%", 0.75f );
    zoomComboBox->addItem( "90%", 0.9f );
    zoomComboBox->addItem( "100%", 1.0f );
    zoomComboBox->addItem( "110%", 1.10f );
    zoomComboBox->addItem( "125%", 1.25f );
    zoomComboBox->addItem( "150%", 1.5f );
    connect(zoomComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleZoomComboBoxChange(int)));
    controlLayout->addWidget( zoomComboBox );
    controlLayout->addSpacing( 10 );

    // Set zoom based on settings.
    QString zoomSetting = mSettings.getZoom();
    zoomComboBox->setCurrentText( !zoomSetting.isEmpty() ? zoomSetting : "100%" );

    // Add a categorization combobox to the control area.
    QLabel* catLabel = new QLabel( "Categorize:" );
    controlLayout->addWidget( catLabel );
    QComboBox *catComboBox = new QComboBox();
    catComboBox->addItem( "None", QVariant::fromValue( CARD_CATEGORIZATION_NONE ) );
    catComboBox->addItem( "CMC", QVariant::fromValue( CARD_CATEGORIZATION_CMC ) );
    catComboBox->addItem( "Color", QVariant::fromValue( CARD_CATEGORIZATION_COLOR ) );
    catComboBox->addItem( "Type", QVariant::fromValue( CARD_CATEGORIZATION_TYPE ) );
    catComboBox->addItem( "Rarity", QVariant::fromValue( CARD_CATEGORIZATION_RARITY ) );
    connect(catComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleCategorizationComboBoxChange(int)));
    controlLayout->addWidget( catComboBox );
    controlLayout->addSpacing( 10 );

    // Set categorization based on settings.
    QString catSetting = mSettings.getCategorization();
    catComboBox->setCurrentText( !catSetting.isEmpty() ? catSetting : "None" );

    // Add a sorting combobox to the control area.
    QLabel* sortLabel = new QLabel( "Sort:" );
    controlLayout->addWidget( sortLabel );
    QComboBox* sortComboBox = new QComboBox();
    sortComboBox->addItem( "Name",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "CMC",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_CMC, CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "CMC (after Color)",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_CMC, CARD_SORT_CRITERION_COLOR, CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "Color",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_COLOR, CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "Color (after Rarity)",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_COLOR, CARD_SORT_CRITERION_RARITY, CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "Rarity",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_RARITY, CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "Rarity (after Color)",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_RARITY, CARD_SORT_CRITERION_COLOR, CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "Type",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_TYPE, CARD_SORT_CRITERION_NAME } ) ) );
    sortComboBox->addItem( "Type (after Color)",
            QVariant::fromValue( CardSortCriterionVector( { CARD_SORT_CRITERION_TYPE, CARD_SORT_CRITERION_COLOR, CARD_SORT_CRITERION_NAME } ) ) );
    connect(sortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleSortComboBoxChange(int)));
    controlLayout->addWidget( sortComboBox );

    // Set sorting based on settings.
    QString sortSetting = mSettings.getSort();
    sortComboBox->setCurrentText( !sortSetting.isEmpty() ? sortSetting : "Name" );

    // Ensure some space to the right of the layout.
    controlLayout->addSpacing( 10 );

    // Add a stretch to the control outerLayout so everything else is right-justified.
    controlLayout->addStretch( 1 );

    outerLayout->addLayout( controlLayout, 1, 0, 1, 1, Qt::AlignLeft );
    outerLayout->addWidget( mStackedWidget, 1, 1, 1, 1, Qt::AlignRight );

}
 

void
CommanderPane::setHideIfEmpty( const CardZoneType& cardZone, bool enable )
{
    if( enable )
    {
        mHideIfEmptyCardZoneSet.insert( cardZone );
        evaluateHiddenTabs();
    }
    else
    {
        mHideIfEmptyCardZoneSet.remove( cardZone );
        showHiddenTab( cardZone );
    }
}


bool
CommanderPane::setCurrentCardZone( const CardZoneType& cardZone )
{
    // Only switch if the zone is visible.
    int tabIndex = mVisibleCardZoneList.indexOf( cardZone );
    if( tabIndex >= 0 )
    {
        mCardViewerTabWidget->setCurrentIndex( tabIndex );
        return true;
    }
    else
    {
        return false;
    }
}


void
CommanderPane::setBasicLandCardDataMap( const BasicLandCardDataMap& val )
{
    mBasicLandCardDataMap = val;
    for( auto iter = mCardViewerWidgetMap.constBegin(); iter != mCardViewerWidgetMap.constEnd(); ++iter )
    {
        CardViewerWidget* cardViewerWidget = iter.value();
        cardViewerWidget->setBasicLandCardDataMap( val );
    }
}


void
CommanderPane::setCards( const CardZoneType& cardZone, const QList<CardDataSharedPtr>& cards )
{
    auto iter = mCardViewerWidgetMap.find( cardZone );
    if( iter != mCardViewerWidgetMap.end() )
    {
        CardViewerWidget *cardViewerWidget = iter.value();
        cardViewerWidget->setCards( cards );
        updateTabSettings( cardZone );
    }

    // If this tab is or could be hidden, update visible tabs.
    if( mHideIfEmptyCardZoneSet.contains( cardZone ) )
    {
        evaluateHiddenTabs();
    }
}


void
CommanderPane::setBasicLandQuantities( const CardZoneType& cardZone, const BasicLandQuantities& basicLandQtys )
{
    // This will behave as if the widget was updated and signal the
    // card viewer widget to update accordingly.
    auto iter = mBasicLandControlWidgetMap.find( cardZone );
    if( iter != mBasicLandControlWidgetMap.end() )
    {
        BasicLandControlWidget *widget = iter.value();
        widget->setBasicLandQuantities( basicLandQtys );
        updateTabSettings( cardZone );
    }
}


void
CommanderPane::setDraftQueuedPacks( int count )
{
    mLogger->debug( "draft queued packs changed, count={}", count );

    if( mDraftPackQueueLayout != nullptr )
    {
        const int oldCount = mDraftPackQueueLayout->count();
        if( count <= 0 )
        {
            qtutils::clearLayout( mDraftPackQueueLayout );
            mDraftPackQueueLayout->update();
        }
        else if( count > oldCount )
        {
            // add widgets until we get up to count
            for( int i = 0; i < count - oldCount; ++i )
            {
                // Size the pack graphic to the same height as the timer widget
                QSize scalingSize( std::numeric_limits<int>::max(), mDraftTimerWidget->height() );
                SizedSvgWidget *packWidget = new SizedSvgWidget( scalingSize, Qt::KeepAspectRatio );
                packWidget->load( RESOURCE_SVG_CARD_BACK );
                mDraftPackQueueLayout->addWidget( packWidget );
            }
        }
        else
        {
            // delete widgets until we get down to count
            for( int i = 0; i < oldCount - count; ++i )
            {
                QLayoutItem* item = mDraftPackQueueLayout->takeAt( 0 );
                delete item->widget();
            }
        }
    }
}


void
CommanderPane::setDraftTickCount( int count )
{
    if( mDraftTimerWidget != nullptr )
    {
        mDraftTimerWidget->setCount( count );
    }
}


void
CommanderPane::setDraftAlert( bool alert )
{
    mLogger->debug( "draft alert status changed: {}", alert );
    mDraftAlert = alert;
    updateTabSettings( CARD_ZONE_DRAFT );
}


void
CommanderPane::handleZoomComboBoxChange( int index )
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(QObject::sender());
    float zoomFactor = comboBox->itemData( index ).toFloat();
    for( auto iter = mCardViewerWidgetMap.constBegin(); iter != mCardViewerWidgetMap.constEnd(); ++iter )
    {
        CardViewerWidget* cardViewerWidget = iter.value();
        cardViewerWidget->setZoomFactor( zoomFactor );
    }

    mSettings.setZoom( comboBox->currentText() );
}


void
CommanderPane::handleCategorizationComboBoxChange( int index )
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(QObject::sender());
    CardCategorizationType cat = comboBox->itemData( index ).value<CardCategorizationType>();
    mLogger->debug( "categorization changed: index={}, cat={}", index, cat );
    for( auto iter = mCardViewerWidgetMap.constBegin(); iter != mCardViewerWidgetMap.constEnd(); ++iter )
    {
        CardViewerWidget* cardViewerWidget = iter.value();
        cardViewerWidget->setCategorization( cat );
    }

    mSettings.setCategorization( comboBox->currentText() );
}


void
CommanderPane::handleSortComboBoxChange( int index )
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(QObject::sender());
    mLogger->debug( "sort changed: {}", index );
    CardSortCriterionVector sortCriteria = comboBox->itemData( index ).value<CardSortCriterionVector>();
    for( auto iter = mCardViewerWidgetMap.constBegin(); iter != mCardViewerWidgetMap.constEnd(); ++iter )
    {
        CardViewerWidget* cardViewerWidget = iter.value();
        cardViewerWidget->setSortCriteria( sortCriteria );
    }

    mSettings.setSort( comboBox->currentText() );
}


void
CommanderPane::handleCardDoubleClicked( const CardDataSharedPtr& cardData )
{
    mLogger->debug( "card selected: {}", cardData->getName() );
    emit cardSelected( mCurrentCardZone, cardData );
}


void
CommanderPane::handleCardShiftClicked( const CardDataSharedPtr& cardData )
{
    // Ignore shift-clicks from draft zone.
    if( mCurrentCardZone == CARD_ZONE_DRAFT ) return;

    mLogger->debug( "card shift-clicked: {}", cardData->getName() );

    BasicLandType basic;
    if( isBasicLandCardData( cardData, basic ) )
    {
        // As if the user had decreased the basic lands via the widget.
        mBasicLandControlWidgetMap[mCurrentCardZone]->decrementBasicLandQuantity( basic );
    }
    else
    {
        emit cardZoneMoveRequest( mCurrentCardZone, cardData, CARD_ZONE_JUNK ); 
    }
}


void
CommanderPane::handleCardContextMenu( const CardDataSharedPtr& cardData, const QPoint& pos )
{
    mLogger->debug( "card context menu: {}", cardData->getName() );

    CardViewerWidget* cardViewerWidget = mCardViewerWidgetMap[mCurrentCardZone];
    const QPoint globalPos = cardViewerWidget->mapToGlobal( pos );

    // Set up a pop-up menu.
    QMenu menu;
    QWidgetAction *title = new QWidgetAction(0);
    QString cardName = QString::fromStdString( cardData->getName() );
    QLabel *label = new QLabel( "<b><center>" + cardName + "</center></b>" );
    title->setDefaultWidget( label );
    menu.addAction( title );
    menu.addSeparator();

    // Set up menu actions based on card type or assigned zone.
    QAction *mainAction = 0;
    QAction *sbAction = 0;
    QAction *junkAction = 0;
    QAction *removeLandAction = 0;

    BasicLandType basic;
    if( mCurrentCardZone == CARD_ZONE_DRAFT )
    {
        mainAction = menu.addAction( "Draft to Main" );
        sbAction = menu.addAction( "Draft to Sideboard" );
        junkAction = menu.addAction( "Draft to Junk" );
    }
    else if( isBasicLandCardData( cardData, basic ) )
    {
        removeLandAction = menu.addAction( "Remove" );
    }
    else
    {
        if( mCurrentCardZone != CARD_ZONE_MAIN ) mainAction = menu.addAction( "Move to Main" );
        if( mCurrentCardZone != CARD_ZONE_SIDEBOARD ) sbAction = menu.addAction( "Move to Sideboard" );
        if( mCurrentCardZone != CARD_ZONE_JUNK ) junkAction = menu.addAction( "Move to Junk" );
    }

    // Execute the menu and act on result.
    QAction *result = menu.exec( globalPos );

    if( result == 0 ) return;
    else if( result == mainAction ) emit cardZoneMoveRequest( mCurrentCardZone, cardData, CARD_ZONE_MAIN ); 
    else if( result == sbAction ) emit cardZoneMoveRequest( mCurrentCardZone, cardData, CARD_ZONE_SIDEBOARD ); 
    else if( result == junkAction ) emit cardZoneMoveRequest( mCurrentCardZone, cardData, CARD_ZONE_JUNK ); 
    else if( result == removeLandAction )
    {
        // As if the user had decreased the basic lands via the widget.
        mBasicLandControlWidgetMap[mCurrentCardZone]->decrementBasicLandQuantity( basic );
    }
}


void
CommanderPane::handleViewerContextMenu( const QPoint& pos )
{
    mLogger->debug( "viewer context menu" );

    // Nothing to do in draft context.
    if( mCurrentCardZone == CARD_ZONE_DRAFT ) return;

    CardViewerWidget* cardViewerWidget = mCardViewerWidgetMap[mCurrentCardZone];
    const QPoint globalPos = cardViewerWidget->mapToGlobal( pos );

    // Set up a pop-up menu.
    QMenu menu;

    // Set up menu actions based on card type or assigned zone.
    QAction *mainAction = 0;
    QAction *sbAction = 0;
    QAction *junkAction = 0;

    if( mCurrentCardZone != CARD_ZONE_MAIN ) mainAction = menu.addAction( "Move all to Main" );
    if( mCurrentCardZone != CARD_ZONE_SIDEBOARD ) sbAction = menu.addAction( "Move all to Sideboard" );
    if( mCurrentCardZone != CARD_ZONE_JUNK ) junkAction = menu.addAction( "Move all to Junk" );

    // Execute the menu and act on result.
    QAction *result = menu.exec( globalPos );

    if( result == 0 ) return;
    else if( result == mainAction ) emit cardZoneMoveAllRequest( mCurrentCardZone, CARD_ZONE_MAIN ); 
    else if( result == sbAction ) emit cardZoneMoveAllRequest( mCurrentCardZone, CARD_ZONE_SIDEBOARD ); 
    else if( result == junkAction ) emit cardZoneMoveAllRequest( mCurrentCardZone, CARD_ZONE_JUNK ); 
    mLogger->debug( "result: {}", (size_t)result );
}


void
CommanderPane::evaluateHiddenTabs()
{
    for( CardZoneType cardZone : mHideIfEmptyCardZoneSet )
    {
        // Make sure the zone is actually part of the overall list of zones.
        if( !mCardZones.contains( cardZone ) ) return;

        // Get number of cards and tab index for the zone.
        int numCards = mCardViewerWidgetMap[cardZone]->getTotalCardCount();
        int tabIndex = mVisibleCardZoneList.indexOf( cardZone );

        if( (numCards == 0) && (tabIndex >= 0) )
        {
            // This tab is visible but empty so it needs to be hidden.  Hide
            // it unless it's the current tab.   (It can be hidden once the
            // user navigates away to a different tab.)
            if( tabIndex != mCardViewerTabWidget->currentIndex() )
            {
                mVisibleCardZoneList.removeAt( tabIndex );
                mHiddenCardZoneWidgetMap.insert( cardZone, mCardViewerTabWidget->widget( tabIndex ) );
                mCardViewerTabWidget->removeTab( tabIndex );
            }
        }
        else if( (numCards > 0) && (tabIndex < 0) )
        {
            // This tab is hidden but has cards so it needs to be made visible.
            showHiddenTab( cardZone );
        }
    }
}


void
CommanderPane::showHiddenTab( const CardZoneType& cardZone )
{
    // Make sure this is a zone being managed and it's not visible already.
    if( !mCardZones.contains( cardZone ) ) return;
    if( mVisibleCardZoneList.contains( cardZone ) ) return;

    // Find the right place to insert the tab based on all managed
    // zones and what's visible.
    int insertIndex = 0;
    for( CardZoneType z : mCardZones )
    {
        if( z == cardZone ) break;
        if( mVisibleCardZoneList.contains( z ) ) ++insertIndex;
    }

    mVisibleCardZoneList.insert( insertIndex, cardZone );
    QWidget* widget = mHiddenCardZoneWidgetMap.take( cardZone );
    if( widget != nullptr )
    {
        mCardViewerTabWidget->insertTab( insertIndex, widget, QString() );
        updateTabSettings( cardZone );
    }
}


void
CommanderPane::updateTabSettings( const CardZoneType& cardZone )
{
    int tabIndex = mVisibleCardZoneList.indexOf( cardZone );
    if( tabIndex >= 0 )
    {
        const int numCards = mCardViewerWidgetMap[cardZone]->getTotalCardCount();
        const QString text = QString::fromStdString( stringify( cardZone ) ) + " (" + QString::number( numCards ) + ")";
        mCardViewerTabWidget->setTabText( tabIndex, text );

        QTabBar* tabBar = mCardViewerTabWidget->tabBar();
        switch( cardZone )
        {
            case CARD_ZONE_DRAFT:
                tabBar->setTabTextColor( tabIndex, mDraftAlert ? QColor(Qt::red) : mDefaultDraftTabTextColor );
                tabBar->setTabToolTip( tabIndex, tr("Cards eligible to be selected") );
                break;
            case CARD_ZONE_AUTO:
                tabBar->setTabTextColor( tabIndex, QColor(Qt::blue) );
                tabBar->setTabToolTip( tabIndex, tr("Cards that have been selected for you automatically") );
                break;
            default:
                break;
        }
    }
}


bool
CommanderPane::isBasicLandCardData( const CardDataSharedPtr& cardData, BasicLandType& basicOut )
{
    for( auto basic : gBasicLandTypeArray )
    {
        if( cardData == mBasicLandCardDataMap.getCardData( basic ) )
        {
            basicOut = basic;
            return true;
        }
    }
    return false;
}

