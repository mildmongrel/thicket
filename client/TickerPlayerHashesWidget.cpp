#include "TickerPlayerHashesWidget.h"

#include <QBoxLayout>
#include <QLabel>

#include "qtutils_widget.h"


void
TickerPlayerHashesWidget::setChairs( int chairCount, const QMap<int,PlayerInfo>& playerInfoMap )
{
    qtutils::clearLayout( mLayout );

    for( int i = 0; i < chairCount; ++i )
    {
        QLabel* label;

        if( playerInfoMap.contains( i ) )
        {
            label = new QLabel( QString( "<b>%1:</b> %2" ).arg( playerInfoMap[i].name ).arg( playerInfoMap[i].cockatriceHash ) );
        }
        else
        {
            label = new QLabel( "(open chair)" );
            label->setStyleSheet( "QLabel { color: gray; }" );
        }

        label->setContentsMargins( 0, 0, 0, 0 );
        mLayout->addWidget( label );

        if( i < chairCount - 1 )
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

