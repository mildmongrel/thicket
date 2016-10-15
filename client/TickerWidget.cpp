#include "TickerWidget.h"
#include <QTimer>
#include <QFontMetrics>
#include <QResizeEvent>

// Tick rate.  50ms = 20 ticks/sec.
static const int TICK_RATE_MILLIS = 50;
static const int TICKS_PER_SECOND = 1000 / TICK_RATE_MILLIS;


TickerWidget::TickerWidget( QWidget* parent )
  : QFrame( parent ),
    mCurrentWidget( nullptr ),
    mOffsetY( 0 ),
    mOffsetYTarget( 0 )
{
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    setBackgroundRole( QPalette::Base );
    setAutoFillBackground( true );

    mTimer = new QTimer( this );
    connect( mTimer, &QTimer::timeout, this, &TickerWidget::handleTimerEvent );
}


TickerWidget::~TickerWidget()
{
    // This object owns its widgets even if they're not visible.
    while( !mPermanentWidgetList.empty() ) mPermanentWidgetList.takeFirst()->deleteLater();
    while( !mOneShotWidgetList.empty() ) mOneShotWidgetList.takeFirst()->deleteLater();
}


void
TickerWidget::start()
{
    mTimer->start( TICK_RATE_MILLIS );
}


void
TickerWidget::stop()
{
    mTimer->stop();
}

void
TickerWidget::addPermanentWidget( QWidget* widget )
{
    mPermanentWidgetList.push_back( widget );

    if( mCurrentWidget == nullptr )
    {
        startNextWidget();
    }
}


void
TickerWidget::enqueueOneShotWidget( QWidget* widget )
{
    mOneShotWidgetList.push_back( widget );

    if( mCurrentWidget == nullptr )
    {
        startNextWidget();
    }
}


QWidget*
TickerWidget::takePermanentWidgetAt( int index )
{
    if( (index < 0) || (index >= mPermanentWidgetList.size()) ) return nullptr;

    QWidget* widget = mPermanentWidgetList.takeAt( index );
    if( (widget == mCurrentWidget) )
    {
        widget->hide();
        widget->setParent( 0 );
        mCurrentWidget = nullptr;
    }
    startNextWidget();

    return widget;
}


int
TickerWidget::getInteriorHeight()
{
    return height() - (frameWidth() * 2);
}


void
TickerWidget::showEvent( QShowEvent* showEvent )
{
    // When the window is shown we want to (re)start our animation for the current widget.
// TODO not sure this is wanted anymore - why was it here?
//    mCurrentWidget = nullptr;
//    startNextWidget();
    QFrame::showEvent( showEvent );
}


void
TickerWidget::resizeEvent( QResizeEvent* resizeEvent )
{
    // Adjust the offset to recenter when the frame is resized.
    if( mCurrentWidget != nullptr )
    {
        QPoint topLeft = rect().topLeft();
        int offsetX = (width() / 2) - (mCurrentWidget->width() / 2);
        mCurrentWidget->move( topLeft.x() + offsetX, topLeft.y() + mOffsetY );
    }
}


void
TickerWidget::handleTimerEvent()
{
    static int pauseCounter = 0;

    if( mCurrentWidget == nullptr ) return;

    QPoint topLeft = rect().topLeft();
    if( mOffsetY > mOffsetYTarget )
    {
        // Target reached.
        mOffsetY -= 2;
        if( mOffsetY < mOffsetYTarget )
        {
            mOffsetY = mOffsetYTarget;
        }
        pauseCounter = TICKS_PER_SECOND * 3;
    }
    else if( pauseCounter > 0 )
    {
        const bool permanent = mPermanentWidgetList.contains( mCurrentWidget );
        if( permanent && !mOneShotWidgetList.empty() )
        {
            // If showing a permanent widget and a one-shot showed up, resume right away.
            pauseCounter = 0;
        }
        else if( permanent && (mPermanentWidgetList.size() == 1) )
        {
            // If showing a permanent widget and it's the only one, pause indefinitely.
        }
        else if( underMouse() )
        {
            // If the user is hovering over the widget, pause indefinitely.
        }
        else
        {
            pauseCounter--;
        }
    }
    else
    {
        mOffsetY -= 2;
        if( mOffsetY < -(mCurrentWidget->height()) )
        {
            startNextWidget();

            // If there was no new widget to start, then return right away.
            if( mCurrentWidget == nullptr ) return;
        }
    }

    // Move the widget, keeping it centered horizontally.
    int offsetX = (width() / 2) - (mCurrentWidget->width() / 2);
    mCurrentWidget->move( topLeft.x() + offsetX, topLeft.y() + mOffsetY );
}


void
TickerWidget::startNextWidget()
{
    // First, stop showing the current widget.
    if( mCurrentWidget != nullptr )
    {
        mCurrentWidget->hide();
        mCurrentWidget->setParent( 0 );
    }

    // If the current widget was a one-shot, remove and delete it here.
    if( mOneShotWidgetList.removeOne( mCurrentWidget ) )
    {
        mCurrentWidget->deleteLater();
    }

    // Now find the next widget and make it our current widget.
    if( !mOneShotWidgetList.isEmpty() )
    {
        mCurrentWidget = mOneShotWidgetList[0];
    }
    else if( !mPermanentWidgetList.isEmpty() )
    {
        int index = mPermanentWidgetList.indexOf( mCurrentWidget );
        int nextIndex = (index < 0) ? 0 : index + 1;
        nextIndex = nextIndex % mPermanentWidgetList.size();
        mCurrentWidget = mPermanentWidgetList[nextIndex];
    }
    else
    {
        mCurrentWidget = nullptr;
        return;
    }

    mCurrentWidget->setParent( this );
    mCurrentWidget->show();

    // Compute the target y-offset so that the widget will be centered
    // vertically once it gets there.
    mOffsetYTarget = (height() / 2) - (mCurrentWidget->height() / 2);

    // Center current widget horizontally in frame, just out-of-frame vertically.
    QPoint topLeft = rect().topLeft();
    int offsetX = (width() / 2) - (mCurrentWidget->width() / 2);
    mOffsetY = height();
    mCurrentWidget->move( topLeft.x() + offsetX, topLeft.y() + mOffsetY );
}
