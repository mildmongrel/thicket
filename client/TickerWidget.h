#ifndef TICKERWIDGET_H
#define TICKERWIDGET_H

#include <QFrame>

class TickerWidget : public QFrame
{
    Q_OBJECT

public:

    TickerWidget( QWidget* parent = 0 );
    virtual ~TickerWidget();

    void start();
    void stop();

    // Add a permanent widget that will be cycled through along with any
    // others.  The ticker widget takes ownership of the widget.
    void addPermanentWidget( QWidget* widget );

    // Take a permanent widget from the ticker.
    QWidget* takePermanentWidgetAt( int index );

    // Add a widget that shows once immediately then returns to showing
    // permanent widgets.  The ticker widget taked ownership of the
    // widget, deleting it once it's done showing.
    void enqueueOneShotWidget( QWidget* widget );

    int getInteriorHeight();

protected:

    virtual void resizeEvent( QResizeEvent* resizeEvent ) override;

private slots:

    virtual void handleTimerEvent();

private:

    void startNextWidget();

    QList<QWidget*> mPermanentWidgetList;
    QList<QWidget*> mOneShotWidgetList;
    QWidget*        mCurrentWidget;

    QTimer* mTimer;
    int     mOffsetY;
    int     mOffsetYTarget;
};

#endif  // TICKERWIDGET_H
