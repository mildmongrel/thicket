#include "TickerPlayerHashesWidget.h"

#include <QBoxLayout>
#include <QLabel>

#include "qtutils_widget.h"
#include "RoomStateAccumulator.h"


void
TickerPlayerHashesWidget::update( const RoomStateAccumulator& roomState )
{
    qtutils::clearLayout( mLayout );

    for( int i = 0; i < roomState.getChairCount(); ++i )
    {
        QLabel* label;

        if( roomState.hasPlayerName( i ) && roomState.hasPlayerCockatriceHash( i ) )
        {
            QString name = QString::fromStdString( roomState.getPlayerName( i ) );
            QString hash = QString::fromStdString( roomState.getPlayerCockatriceHash( i ) );
            label = new QLabel( QString( "<b>%1:</b> %2" ).arg( name ).arg( hash ) );
        }
        else
        {
            label = new QLabel( "(open chair)" );
            label->setStyleSheet( "QLabel { color: gray; }" );
        }

        label->setContentsMargins( 0, 0, 0, 0 );
        mLayout->addWidget( label );

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
        }
    }

    adjustSize();
}
