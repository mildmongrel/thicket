#include "CommanderPane.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QScrollArea>
#include <QLabel>
#include <QMenu>
#include <QWidgetAction>
#include <QTabWidget>
#include <QPropertyAnimation>
#include <QToolButton>
#include <QEvent>

#include "CardData.h"
#include "qtutils_core.h"
#include "qtutils_widget.h"

#include "CardViewerWidget.h"
#include "GridCardViewerWidget.h"
#include "BasicLandControlWidget.h"
#include "BasicLandQuantities.h"
#include "CapsuleIndicator.h"

static const int CAPSULE_HEIGHT = 35;
static const int CAPSULE_SPACING = 5;

CommanderPane::CommanderPane( CommanderPaneSettings            commanderPaneSettings,
                              const std::vector<CardZoneType>& cardZones,
                              ImageLoaderFactory*              imageLoaderFactory,
                              const Logging::Config&           loggingConfig,
                              QWidget*                         parent )
  : QWidget( parent ),
    mSettings( commanderPaneSettings ),
    mCardZones( QVector<CardZoneType>::fromStdVector( cardZones ) ),
    mImageLoaderFactory( imageLoaderFactory ),
    mBoosterDraftQueuedPacksCapsule( nullptr ),
    mBoosterDraftTimeRemainingCapsule( nullptr ),
    mGridDraftTimeRemainingCapsule( nullptr ),
    mDraftActive( false ),
    mDraftAlert( false ),
    mDraftQueuedPacks( -1 ),
    mDraftTimeRemaining( -1 ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    QVBoxLayout *outerLayout = new QVBoxLayout();
    outerLayout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( outerLayout );

    mZoneViewerTabWidget = new CommanderPane_TabWidget();

    QToolButton* modifyLandButton = new QToolButton();
    modifyLandButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    modifyLandButton->setCheckable( true );
    modifyLandButton->setText( tr("Modify Land") );
    modifyLandButton->setArrowType( Qt::DownArrow );

    connect( modifyLandButton, &QToolButton::toggled, this,
        [=](bool checked) {
            if( checked ) {
                modifyLandButton->setArrowType( Qt::UpArrow );
                showBasicLandControls();
            } else {
                modifyLandButton->setArrowType( Qt::DownArrow );
                hideBasicLandControls();
            }
        } );

    QWidget* tabCornerWidget = new QWidget();
    QHBoxLayout* tabCornerLayout = new QHBoxLayout( tabCornerWidget );
    tabCornerLayout->setMargin( 0 );
    tabCornerLayout->addWidget( modifyLandButton );
    tabCornerLayout->addSpacing( 10 );
    mZoneViewerTabWidget->setCornerWidget( tabCornerWidget );

    // Set up all CardViewerWidgets in tabs for each zone.
    for( auto cardZone : cardZones )
    {
        QWidget* tabWidget = new QWidget();
        QGridLayout *tabLayout = new QGridLayout( tabWidget );

        // Add the tab.  Title will be updated after internal maps are set up.
        int tabIndex = mZoneViewerTabWidget->addTab( tabWidget, QString() );

        // One-off: grab the default tab text color here.
        if( tabIndex == 0 ) mDefaultTabTextColor = mZoneViewerTabWidget->tabBar()->tabTextColor( 0 );

        // Widget to hold the cards.  Make the background white to hide
        // the white corners on JPG cards returned by gatherer. 
        CardViewerWidget *cardViewerWidget;
        if( cardZone == CARD_ZONE_GRID_DRAFT )
        {
            cardViewerWidget = new GridCardViewerWidget( mImageLoaderFactory, mLoggingConfig.createChildConfig("gridcardviewerwidget"), this );
        }
        else
        {
            cardViewerWidget = new CardViewerWidget( mImageLoaderFactory, mLoggingConfig.createChildConfig("cardviewerwidget"), this );
        }
        cardViewerWidget->setContextMenuPolicy( Qt::CustomContextMenu );
        cardViewerWidget->setSortCriteria( { CARD_SORT_CRITERION_NAME } );
        cardViewerWidget->setCardsPreselectable( cardZone == CARD_ZONE_BOOSTER_DRAFT );  // only preselectable in draft
        connect(cardViewerWidget, SIGNAL(cardPreselectRequested(CardWidget*,const CardDataSharedPtr&)),
                this, SLOT(handleCardPreselectRequested(CardWidget*,const CardDataSharedPtr&)));
        connect(cardViewerWidget, SIGNAL(cardSelectRequested(const CardDataSharedPtr&)),
                this, SLOT(handleCardSelectRequested(const CardDataSharedPtr&)));
        connect(cardViewerWidget, SIGNAL(cardIndicesSelectRequested(const QList<int>&)),
                this, SLOT(handleCardIndicesSelectRequested(const QList<int>&)));
        connect(cardViewerWidget, SIGNAL(cardMoveRequested(const CardDataSharedPtr&)),
                this, SLOT(handleCardMoveRequested(const CardDataSharedPtr&)));
        connect(cardViewerWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
                this, SLOT(handleViewerContextMenu(const QPoint&)));
        connect(cardViewerWidget, SIGNAL(cardContextMenuRequested(CardWidget*,const CardDataSharedPtr&,const QPoint&)),
                this, SLOT(handleCardContextMenu(CardWidget*,const CardDataSharedPtr&,const QPoint&)));

        mCardViewerWidgetMap[cardZone] = cardViewerWidget;

        CommanderPane_CardScrollArea *cardScrollArea = new CommanderPane_CardScrollArea();
        // Important or else the widget won't expand to the size of the
        // QScrollArea, resulting in the FlowLayout showing up as a vertical
        // list of items rather than a flow layout.
        cardScrollArea->setWidgetResizable(true);
        cardScrollArea->setWidget( cardViewerWidget );
        cardScrollArea->setMinimumWidth( 300 );
        cardScrollArea->setMinimumHeight( 200 );

        int tabLayoutRow = 0;

        // Create basic land widgets and handling for main and sideboard zones.
        if( (cardZone == CARD_ZONE_MAIN) || (cardZone == CARD_ZONE_SIDEBOARD) )
        {
            BasicLandControlWidget *basicLandControlWidget = new BasicLandControlWidget();

            tabLayout->addWidget( basicLandControlWidget, tabLayoutRow++, 0, Qt::AlignRight );
            basicLandControlWidget->hide();

            // Connect the basic land qtys signal to our cardviewer widget.
            connect(basicLandControlWidget, SIGNAL(basicLandQuantitiesUpdate(const BasicLandQuantities&)),
                    cardViewerWidget, SLOT(setBasicLandQuantities(const BasicLandQuantities&)));

            // Forward the basic land qtys signal from from the widget to
            // this class's signal, adding our zone.
            connect( basicLandControlWidget, &BasicLandControlWidget::basicLandQuantitiesUpdate,
                     [this,cardZone] (const BasicLandQuantities& qtys)
                     {
                         mLogger->debug( "forwarding basic land qtys signal, zone={}", cardZone );
                         emit basicLandQuantitiesUpdate( cardZone, qtys );
                     });

            mBasicLandControlWidgetMap[cardZone] = basicLandControlWidget;
        }

        if( cardZone == CARD_ZONE_BOOSTER_DRAFT )
        {
            mBoosterDraftQueuedPacksCapsule = new CapsuleIndicator( CapsuleIndicator::STYLE_NORMAL, CAPSULE_HEIGHT, cardScrollArea );
            mBoosterDraftQueuedPacksCapsule->setLabelText( tr("packs") );
            mBoosterDraftQueuedPacksCapsule->setToolTip( tr("Packs queued for selection") );

            mBoosterDraftTimeRemainingCapsule = new CapsuleIndicator( CapsuleIndicator::STYLE_NORMAL, CAPSULE_HEIGHT, cardScrollArea );
            mBoosterDraftTimeRemainingCapsule->setLabelText( tr("secs") );
            mBoosterDraftTimeRemainingCapsule->setToolTip( tr("Time remaining to select a card") );

            // Capsule widths will vary based on label text.  Make both capsules the same size.
            QSize sz = mBoosterDraftQueuedPacksCapsule->sizeHint();
            sz.expandedTo( mBoosterDraftTimeRemainingCapsule->sizeHint() );
            mBoosterDraftQueuedPacksCapsule->setFixedSize( sz );
            mBoosterDraftTimeRemainingCapsule->setFixedSize( sz );

            connect( cardScrollArea, &CommanderPane_CardScrollArea::viewportRectUpdated, this, [=]( const QRect& rect ) {
                    const QPoint br = rect.bottomRight();
                    mBoosterDraftTimeRemainingCapsule->move( br.x() - (mBoosterDraftTimeRemainingCapsule->width() + CAPSULE_SPACING),
                                                             br.y() - (mBoosterDraftTimeRemainingCapsule->height() + CAPSULE_SPACING) );
                    mBoosterDraftQueuedPacksCapsule->move( br.x() - (mBoosterDraftTimeRemainingCapsule->width() + CAPSULE_SPACING +
                                                                     mBoosterDraftQueuedPacksCapsule->width() + CAPSULE_SPACING),
                                                           br.y() - (mBoosterDraftQueuedPacksCapsule->height() + CAPSULE_SPACING) );
                } );

            // For the draft tab, add a footer to the cardwidget to create space for the capsules.
            cardViewerWidget->setFooterSpacing( CAPSULE_HEIGHT + CAPSULE_SPACING * 2 );
        }

        if( cardZone == CARD_ZONE_GRID_DRAFT )
        {
            mGridDraftTimeRemainingCapsule = new CapsuleIndicator( CapsuleIndicator::STYLE_NORMAL, CAPSULE_HEIGHT, cardScrollArea );
            mGridDraftTimeRemainingCapsule->setLabelText( tr("secs") );
            mGridDraftTimeRemainingCapsule->setToolTip( tr("Time remaining to select a card") );

            mGridDraftWaitingLabel = new QLabel( tr("WAITING FOR OPPONENT"), cardScrollArea );
            mGridDraftWaitingLabel->setStyleSheet( "border: 2px dashed darkgray;"
                                                   "border-radius: 12px;"
                                                   "background-color: lightgray;"
                                                   "padding: 5px;" );
            connect( cardScrollArea, &CommanderPane_CardScrollArea::viewportRectUpdated, this, [=]( const QRect& rect ) {
                    const QPoint br = rect.bottomRight();
                    mGridDraftTimeRemainingCapsule->move( br.x() - (mBoosterDraftTimeRemainingCapsule->width() + CAPSULE_SPACING),
                                                          br.y() - (mBoosterDraftTimeRemainingCapsule->height() + CAPSULE_SPACING) );
                    mGridDraftWaitingLabel->move( br.x() - (mGridDraftWaitingLabel->width() + CAPSULE_SPACING),
                                                          br.y() - (mGridDraftWaitingLabel->height() + CAPSULE_SPACING) );
                } );

            // For the draft tab, add a footer to the cardwidget to create space for the capsules.
            cardViewerWidget->setFooterSpacing( CAPSULE_HEIGHT + CAPSULE_SPACING * 2 );
        }

        tabLayout->addWidget( cardScrollArea, tabLayoutRow++, 0 );

        mVisibleCardZoneList.append( cardZone );

        updateTabSettings( cardZone );
    }

    updateDraftCapsulesLookAndFeel();
    updateDraftCapsulesVisibility();

    outerLayout->addWidget( mZoneViewerTabWidget );

    QHBoxLayout* controlLayout = new QHBoxLayout();

    // A little space to the left, then add a stretch to center the layout.
    controlLayout->addSpacing( 10 );
    controlLayout->addStretch( 1 );

    // Add a zoom combobox to the control area.
    QLabel* zoomLabel = new QLabel( tr("Zoom:") );
    controlLayout->addWidget( zoomLabel );
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
    QLabel* catLabel = new QLabel( tr("Categorize:") );
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
    QLabel* sortLabel = new QLabel( tr("Sort:") );
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

    // Add a stretch to center, then ensure some space to the right of the layout.
    controlLayout->addStretch( 1 );
    controlLayout->addSpacing( 10 );

    outerLayout->addLayout( controlLayout );

    connect( mZoneViewerTabWidget, &QTabWidget::currentChanged,
             [=] (int index)
             {
                 mCurrentCardZone = mVisibleCardZoneList[index];
                 mLogger->debug( "current zone changed to {}", mCurrentCardZone );

                 if( (mCurrentCardZone == CARD_ZONE_MAIN) || (mCurrentCardZone == CARD_ZONE_SIDEBOARD) )
                 {
                     modifyLandButton->show();
                 } else {
                     modifyLandButton->hide();
                 }

                 if( mCurrentCardZone == CARD_ZONE_GRID_DRAFT )
                 {
                     catLabel->hide();
                     catComboBox->hide();
                     sortLabel->hide();
                     sortComboBox->hide();
                 }
                 else
                 {
                     catLabel->show();
                     catComboBox->show();
                     sortLabel->show();
                     sortComboBox->show();
                 }

                 evaluateHiddenTabs();
             });

    // Set active tab and current card zone to the first tab.
    mZoneViewerTabWidget->setCurrentIndex( 0 );
    mCurrentCardZone = mVisibleCardZoneList.value( 0, CARD_ZONE_MAIN );
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
    // Make sure this zone is managed by this CommanderPane.
    if( !mCardZones.contains( cardZone ) ) return false;

    // If the zone was hidden, allow this call to override and show it.
    if( !mVisibleCardZoneList.contains( cardZone ) )
    {
        showHiddenTab( cardZone );
    }

    // Switch to the zone.
    int tabIndex = mVisibleCardZoneList.indexOf( cardZone );
    if( tabIndex >= 0 )
    {
        mZoneViewerTabWidget->setCurrentIndex( tabIndex );
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
CommanderPane::setDraftActive( bool active )
{
    mDraftActive = active;
    updateDraftCapsulesVisibility();
}


void
CommanderPane::setDraftQueuedPacks( int packs )
{
    mDraftQueuedPacks = packs;
    if( mBoosterDraftQueuedPacksCapsule )
    {
        mBoosterDraftQueuedPacksCapsule->setValueText( (packs >= 0) ? QString::number( packs )
                                                                    : QString() );
    }
    updateDraftCapsulesVisibility();
}


void
CommanderPane::setDraftTimeRemaining( int time )
{
    mDraftTimeRemaining = time;

    for( auto iter = mCardViewerWidgetMap.begin(); iter != mCardViewerWidgetMap.end(); ++iter )
    {
        CardViewerWidget* cardViewerWidget = iter.value();
        cardViewerWidget->setWaitingForTurn( (time < 0) );
    }

    if( mBoosterDraftTimeRemainingCapsule )
    {
        mBoosterDraftTimeRemainingCapsule->setValueText( (time >= 0) ? QString::number( time )
                                                                     : QString() );
    }
    if( mGridDraftTimeRemainingCapsule )
    {
        mGridDraftTimeRemainingCapsule->setValueText( (time >= 0) ? QString::number( time )
                                                                  : QString() );
    }
    updateDraftCapsulesVisibility();
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
CommanderPane::setSelectedCards( const CardZoneType cardZone, const QMap<CardDataSharedPtr,SelectedCardData>& selectedCards )
{
    auto iter = mCardViewerWidgetMap.find( cardZone );
    if( iter != mCardViewerWidgetMap.end() )
    {
        CardViewerWidget *cardViewerWidget = iter.value();
        cardViewerWidget->setSelectedCards( selectedCards );
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
CommanderPane::setDraftAlert( bool alert )
{
    mLogger->debug( "draft alert status changed: {}", alert );
    mDraftAlert = alert;

    // Set alert status on draft widgets.
    for( auto iter = mCardViewerWidgetMap.begin(); iter != mCardViewerWidgetMap.end(); ++iter )
    {
        if( (iter.key() == CARD_ZONE_BOOSTER_DRAFT) ||
            (iter.key() == CARD_ZONE_GRID_DRAFT) )
        {
            CardViewerWidget *cardViewerWidget = iter.value();
            cardViewerWidget->setAlert( alert );
        }
    }

    updateDraftCapsulesLookAndFeel();
    updateTabSettings( CARD_ZONE_BOOSTER_DRAFT );
    updateTabSettings( CARD_ZONE_GRID_DRAFT );
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
CommanderPane::handleCardPreselectRequested( CardWidget* cardWidget, const CardDataSharedPtr& cardData )
{
    mLogger->debug( "card preselect requested: {}", cardData->getName() );

    if( mCurrentCardZone != CARD_ZONE_BOOSTER_DRAFT )
    {
        mLogger->debug( "ignoring card preselect request from non-booster-draft zone" );
        return;
    }

    // Set the preselected card within the CardViewerWidget.
    CardViewerWidget* cardViewerWidget = mCardViewerWidgetMap[mCurrentCardZone];
    cardViewerWidget->setPreselected( cardWidget );

    emit cardPreselected( cardData ); 
}


void
CommanderPane::handleCardSelectRequested( const CardDataSharedPtr& cardData )
{
    if( mCurrentCardZone == CARD_ZONE_GRID_DRAFT )
    {
        mLogger->debug( "ignoring card select request from grid draft zone" );
        return;
    }

    mLogger->debug( "card select requested: {}", cardData->getName() );
    emit cardSelected( mCurrentCardZone, cardData );
}


void
CommanderPane::handleCardMoveRequested( const CardDataSharedPtr& cardData )
{
    // Ignore move requests from draft zone.
    if( (mCurrentCardZone == CARD_ZONE_BOOSTER_DRAFT) || (mCurrentCardZone == CARD_ZONE_GRID_DRAFT) )
    {
        mLogger->debug( "ignoring card move request from draft zone" );
        return;
    }

    mLogger->debug( "card move requested: {}", cardData->getName() );

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
CommanderPane::handleCardIndicesSelectRequested( const QList<int>& indices )
{
    // Ignore index-based requests outside of grid draft zone.
    if( mCurrentCardZone != CARD_ZONE_GRID_DRAFT )
    {
        mLogger->debug( "ignoring card indices selection request from non grid-draft zone" );
        return;
    }

    mLogger->debug( "card indices select requested: {}", stringify( indices ) );
    emit cardIndicesSelected( mCurrentCardZone, indices );
}


void
CommanderPane::handleCardContextMenu( CardWidget* cardWidget, const CardDataSharedPtr& cardData, const QPoint& pos )
{
    mLogger->debug( "card context menu: {}", cardData->getName() );

    if( mCurrentCardZone == CARD_ZONE_GRID_DRAFT )
    {
        mLogger->debug( "ignoring card context request from grid draft zone" );
        return;
    }

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
    QAction *draftPreselectAction = 0;
    QAction *removeLandAction = 0;

    BasicLandType basic;
    if( mCurrentCardZone == CARD_ZONE_BOOSTER_DRAFT )
    {
        mainAction = menu.addAction( "Draft to Main" );
        sbAction = menu.addAction( "Draft to Sideboard" );
        junkAction = menu.addAction( "Draft to Junk" );
        menu.addSeparator();
        draftPreselectAction = menu.addAction( "Preselect" );
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
    else if( result == draftPreselectAction )
    {
        cardViewerWidget->setPreselected( cardWidget );
        emit cardPreselected( cardData ); 
    }
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
    if( (mCurrentCardZone == CARD_ZONE_BOOSTER_DRAFT) || (mCurrentCardZone == CARD_ZONE_GRID_DRAFT) )
    {
        mLogger->debug( "ignoring viewer context menu request from draft zone" );
        return;
    }

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
            if( tabIndex != mZoneViewerTabWidget->currentIndex() )
            {
                mVisibleCardZoneList.removeAt( tabIndex );
                mHiddenCardZoneWidgetMap.insert( cardZone, mZoneViewerTabWidget->widget( tabIndex ) );
                mZoneViewerTabWidget->removeTab( tabIndex );
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
        mZoneViewerTabWidget->insertTab( insertIndex, widget, QString() );
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
        mZoneViewerTabWidget->setTabText( tabIndex, text );

        QTabBar* tabBar = mZoneViewerTabWidget->tabBar();
        switch( cardZone )
        {
            case CARD_ZONE_BOOSTER_DRAFT:
            case CARD_ZONE_GRID_DRAFT:
                tabBar->setTabTextColor( tabIndex, mDraftAlert ? QColor(Qt::red) : mDefaultTabTextColor );
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


void
CommanderPane::showBasicLandControls()
{
    for( auto iter = mBasicLandControlWidgetMap.constBegin(); iter != mBasicLandControlWidgetMap.constEnd(); ++iter )
    {
        BasicLandControlWidget* basicLandControlWidget = iter.value();

        basicLandControlWidget->show();

        // If this basic land control is visible (on the current tab), animate it.
        if( basicLandControlWidget->isVisible() )
        {
            int desiredHeight = basicLandControlWidget->height();
            QPropertyAnimation* animation = new QPropertyAnimation(basicLandControlWidget, "maximumHeight");
            animation->setDuration( 200 );
            animation->setStartValue( 0 );
            animation->setEndValue( desiredHeight );
            animation->start();

            // Reset maximum height when animation finishes.
            connect( animation, &QPropertyAnimation::finished, this,
                    [=](void) { basicLandControlWidget->setMaximumHeight( QWIDGETSIZE_MAX ); } );
        }
    }
}


void
CommanderPane::hideBasicLandControls()
{
    for( auto iter = mBasicLandControlWidgetMap.constBegin(); iter != mBasicLandControlWidgetMap.constEnd(); ++iter )
    {
        BasicLandControlWidget* basicLandControlWidget = iter.value();

        // If this basic land control is visible (on the current tab), animate it.
        if( basicLandControlWidget->isVisible() )
        {
            QPropertyAnimation* animation = new QPropertyAnimation(basicLandControlWidget, "maximumHeight");
            animation->setDuration( 200 );
            animation->setStartValue( basicLandControlWidget->height() );
            animation->setEndValue( 0 );
            animation->start();

            // Hide and reset maximum height when animation finishes
            connect( animation, &QPropertyAnimation::finished, this,
                    [=](void) {
                        basicLandControlWidget->hide();
                        basicLandControlWidget->setMaximumHeight( QWIDGETSIZE_MAX );
                    } );
        }
        else
        {
            basicLandControlWidget->hide();
        }
    }
}


void
CommanderPane::updateDraftCapsulesVisibility()
{
    if( mBoosterDraftQueuedPacksCapsule && mBoosterDraftTimeRemainingCapsule )
    {
        if( mDraftActive && (mDraftQueuedPacks > 0) && (mDraftTimeRemaining > 0) )
        {
            mBoosterDraftQueuedPacksCapsule->show();
            mBoosterDraftTimeRemainingCapsule->show();
        }
        else
        {
            mBoosterDraftQueuedPacksCapsule->hide();
            mBoosterDraftTimeRemainingCapsule->hide();
        }
    }

    if( mGridDraftTimeRemainingCapsule && mGridDraftWaitingLabel )
    {
        mGridDraftTimeRemainingCapsule->setVisible( mDraftActive && (mDraftTimeRemaining > 0) );
        mGridDraftWaitingLabel->setVisible( mDraftActive && (mDraftTimeRemaining < 0) );
    }
}


void
CommanderPane::updateDraftCapsulesLookAndFeel()
{
    QColor stackForeground( mDraftAlert ? 0xc0c0c0 : 0x404040 );
    QColor stackBackground( mDraftAlert ? 0xff2828 : 0xd0d0d0 );
    QColor timeForeground( mDraftAlert ? 0xffffff : 0x404040 );
    QColor timeBackground( mDraftAlert ? 0xff2828 : 0xaed581 );

    if( mBoosterDraftQueuedPacksCapsule )
    {
        mBoosterDraftQueuedPacksCapsule->setSvgIconPath( mDraftAlert ? ":/stack-lightgray.svg" : ":/stack-darkgray.svg" );
        mBoosterDraftQueuedPacksCapsule->setBorderColor( stackForeground );
        mBoosterDraftQueuedPacksCapsule->setBorderBold( mDraftAlert ? true : false );
        mBoosterDraftQueuedPacksCapsule->setBackgroundColor( stackBackground );
        mBoosterDraftQueuedPacksCapsule->setTextColor( stackForeground );
    }

    if( mBoosterDraftTimeRemainingCapsule )
    {
        mBoosterDraftTimeRemainingCapsule->setSvgIconPath( mDraftAlert ? ":/alarm-clock-white.svg" : ":/alarm-clock-darkgray.svg" );
        mBoosterDraftTimeRemainingCapsule->setBorderColor( timeForeground );
        mBoosterDraftTimeRemainingCapsule->setBorderBold( mDraftAlert ? true : false );
        mBoosterDraftTimeRemainingCapsule->setBackgroundColor( timeBackground );
        mBoosterDraftTimeRemainingCapsule->setTextColor( timeForeground );
    }

    if( mGridDraftTimeRemainingCapsule )
    {
        mGridDraftTimeRemainingCapsule->setSvgIconPath( mDraftAlert ? ":/alarm-clock-white.svg" : ":/alarm-clock-darkgray.svg" );
        mGridDraftTimeRemainingCapsule->setBorderColor( timeForeground );
        mGridDraftTimeRemainingCapsule->setBorderBold( mDraftAlert ? true : false );
        mGridDraftTimeRemainingCapsule->setBackgroundColor( timeBackground );
        mGridDraftTimeRemainingCapsule->setTextColor( timeForeground );
    }
}


QSize
CommanderPane_CardScrollArea::sizeHint() const
{
    return QSize( 750, 600 ); 
}


bool
CommanderPane_CardScrollArea::viewportEvent( QEvent* event )
{
    if( (event->type() == QEvent::Show) ||
        (event->type() == QEvent::Move) ||
        (event->type() == QEvent::Resize) )
    {
        emit viewportRectUpdated( viewport()->rect() );
    }
    return QScrollArea::viewportEvent( event );
}
