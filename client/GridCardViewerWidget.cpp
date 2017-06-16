#include "GridCardViewerWidget.h"

#include <QGridLayout>
#include <QStyleOption>

#include "CardWidget.h"
#include "qtutils_core.h"
#include "qtutils_widget.h"

static QSet<int> getRowIndices( int row )
{
    return { (row*3), (row*3)+1, (row*3)+2 };
}

static QSet<int> getColIndices( int col )
{
    return { col, col+3, col+6 };
}


GridCardViewerWidget::GridCardViewerWidget( ImageLoaderFactory*    imageLoaderFactory,
                                            const Logging::Config& loggingConfig,
                                            QWidget*               parent )
  : CardViewerWidget( imageLoaderFactory, loggingConfig, parent ),
    mGridCardsLayout( nullptr ),
    mWaitingForTurn( false )
{
    // Create column buttons.
    for( int col = 0; col < 3; ++col )
    {
        for( bool top : { true, false } )
        {
            GridToolButton* button = new GridToolButton();
            button->setArrowType( top ? Qt::DownArrow : Qt::UpArrow );
            button->setText( "Select Column" );
            button->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
            button->setToolTip( tr("Select column %0").arg(col+1) );

            // Keep the layout from jerking around when buttons are hidden.
            QSizePolicy sizePolicy = button->sizePolicy();
            sizePolicy.setRetainSizeWhenHidden( true );
            button->setSizePolicy( sizePolicy );

            connect( button, &GridToolButton::mouseEntered, [this,col]() {
                for( auto i : getColSelectableIndices( col ) )
                {
                    if( i < mCardWidgetsList.size() ) mCardWidgetsList[i]->setHighlighted( true );
                }
            } );
            connect( button, &GridToolButton::mouseExited, [this,col]() {
                for( auto i : getColIndices( col ) )
                {
                    if( i < mCardWidgetsList.size() ) mCardWidgetsList[i]->setHighlighted( false );
                }
            } );
            connect( button, &QToolButton::clicked, [this,col]() {
                QList<int> indices = QList<int>::fromSet( getColSelectableIndices( col ) );
                emit cardIndicesSelectRequested( indices );
            } );

            if( top )
            {
                mTopColButtons[col] = button;
            }
            else
            {
                mBottomColButtons[col] = button;
            }
        }
    }

    // Create row buttons.
    for( int row = 0; row < 3; ++row )
    {
        for( bool left : { true, false } )
        {
            GridToolButton* button = new GridToolButton();
            button->setArrowType( left ? Qt::RightArrow : Qt::LeftArrow );
            button->setText( "Select\nRow" );
            button->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
            button->setToolTip( tr("Select row %0").arg(row+1) );

            connect( button, &GridToolButton::mouseEntered, [this,row]() {
                for( auto i : getRowSelectableIndices( row ) )
                {
                    if( i < mCardWidgetsList.size() ) mCardWidgetsList[i]->setHighlighted( true );
                }
            } );
            connect( button, &GridToolButton::mouseExited, [this,row]() {
                for( auto i : getRowIndices( row ) )
                {
                    if( i < mCardWidgetsList.size() ) mCardWidgetsList[i]->setHighlighted( false );
                }
            } );
            connect( button, &QToolButton::clicked, [this,row]() {
                QList<int> indices = QList<int>::fromSet( getRowSelectableIndices( row ) );
                emit cardIndicesSelectRequested( indices );
            } );

            if( left )
            {
                mLeftRowButtons[row] = button;
            }
            else
            {
                mRightRowButtons[row] = button;
            }
        }
    }
}
 

GridCardViewerWidget::~GridCardViewerWidget()
{
}


// Set top-level layout to show cards.
// Entry conditions:
// - mCardWidgetsList contains CardWidgets from previous layout (any
//   widgets not reused should be cleaned up)
// Exit requirements:
// - mCardsList must be set to the list of cards
// - mCardWidgetsList must contain all CardWidgets in the layout
void
GridCardViewerWidget::setCards( const QList<CardDataSharedPtr>& cards )
{
    // No need to lay everything out again if nothing changed.
    if( mCardsList == cards ) return;

    mCardsList = cards;

    //
    // Performance gets bad if creating all new widgets from scratch,
    // especially when images are scaled.  Instead need to extract the
    // widgets from the layout, then only create new stuff as needed.
    //
    
    // Liberate the widgets we want to retain before clearing out the layout.
    for( auto w : mCardWidgetsList )
    {
        w->setParent( 0 );
    }
    for( int col = 0; col < 3; ++col )
    {
        mTopColButtons[col]->setParent( 0 );
        mBottomColButtons[col]->setParent( 0 );
    }
    for( int row = 0; row < 3; ++row )
    {
        mLeftRowButtons[row]->setParent( 0 );
        mRightRowButtons[row]->setParent( 0 );
    }

    // Our grid layout will be actually deleted below by clearLayout, but
    // need to null out our pointer.
    mGridCardsLayout = nullptr;

    // Remove any previously created widget from the alertable set.
    clearAlertableSubwidgets();

    // Clear out the top-level layout - we're rebuilding it here.
    qtutils::clearLayout( mLayout );

    // Function to clean up cardwidgets in the tracking list.
    auto cleanupCardWidgetsListFn = [this]() {
            for( auto w : mCardWidgetsList )
            {
                w->deleteLater();
            }
            mCardWidgetsList.clear();
        };

    // At this point everything is cleared.  Finish cleanup and exit if
    // there aren't exactly 9 cards in the list.
    if( mCardsList.size() != 9 )
    {
        // Zero is normal, but non-zero is not.
        if( mCardsList.size() > 0 )
        {
            mLogger->warn( "card list size ({}) != 9, cannot perform grid layout!", cards.size() );
        }

        // Clean up any cardwidgets remaining in the original list.
        cleanupCardWidgetsListFn();

        return;
    }

    // This will contain all widgets once they're created/reused.
    QList<CardWidget*> newCardWidgetsList;

    // This local function picks a matching already-created CardWidget from
    // our tracking list to speed up the overall "setCards" operation which happens
    // on every sort, categorize, etc.
    auto takeWidgetFromCardWidgetsList =
        [this]( const CardDataSharedPtr& cardDataSharedPtr ) -> CardWidget* {
            for( int i = 0; i < mCardWidgetsList.count(); ++i ) {
                CardWidget* w = mCardWidgetsList[i];
                if( w->getCardData() == cardDataSharedPtr ) {
                    mLogger->debug( "reusing CardWidget for name={} muid={}",
                            cardDataSharedPtr->getName(), cardDataSharedPtr->getMultiverseId() );
                    mCardWidgetsList.takeAt( i );
                    w->resetTraits();
                    updateCardWidgetSelectedAppearance( w );
                    return w;
                }
            }
            return nullptr;
        };

    // This local function creates and connects a CardWidget.
    auto createCardWidget =
        [this]( const CardDataSharedPtr& cardDataSharedPtr ) -> CardWidget* {
            mLogger->debug( "creating CardWidget name={} muid={}", cardDataSharedPtr->getName(), cardDataSharedPtr->getMultiverseId() );
            QString card = QString::fromStdString( cardDataSharedPtr->getName() );

            CardWidget* cardWidget = new CardWidget( cardDataSharedPtr,
                                                     mImageLoaderFactory,
                                                     QSize( 223, 310 ),
                                                     mLoggingConfig.createChildConfig( "cardwidget" ) );
            cardWidget->setZoomFactor( mZoomFactor );
            updateCardWidgetSelectedAppearance( cardWidget );
            cardWidget->loadImage();
            connect(cardWidget, SIGNAL(preselectRequested()),
                    this, SLOT(handleCardPreselectRequested()));
            connect(cardWidget, SIGNAL(selectRequested()),
                    this, SLOT(handleCardSelectRequested()));
            connect(cardWidget, SIGNAL(moveRequested()),
                    this, SLOT(handleCardMoveRequested()));
            cardWidget->setContextMenuPolicy( Qt::CustomContextMenu );
            connect(cardWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
                    this, SLOT(handleCardContextMenu(const QPoint&)));
            return cardWidget;
        };

    //
    // Lay out the cards and buttons in a grid.
    //

    // This widget shouldn't be needed but Qt gets confused when the
    // layout is added directly.  Having a widget own it seems to help.
    QWidget* gridCardsLayoutWidget = new QWidget();

    // The widget needs to turn red when the alert is active.
    addAlertableSubwidget( gridCardsLayoutWidget );

    mGridCardsLayout = new QGridLayout( gridCardsLayoutWidget );

    // Center the gridlayout using a QHBoxLayout and add to parent layout
    // before adding widgets as mentioned in Qt docs.
    QHBoxLayout* alignmentLayout = new QHBoxLayout();
    alignmentLayout->addStretch();
    alignmentLayout->addWidget( gridCardsLayoutWidget );
    alignmentLayout->addStretch();
    mLayout->addLayout( alignmentLayout );

    // Lay out column buttons.
    for( int col = 0; col < 3; ++col )
    {
        mGridCardsLayout->addWidget( mTopColButtons[col], 0, 1 + col, Qt::AlignCenter );
        mGridCardsLayout->addWidget( mBottomColButtons[col], 4, 1 + col, Qt::AlignCenter );
    }

    // Lay out row buttons.
    for( int row = 0; row < 3; ++row )
    {
        mGridCardsLayout->addWidget( mLeftRowButtons[row],  1 + row, 0, Qt::AlignCenter );
        mGridCardsLayout->addWidget( mRightRowButtons[row], 1 + row, 4, Qt::AlignCenter );
    }

    updateButtonVisibility();

    // Create card widgets and add to layout.
    for( int i = 0; i < 9; ++i )
    {
        auto cardDataSharedPtr = mCardsList[i];

        // Look for an existing card widget that matches our card data,
        // and extract it from the list if found.
        CardWidget* cardWidget = nullptr;
        cardWidget = takeWidgetFromCardWidgetsList( cardDataSharedPtr );

        if( !cardWidget )
        {
            // The widget doesn't exist.  Create it.
            cardWidget = createCardWidget( cardDataSharedPtr );
        }
        mGridCardsLayout->addWidget( cardWidget, 1+ i/3, 1 + i%3 );
        newCardWidgetsList.push_back( cardWidget );
    }

    if( mFooterSpacing > 0 ) mLayout->addSpacing( mFooterSpacing );

    // This keeps everything pushed nicely to the top of the main area.
    mLayout->addStretch();

    // Clean up any cardwidgets remaining in the original list.
    cleanupCardWidgetsListFn();

    // Old is new.
    mCardWidgetsList = newCardWidgetsList;
}


void
GridCardViewerWidget::setWaitingForTurn( bool waiting )
{
    if( mWaitingForTurn != waiting )
    {
        mWaitingForTurn = waiting;
        updateButtonVisibility();
    }
};


void
GridCardViewerWidget::updateCardWidgetSelectedAppearance( CardWidget* cardWidget )
{
    if( cardWidget == nullptr ) return;

    const auto iter = mSelectedCards.find( cardWidget->getCardData() );
    if( iter != mSelectedCards.end() )
    {
        cardWidget->setDimmed( true );
        cardWidget->setSelectedByOpponent( iter->isOpponent );
        cardWidget->setSelectedByPlayer( !iter->isOpponent );
    }
}


void
GridCardViewerWidget::selectedCardsUpdateHandler()
{
    for( auto w : mCardWidgetsList )
    {
        updateCardWidgetSelectedAppearance( w );
    }

    updateButtonVisibility();
}


bool GridCardViewerWidget::isIndexSelectable( int i )
{
    return (i < mCardsList.size()) && !mSelectedCards.contains( mCardsList[i] );
};


QSet<int>
GridCardViewerWidget::getRowSelectableIndices( int row )
{
    // Use QMutableSetIterator to operate on set in-place.
    QSet<int> indices = getRowIndices( row );
    QMutableSetIterator<int> iter( indices );
    while( iter.hasNext() )
    {
        int i = iter.next();
        if( !isIndexSelectable( i ) ) iter.remove();
    }
    return indices;
}


QSet<int>
GridCardViewerWidget::getColSelectableIndices( int col )
{
    // Use QMutableSetIterator to operate on set in-place.
    QSet<int> indices = getColIndices( col );
    QMutableSetIterator<int> iter( indices );
    while( iter.hasNext() )
    {
        int i = iter.next();
        if( !isIndexSelectable( i ) ) iter.remove();
    }
    return indices;
}


void
GridCardViewerWidget::updateButtonVisibility()
{
    // Set buttons disabled if there are no selectable cards or waiting for turn.
    for( int col = 0; col < 3; ++col )
    {
        const bool colSelectable = !getColSelectableIndices( col ).empty();
        const bool visible = !mWaitingForTurn && colSelectable;
        mTopColButtons[col]->setVisible( visible );
        mBottomColButtons[col]->setVisible( visible );
    }
    for( int row = 0; row < 3; ++row )
    {
        const bool rowSelectable = !getRowSelectableIndices( row ).empty();
        const bool visible = !mWaitingForTurn && rowSelectable;

        mLeftRowButtons[row]->setVisible( visible );
        mRightRowButtons[row]->setVisible( visible );
    }
}


GridToolButton::GridToolButton( QWidget* parent )
  : QToolButton( parent )
{
}


void
GridToolButton::enterEvent( QEvent* e )
{
    emit mouseEntered();
    QToolButton::enterEvent( e );
}


void
GridToolButton::leaveEvent( QEvent* e )
{
    emit mouseExited();
    QToolButton::leaveEvent( e );
}
