#ifndef TICKERPLAYERREADYWIDGET_H
#define TICKERPLAYERREADYWIDGET_H

#include <QMap>

#include "TickerChildWidget.h"

class RoomStateAccumulator;

class TickerPlayerReadyWidget : public TickerChildWidget
{
    Q_OBJECT

public:

    TickerPlayerReadyWidget( int tickerHeight, QWidget* parent = 0 )
      : TickerChildWidget( tickerHeight, parent )
    {}

    void update( const RoomStateAccumulator& roomState );
};

#endif  // TICKERPLAYERREADYWIDGET_H

