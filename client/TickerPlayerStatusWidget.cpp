#include "TickerPlayerStatusWidget.h"

#include <QBoxLayout>
#include <QLabel>

#include "qtutils_widget.h"
#include "SizedSvgWidget.h"
#include "PlayerStatusWidget.h"


static const QString RESOURCE_SVG_ARROW_LEFT( ":/arrow-left.svg" );
static const QString RESOURCE_SVG_ARROW_RIGHT( ":/arrow-right.svg" );
static const QString RESOURCE_SVG_ARROW_CW_LEFT( ":/arrow-cw-left.svg" );
static const QString RESOURCE_SVG_ARROW_CCW_LEFT( ":/arrow-ccw-left.svg" );
static const QString RESOURCE_SVG_ARROW_CW_RIGHT( ":/arrow-cw-right.svg" );
static const QString RESOURCE_SVG_ARROW_CCW_RIGHT( ":/arrow-ccw-right.svg" );


void
TickerPlayerStatusWidget::update( const RoomStateAccumulator& roomState )
{
    // Build/rebuild the widget if the chair count has changed.
    if( !mBuilt || (mChairCount != roomState.getChairCount()) || (mIsPublicDraftType != roomState.isPublicDraftType()) )
    {
        mChairCount = roomState.getChairCount();
        mIsPublicDraftType = roomState.isPublicDraftType();
        build( roomState );
        mBuilt = true;
    }

    for( int i = 0; i < roomState.getChairCount(); ++i )
    {
        PlayerStatusWidget *widget = mPlayerStatusWidgetList[ i ];
        widget->setPackQueueSize( roomState.hasPlayerPackQueueSize( i ) ? roomState.getPlayerPackQueueSize( i ) : -1 );

        const int timeRemaining = roomState.hasPlayerTimeRemaining( i ) ? roomState.getPlayerTimeRemaining( i ) : -1;
        widget->setTimeRemaining( timeRemaining );

        if( roomState.hasPlayerState( i ) )
        {
            widget->setPlayerActive( roomState.getPlayerState( i ) == PLAYER_STATE_ACTIVE );
        }
        if( mIsPublicDraftType )
        {
            const bool playerDrafting = roomState.isPublicDraftPlayerActive( i );
            widget->setPlayerDrafting( playerDrafting );

            // Only show time remaining for active player with actual time
            // on the clock (i.e. not unlimited)
            widget->setTimeRemainingVisible( (timeRemaining > 0) && playerDrafting );
        }
        if( roomState.hasPlayerName( i ) )
        {
            widget->setName( QString::fromStdString( roomState.getPlayerName( i ) ) );
            adjustSize();
        }
    }

    if( mPassDirection != roomState.getPassDirection() )
    {
        mPassDirection = roomState.getPassDirection();

        if( mPassDirection == PASS_DIRECTION_CW )
        {
            mPassDirLeftWidget->hide();
            mPassDirRightWidget->load( RESOURCE_SVG_ARROW_CCW_LEFT );
            mPassDirRightWidget->show();
            for( auto w : mPassDirWidgetList )
            {
                w->load( RESOURCE_SVG_ARROW_RIGHT );
            }
        }
        else if( mPassDirection == PASS_DIRECTION_CCW )
        {
            mPassDirRightWidget->hide();
            mPassDirLeftWidget->load( RESOURCE_SVG_ARROW_CW_RIGHT );
            mPassDirLeftWidget->show();
            for( auto w : mPassDirWidgetList )
            {
                w->load( RESOURCE_SVG_ARROW_LEFT );
            }
        }
        else
        {
            mPassDirLeftWidget->hide();
            mPassDirRightWidget->hide();
            for( auto w : mPassDirWidgetList )
            {
                w->hide();
            }
        }
    }
}


void
TickerPlayerStatusWidget::build( const RoomStateAccumulator& roomState )
{
    qtutils::clearLayout( mLayout );
    mPassDirWidgetList.clear();
    mPlayerStatusWidgetList.clear();

    const bool queueSizesVisible = !mIsPublicDraftType;

    const int dim = (mTickerHeight * 3) / 4;  // arrows are 3/4 ticker height
    const QSize size( dim, dim );
    const int spacing = 12;

    mPassDirLeftWidget = new SizedSvgWidget( size );
    mPassDirLeftWidget->setContentsMargins( 0, 0, 0, 0 );
    mLayout->addWidget( mPassDirLeftWidget );
    mLayout->addSpacing( spacing );

    for( int i = 0; i < roomState.getChairCount(); ++i )
    {
        SizedSvgWidget* passDirWidget = nullptr;
        if( i > 0 )
        {
            // Place arrow widget
            passDirWidget = new SizedSvgWidget( size );
            mPassDirWidgetList.push_back( passDirWidget );
            mLayout->addWidget( passDirWidget );
            mLayout->addSpacing( spacing );
        }

        // Place player widget.
        PlayerStatusWidget *playerStatusWidget = new PlayerStatusWidget( mTickerHeight - 2, this );
        playerStatusWidget->setPackQueueSizeVisible( queueSizesVisible );
        mPlayerStatusWidgetList.push_back( playerStatusWidget );
        mLayout->addWidget( playerStatusWidget );
        mLayout->addSpacing( spacing );
    }

    mLayout->addSpacing( spacing );
    mPassDirRightWidget = new SizedSvgWidget( size );
    mPassDirRightWidget->setContentsMargins( 0, 0, 0, 0 );
    mLayout->addWidget( mPassDirRightWidget );


    // It's possible (likely?) that this widget is showing.  If so, Qt
    // doesn't automatically show new widgets when they're added to a layout.
    // Need to manually make them visible.
    if( !isHidden() )
    {
        mPassDirLeftWidget->setVisible(true);
        mPassDirRightWidget->setVisible(true);
        for( auto w : mPassDirWidgetList ) w->setVisible(true);
        for( auto w : mPlayerStatusWidgetList ) w->setVisible(true);
    }

    adjustSize();
}


