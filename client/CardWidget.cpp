#include "CardWidget.h"

#include <QMouseEvent>
#include <QToolTip>
#include <QPainter>   // for Overlay painting

#include "qtutils_widget.h"
#include "CardData.h"
#include "ImageLoaderFactory.h"
#include "ImageLoader.h"

CardWidget::CardWidget( const CardDataSharedPtr& cardDataSharedPtr,
                        ImageLoaderFactory*      imageLoaderFactory,
                        const QSize&             defaultSize,
                        const Logging::Config&   loggingConfig,
                        QWidget*                 parent )
    : QLabel( parent ),
      mCardDataSharedPtr( cardDataSharedPtr ),
      mImageLoaderFactory( imageLoaderFactory ),
      mImageLoader( 0 ),
      mDefaultSize( defaultSize ),
      mZoomFactor( 1.0f ),
      mPreselectable( false ),
      mPreselected( false ),
      mMouseWithin( false ),
      mOverlay( nullptr ),
      mLoggingConfig( loggingConfig ),
      mLogger( loggingConfig.createLogger() )
{
    // Set up the label in it's unloaded state.
    setAlignment( Qt::AlignCenter );
    setWordWrap( true );
    setText( "<b>" + QString::fromStdString( mCardDataSharedPtr->getName() ) + "</b>" );

    // Set a style to draw a border and background before the card is loaded
    // with an image.
    setStyleSheet( "border: 2px dashed darkgray;"
                   "border-radius: 12px;"
                   "background-color: lightgray;" );
}
 

void
CardWidget::setZoomFactor( float zoomFactor )
{
    // Don't do any work if nothing has changed.
    if( mZoomFactor == zoomFactor ) return;

    mZoomFactor = zoomFactor;
    updatePixmap();
}


void
CardWidget::updatePixmap()
{
    if( mPixmap.isNull() ) return;

    setStyleSheet( "" );

    if( mZoomFactor == 1.0f )
    {
        setPixmap( mPixmap );
        setToolTip( QString() );
    }
    else
    {
        QSize scaledSize = mPixmap.size() * mZoomFactor;
        QPixmap scaledPixmap = mPixmap.scaled( scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        setPixmap( scaledPixmap );
    }
    adjustSize();
}


void
CardWidget::initOverlay()
{
    mOverlay = new CardWidget_Overlay( this );

    // Forward preselect signal from overlay to this widget.
    connect( mOverlay, SIGNAL(preselectRequested()), this, SIGNAL(preselectRequested()) );

    mOverlay->show();
}


void
CardWidget::loadImage()
{
    const int muid = mCardDataSharedPtr->getMultiverseId();
    if( muid < 0 )
    {
        mLogger->debug( "skipping image load of -1 muid" );
        return;
    }

    if( mImageLoader != 0 ) mImageLoader->deleteLater();
    mImageLoader = mImageLoaderFactory->createImageLoader(
            mLoggingConfig.createChildConfig( "imageloader" ), this );
    connect(mImageLoader, SIGNAL(imageLoaded(int, const QImage&)),
            this, SLOT(handleImageLoaded(int, const QImage&)));
    mImageLoader->loadImage( muid );
}


void
CardWidget::setPreselected( bool enabled )
{
    if( !mPreselectable ) return;

    // If this was switched on without an overlay, create the overlay.
    if( enabled && !mOverlay )
    {
        initOverlay();
    }

    // If this was switched off with an overlay and the mouse isn't inside then destroy the overlay.
    if( !enabled && mOverlay && !mMouseWithin )
    {
        mOverlay->deleteLater();
        mOverlay = nullptr;
    }

    // If there's an overlay left, set its preselect state.
    if( mOverlay )
    {
        mOverlay->setPreselected( enabled );
    }

    mPreselected = enabled;
}


QSize
CardWidget::sizeHint() const
{
    return mPixmap.isNull() ? mDefaultSize * mZoomFactor : QLabel::sizeHint();
}


void
CardWidget::mousePressEvent( QMouseEvent* event )
{
    if( event->modifiers() & Qt::ShiftModifier )
    {
        emit moveRequested();
    }
}


void
CardWidget::mouseDoubleClickEvent( QMouseEvent* event )
{
    emit selectRequested();
}


void
CardWidget::handleImageLoaded( int multiverseId, const QImage &image )
{
    mLogger->debug( "image {} loaded", multiverseId );

    if( mCardDataSharedPtr->getMultiverseId() == multiverseId )
    {
        mPixmap = QPixmap::fromImage( image );
        updatePixmap();
    }
    else
    {
        mLogger->error( "wrong image loaded, expected {} got {}", mCardDataSharedPtr->getMultiverseId(), multiverseId );
    }

    // Finished with the loader after this is called.
    mImageLoader->deleteLater();
}


void
CardWidget::enterEvent( QEvent* event )
{
    mMouseWithin = true;

    if( !mPixmap.isNull() )
    {
        // If this is a split card, always show a rotated tooltip.  Otherwise
        // show a normal tooltip if the view is zoomed out.
        if( mCardDataSharedPtr->isSplit() )
        {
            QTransform transform;
            transform.rotate( 90 );
            QPixmap rotatedPixmap = mPixmap.transformed( transform );
            QString toolTipStr = qtutils::getPixmapAsHtmlText( rotatedPixmap );
            QToolTip::showText( QCursor::pos(), toolTipStr, this );
        }
        else if( mZoomFactor < 1.0f )
        {
            QString toolTipStr = qtutils::getPixmapAsHtmlText( mPixmap );
            QToolTip::showText( QCursor::pos(), toolTipStr, this );
        }
    }

    if( mPreselectable && !mOverlay )
    {
        initOverlay();
    }

    QLabel::enterEvent( event );
}


void
CardWidget::leaveEvent( QEvent* event )
{
    mMouseWithin = false;

    if( mOverlay && !mPreselected )
    {
        mOverlay->deleteLater();
        mOverlay = nullptr;
    }
    QLabel::leaveEvent( event );
}


/*
 * CardWidget_Overlay
 */


CardWidget_Overlay::CardWidget_Overlay( QWidget* parent )
  : OverlayWidget( parent ),
    mPreselected( false )
{
    setAttribute( Qt::WA_TranslucentBackground );
    setMouseTracking( true );
}


void
CardWidget_Overlay::setPreselected( bool enabled )
{
    mPreselected = enabled;
    update();
}


void
CardWidget_Overlay::mouseMoveEvent( QMouseEvent *event )
{
    mMousePos = event->pos();
    OverlayWidget::mouseMoveEvent( event );
}


void
CardWidget_Overlay::mousePressEvent( QMouseEvent *event )
{
    if( !mPreselected && mPreselectRect.contains( event->pos() ) )
    {
        emit preselectRequested();
    }
    OverlayWidget::mousePressEvent( event );
}


void
CardWidget_Overlay::mouseDoubleClickEvent( QMouseEvent *event )
{
    // Do not call parent mouseDoubleClickEvent() if within the mPreselectRect
    // to prevent the preselect action from triggering an actual selection
    if( !mPreselectRect.contains( event->pos() ) )
    {
        OverlayWidget::mousePressEvent( event );
    }
}


bool
CardWidget_Overlay::event( QEvent *event )
{
    // Catch tooltip events for the entire overlay and create specific behavior.
    if( event->type() == QEvent::ToolTip )
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>( event );
        if( mPreselectRect.contains( helpEvent->pos() ) )
        {
            QToolTip::showText( helpEvent->globalPos(), tr("Select this card when timer expires") );
        }
        else
        {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }
    return OverlayWidget::event( event );
}


void
CardWidget_Overlay::resizeEvent( QResizeEvent* resizeEvent )
{
    // Make some size-based calculations here for mouse tracking and painting.
    const int margin = (qreal)rect().height() * 0.02f;
    const int preselectSideLen = (qreal)rect().height() * 0.09f;
    mPreselectRect.setWidth( preselectSideLen );
    mPreselectRect.setHeight( preselectSideLen );
    mPreselectRect.moveBottomLeft( rect().bottomLeft() );
    mPreselectRect.translate( margin, -margin );

    mPreselectRectF = QRectF( mPreselectRect );

    // Fill background area with black in the center, fade to transparent at edges
    mBackgroundRadialGradient = QRadialGradient( mPreselectRectF.center(), mPreselectRectF.width() / 2.0f );
    mBackgroundRadialGradient.setColorAt( 0.0f, QColor( 0, 0, 0, 200 ) );
    mBackgroundRadialGradient.setColorAt( 0.7f, QColor( 0, 0, 0, 200 ) );
    mBackgroundRadialGradient.setColorAt( 1.0f, QColor( 0, 0, 0, 0 ) );

    // Foreground rectangle.
    mIconRectF.setSize( mPreselectRectF.size() * 0.75f );
    mIconRectF.moveCenter( mPreselectRectF.center() );

    //
    // Pushpin elements.
    //

    mPinTopRectF = mIconRectF;
    mPinTopRectF.setWidth( mIconRectF.width() * 0.4f );
    mPinTopRectF.setHeight( mIconRectF.width() * 0.1f );
    QPointF pinTopCenterF( mIconRectF.center().x(), mIconRectF.top() + (mIconRectF.height() * 0.2f) );
    mPinTopRectF.moveCenter( pinTopCenterF );

    mPinHandleRectF = mIconRectF;
    mPinHandleRectF.setWidth( mIconRectF.width() * 0.2f );
    mPinHandleRectF.setHeight( mIconRectF.width() * 0.4f );
    QPointF pinHandleCenterF( mIconRectF.center().x(), mIconRectF.top() + (mIconRectF.height() * 0.4f) );
    mPinHandleRectF.moveCenter( pinHandleCenterF );

    QRectF pinBaseRectF = mIconRectF;
    pinBaseRectF.setWidth( mIconRectF.width() * 0.5f );
    pinBaseRectF.setHeight( mIconRectF.width() * 0.2f );
    QPointF pinBaseCenterF( mIconRectF.center().x(), mIconRectF.top() + (mIconRectF.height() * 0.5f) );
    pinBaseRectF.moveCenter( pinBaseCenterF );
    mPinBasePath.moveTo( pinBaseRectF.bottomLeft() );
    mPinBasePath.lineTo( pinBaseRectF.center().x(), pinBaseRectF.top() );
    mPinBasePath.lineTo( pinBaseRectF.bottomRight() );
    mPinBasePath.moveTo( pinBaseRectF.bottomLeft() );

    mPinPath.moveTo( pinTopCenterF );
    mPinPath.lineTo( pinTopCenterF.x(), mIconRectF.top() + (mIconRectF.height() * 0.9f) );

    // Allow parent class to handle resize event.
    OverlayWidget::resizeEvent( resizeEvent );
}


void
CardWidget_Overlay::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    p.setRenderHint( QPainter::Antialiasing );

    // draw background shadow
    p.setPen( Qt::NoPen );
    p.setBrush( mBackgroundRadialGradient );
    p.drawEllipse( mPreselectRectF );

    // draw icon background
    if( mPreselected )
    {
        p.setBrush( QColor( 255, 255, 255, 255 ) );
        p.drawEllipse( mIconRectF );
    }
    else
    {
        p.setBrush( QColor( 255, 255, 255, 128 ) );
        p.drawEllipse( mIconRectF );
    }

    p.fillRect( mPinTopRectF, Qt::black );
    p.fillRect( mPinHandleRectF, Qt::black );
    p.fillPath( mPinBasePath, Qt::black );
    p.setPen( Qt::black );
    p.drawPath( mPinPath );

    OverlayWidget::paintEvent( event );
}
