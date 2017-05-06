#ifndef TICKERPLAYERSTATUSWIDGET_H
#define TICKERPLAYERSTATUSWIDGET_H

#include <QMap>

#include "TickerChildWidget.h"
#include "RoomStateAccumulator.h"

class SizedSvgWidget;
class PlayerStatusWidget;

class TickerPlayerStatusWidget : public TickerChildWidget
{
    Q_OBJECT

public:

    TickerPlayerStatusWidget( int tickerHeight, QWidget* parent = 0 )
      : TickerChildWidget( tickerHeight, parent ),
        mBuilt( false ),
        mChairCount( 0 ),
        mIsPublicDraftType( false ),
        mPassDirection( PASS_DIRECTION_NONE )
    {}

    void update( const RoomStateAccumulator& roomState );

private:

    void build( const RoomStateAccumulator& roomState );

    bool mBuilt;
    int mChairCount;
    bool mIsPublicDraftType;
    PassDirection mPassDirection;

    SizedSvgWidget* mPassDirLeftWidget;
    SizedSvgWidget* mPassDirRightWidget;

    // Arrows between players.  There are (CHAIR_COUNT - 1) of these.
    QList<SizedSvgWidget*>     mPassDirWidgetList;

    // Player status widgets.  There are CHAIR_COUNT of these.
    QList<PlayerStatusWidget*> mPlayerStatusWidgetList;
};

#endif  // TICKERPLAYERSTATUSWIDGET_H

