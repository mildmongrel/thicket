#ifndef TICKERPLAYERREADYWIDGET_H
#define TICKERPLAYERREADYWIDGET_H

#include <QMap>

#include "TickerChildWidget.h"

class SizedSvgWidget;
class RoomStateAccumulator;

class TickerPlayerReadyWidget : public TickerChildWidget
{
    Q_OBJECT

public:

    struct PlayerInfo
    {
        PlayerInfo() {}
        PlayerInfo( QString zName, bool zReady )
          : name( zName ),
            ready( zReady )
        {}

        QString name;
        bool    ready;
    };

    TickerPlayerReadyWidget( int tickerHeight, QWidget* parent = 0 )
      : TickerChildWidget( tickerHeight, parent )
    {}

    void update( const RoomStateAccumulator& roomState );
};

#endif  // TICKERPLAYERREADYWIDGET_H

