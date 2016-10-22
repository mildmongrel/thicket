#ifndef TICKERPLAYERHASHESWIDGET_H
#define TICKERPLAYERHASHESWIDGET_H

#include <QMap>

#include "TickerChildWidget.h"

class RoomStateAccumulator;

class TickerPlayerHashesWidget : public TickerChildWidget
{
    Q_OBJECT

public:

    struct PlayerInfo
    {
        PlayerInfo() {}
        PlayerInfo( QString zName, QString zCockatriceHash )
          : name( zName ),
            cockatriceHash( zCockatriceHash )
        {}

        QString name;
        QString cockatriceHash;
    };

    TickerPlayerHashesWidget( int tickerHeight, QWidget* parent = 0 )
      : TickerChildWidget( tickerHeight, parent )
    {}

    void update( const RoomStateAccumulator& roomState );
};

#endif  // TICKERPLAYERHASHESWIDGET_H

