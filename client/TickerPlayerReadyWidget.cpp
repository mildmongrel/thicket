#include "TickerPlayerReadyWidget.h"

#include <QBoxLayout>
#include <QLabel>

#include "qtutils_widget.h"
#include "SizedSvgWidget.h"
#include "RoomStateAccumulator.h"

static const QString RESOURCE_SVG_CANCEL_BRIGHT( ":/cancel-bright.svg" );
static const QString RESOURCE_SVG_APPROVE_BRIGHT( ":/approve-bright.svg" );


void
TickerPlayerReadyWidget::update( const RoomStateAccumulator& roomState )
{
    qtutils::clearLayout( mLayout );

    for( int i = 0; i < roomState.getChairCount(); ++i )
    {
        QLabel* label;
        SizedSvgWidget* readyInd = nullptr;

        if( roomState.hasPlayerName( i ) && roomState.hasPlayerState( i ) )
        {
            QString name = QString::fromStdString( roomState.getPlayerName( i ) );
            label = new QLabel( QString( "<b>%1:</b>" ).arg( name ) );
            bool inRoom = roomState.getPlayerState( i ) == PLAYER_STATE_STANDBY ||
                          roomState.getPlayerState( i ) == PLAYER_STATE_READY   ||
                          roomState.getPlayerState( i ) == PLAYER_STATE_ACTIVE;
            label->setEnabled( inRoom );

            readyInd = new SizedSvgWidget( QSize( mTickerHeight, mTickerHeight ) );
            readyInd->setContentsMargins( 0, 0, 0, 0 );
            const bool ready = roomState.getPlayerState( i ) == PLAYER_STATE_READY;
            readyInd->load( ready ? RESOURCE_SVG_APPROVE_BRIGHT : RESOURCE_SVG_CANCEL_BRIGHT );
        }
        else
        {
            label = new QLabel( "(open chair)" );
            label->setStyleSheet( "QLabel { color: gray; }" );
        }

        label->setContentsMargins( 0, 0, 0, 0 );
        mLayout->addWidget( label );
        if( readyInd ) mLayout->addWidget( readyInd );

        if( i < roomState.getChairCount() - 1 )
        {
            mLayout->addSpacing( 30 );
        }

        // It's possible (likely?) that this widget is showing.  If so, Qt
        // doesn't automatically show new widgets when they're added to a layout.
        // Need to manually make them visible.
        if( !isHidden() )
        {
            label->setVisible(true);
            if( readyInd ) readyInd->setVisible(true);
        }
    }

    adjustSize();
}
