#include "TickerPlayerHashesWidget.h"

#include <QBoxLayout>
#include <QLabel>

#include "qtutils_widget.h"
#include "RoomStateAccumulator.h"


void
TickerPlayerHashesWidget::update( const RoomStateAccumulator& roomState )
{
    qtutils::clearLayout( mLayout );

    int hashes = 0;
    for( int i = 0; i < roomState.getChairCount(); ++i )
    {
        // If there's a name and a hash, proceed regardless of player state
        // (could be stale or departed)  Want all hashes to be shown after draft.
        if( roomState.hasPlayerName( i ) && roomState.hasPlayerCockatriceHash( i ) )
        {
            QString name = QString::fromStdString( roomState.getPlayerName( i ) );
            QString hash = QString::fromStdString( roomState.getPlayerCockatriceHash( i ) );
            QLabel* label = new QLabel( QString( "<b>%1:</b> %2" ).arg( name ).arg( hash ) );

            label->setContentsMargins( 0, 0, 0, 0 );

            if( hashes > 0 )
            {
                mLayout->addSpacing( 30 );
            }
            mLayout->addWidget( label );

            // It's possible (likely?) that this widget is showing.  If so, Qt
            // doesn't automatically show new widgets when they're added to a layout.
            // Need to manually make them visible.
            if( !isHidden() )
            {
                label->setVisible(true);
            }

            hashes++;
        }
    }

    adjustSize();
}
