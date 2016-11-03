#ifndef TICKERPLAYERHASHESWIDGET_H
#define TICKERPLAYERHASHESWIDGET_H

#include <QMap>

#include "TickerChildWidget.h"

class RoomStateAccumulator;

class TickerPlayerHashesWidget : public TickerChildWidget
{
    Q_OBJECT

public:

    TickerPlayerHashesWidget( int tickerHeight, QWidget* parent = 0 )
      : TickerChildWidget( tickerHeight, parent )
    {}

    void update( const RoomStateAccumulator& roomState );
};

#endif  // TICKERPLAYERHASHESWIDGET_H

