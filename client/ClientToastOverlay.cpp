#include "ClientToastOverlay.h"

#include <QPainter>
#include <QTextDocument>
#include <QTimer>
#include <QVector>

static const int TOAST_DEFAULT_TTL         = 10000;
static const int TOAST_WIDTH               = 200;
static const int TOAST_MARGIN              = 15;
static const int TOAST_SPACING             = 5;
static const int TOAST_BG_STARTING_OPACITY = 192;


ClientToastOverlay::ClientToastOverlay( QWidget* parent )
  : OverlayWidget( parent ),
    mTimeToLive( TOAST_DEFAULT_TTL )
{
    setAttribute( Qt::WA_TranslucentBackground );

    mTimer = new QTimer( this );
    mTimer->setInterval( 25 );
    connect( mTimer, &QTimer::timeout, this, &ClientToastOverlay::handleTimerExpired );
}


void
ClientToastOverlay::addToast( const QString& text )
{
    ToastData td;

    td.text = text;
    td.age = 0;
    td.opacity = TOAST_BG_STARTING_OPACITY;

    td.doc = new QTextDocument();
    td.doc->setTextWidth( TOAST_WIDTH );
    td.doc->setDocumentMargin( TOAST_MARGIN );
    formatTextDocument( *(td.doc), td.text, td.opacity );

    mToasts.push_back( td );

    // Start the timer if it isn't already running.
    if( !mTimer->isActive() ) mTimer->start();

    updateToastRects();
}


void
ClientToastOverlay::clearToasts()
{
    auto iter = mToasts.begin();
    while( iter != mToasts.end() )
    {
        iter->doc->deleteLater();
        iter = mToasts.erase( iter );
    }

    // With no toasts this will end up hiding the widget altogether.
    updateToastRects();

    mTimer->stop();
}


void
ClientToastOverlay::mousePressEvent( QMouseEvent *event )
{
    // Currently does nothing, but can be used as a hook to dismiss a toast.
    OverlayWidget::mousePressEvent( event );
}


void
ClientToastOverlay::resizeEvent( QResizeEvent* resizeEvent )
{
    updateToastRects();

    // Allow parent class to handle resize event.
    OverlayWidget::resizeEvent( resizeEvent );
}


void
ClientToastOverlay::paintEvent( QPaintEvent* event )
{
    // Bail immediately if there aren't any toasts to show.
    if( mToasts.empty() ) return;

    for( const auto& toast : mToasts )
    {
        QPainter p( this );
        p.setRenderHint( QPainter::Antialiasing );

        // Paint background rectangle.
        p.setPen( Qt::NoPen );
        p.setBrush( QColor( 0, 0, 0, toast.opacity ) );
        p.drawRoundedRect( toast.rect, 10, 10 );

        // Paint text.
        p.translate( toast.rect.topLeft() );
        toast.doc->drawContents( &p );
    }

    OverlayWidget::paintEvent( event );
}


void
ClientToastOverlay::handleTimerExpired()
{
    bool toastRemoved = false;

    auto iter = mToasts.begin();
    while( iter != mToasts.end() )
    {
        ToastData& td = *iter;
        td.age += mTimer->interval();
        if( td.age >= mTimeToLive )
        {
            td.doc->deleteLater();
            iter = mToasts.erase( iter );
            toastRemoved = true;
        }
        else
        {
            // Opacity of the toast fades with age.
            int opacity = TOAST_BG_STARTING_OPACITY;
            const int rolloffStart = mTimeToLive / 2;
            const int rolloffLength = mTimeToLive  - rolloffStart;
            if( td.age > rolloffStart )
            {
                int rolloffProgress = td.age - rolloffStart;
                opacity -= (opacity * rolloffProgress) / rolloffLength;
            }
            td.opacity = opacity;

            formatTextDocument( *(td.doc), td.text, opacity );

            iter++;
        }
    }

    // Anytime a toast is removed, the repaint can leave artifacts of the
    // removed toast laying around, particularly in viewports of scroll
    // areas.  Hiding the widget (and re-showing it if necessary in
    // updateToastRects) does the trick of forcing the parent to repaint
    // stuff under this widget so that the artifacts don't appear.
    if( toastRemoved ) setVisible( false );

    updateToastRects();

    // Stop the time if no toasts left.
    if( mToasts.empty() ) mTimer->stop();
}


void
ClientToastOverlay::updateToastRects()
{
    // This region will hold the widget mask.  Setting the mask allows
    // mouse events to properly "go around" the unmasked areas, i.e. to
    // the rest of the client.
    QRegion maskRegion;

    QPoint br = rect().bottomRight() + mBottomRightOffset;
    for( auto& toast : mToasts )
    {
        toast.rect = QRect( 0, 0, toast.doc->textWidth(), toast.doc->size().height() );
        toast.rect.moveBottomRight( br );

        maskRegion += toast.rect;

        br.ry() -= toast.rect.height() + TOAST_SPACING;
    }

    if( !maskRegion.isEmpty() )
    {
        setVisible( true );
        setMask( maskRegion );
    }
    else
    {
        setVisible( false );
    }

    update();
}


void
ClientToastOverlay::formatTextDocument( QTextDocument& doc, const QString& text, int opacity )
{
    static const QString styleSheetTemplate( "body { color: rgba(255,255,255,%1); font-size: large; vertical-align: middle }" );
    static const QString htmlTemplate( "<body>%1</body>" );

    doc.setDefaultStyleSheet( styleSheetTemplate.arg( QString::number( opacity ) ) );
    doc.setHtml( htmlTemplate.arg( text ) );
}

