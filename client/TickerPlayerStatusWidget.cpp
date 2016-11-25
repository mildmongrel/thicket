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
    if( !mBuilt || mChairCount != roomState.getChairCount() )
    {
        mChairCount = roomState.getChairCount();
        build( roomState );
        mBuilt = true;
    }

    for( int i = 0; i < roomState.getChairCount(); ++i )
    {
        PlayerStatusWidget *widget = mPlayerStatusWidgetList[ i ];
        widget->setPackQueueSize( roomState.hasPlayerPackQueueSize( i ) ? roomState.getPlayerPackQueueSize( i ) : -1 );
        widget->setTimeRemaining( roomState.hasPlayerTimeRemaining( i ) ? roomState.getPlayerTimeRemaining( i ) : -1 );

        if( roomState.hasPlayerState( i ) )
        {
            widget->setPlayerActive( roomState.getPlayerState( i ) == PLAYER_STATE_ACTIVE );
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

    const int dim = (mTickerHeight * 3) / 4;  // arrows are 3/4 ticker height
    const QSize size( dim, dim );

    mPassDirLeftWidget = new SizedSvgWidget( size );
    mPassDirLeftWidget->setContentsMargins( 0, 0, 0, 0 );
    mLayout->addWidget( mPassDirLeftWidget );
    mLayout->addSpacing( 15 );

    for( int i = 0; i < roomState.getChairCount(); ++i )
    {
        SizedSvgWidget* passDirWidget = nullptr;
        if( i > 0 )
        {
            // Place arrow widget
            passDirWidget = new SizedSvgWidget( size );
            mPassDirWidgetList.push_back( passDirWidget );
            mLayout->addWidget( passDirWidget );
            mLayout->addSpacing( 15 );
        }

        // Place player widget.
        PlayerStatusWidget *playerStatusWidget = new PlayerStatusWidget();
        mPlayerStatusWidgetList.push_back( playerStatusWidget );
        mLayout->addWidget( playerStatusWidget );
        mLayout->addSpacing( 15 );
    }

    mLayout->addSpacing( 15 );
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


