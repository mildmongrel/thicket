#ifndef TICKERPOSTROUNDTIMERWIDGET_H
#define TICKERPOSTROUNDTIMERWIDGET_H

#include "TickerChildWidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class TickerPostRoundTimerWidget : public TickerChildWidget
{
    Q_OBJECT

public:

    TickerPostRoundTimerWidget( int tickerHeight, QWidget* parent = 0 );

    void setSecondsRemaining( int secs );

private:
    QLabel* mLabel;
};

#endif  // TICKERPOSTROUNDTIMERWIDGET_H

