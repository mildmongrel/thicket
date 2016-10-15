#ifndef TICKERCHILDWIDGET_H
#define TICKERCHILDWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
QT_END_NAMESPACE

class TickerChildWidget : public QWidget
{
    Q_OBJECT

public:

    TickerChildWidget( int tickerHeight, QWidget* parent = 0 );
    virtual ~TickerChildWidget();

protected:

    const int mTickerHeight;
    QHBoxLayout* mLayout;

};

#endif  // TICKERCHILDWIDGET_H

