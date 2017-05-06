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
      mDimmed( false ),
      mHighlighted( false ),
      mSelectedByOpponent( false ),
      mSelectedByPlayer( false ),
      mMouseWithin( false ),
      mOverlay( nullptr ),
      mLoggingConfig( loggingConfig ),
      mLogger( loggingConfig.createLogger() )
{
    // Set up the label in it's unloaded state.
    setAlignment( Qt::AlignCenter );
    setWordWrap( true );
    setText( "<b>" + QString::fromStdString( mCardDataSharedPtr->getName() ) + "</b>" );

    // Set fixed size to default.
    setFixedSize( mDefaultSize );

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
    updateScaling();
}


void
CardWidget::setPreselectable( bool enabled )
{
    mPreselectable = enabled;
    updateOverlay();
}


void
CardWidget::setPreselected( bool enabled )
{
    mPreselected = enabled;
    updateOverlay();
}


void
CardWidget::setDimmed( bool enabled )
{
    mDimmed = enabled;
    updateOverlay();
}


void
CardWidget::setHighlighted( bool enabled )
{
    mHighlighted = enabled;
    updateOverlay();
}


void
CardWidget::setSelectedByOpponent( bool enabled )
{
    mSelectedByOpponent = enabled;
    updateOverlay();
}


void
CardWidget::setSelectedByPlayer( bool enabled )
{
    mSelectedByPlayer = enabled;
    updateOverlay();
}


void
CardWidget::resetTraits()
{
    mPreselectable = false;
    mPreselected = false;
    mDimmed = false;
    mHighlighted = false;
    mSelectedByOpponent = false;
    mSelectedByPlayer = false;
    updateOverlay();
}


void
CardWidget::updateScaling()
{
    if( mPixmap.isNull() )
    {
        setFixedSize( mDefaultSize * mZoomFactor );
    }
    else
    {
        setStyleSheet( "" );

        if( mZoomFactor == 1.0f )
        {
            setPixmap( mPixmap );
            setFixedSize( mPixmap.size() );
        }
        else
        {
            QSize scaledSize = mPixmap.size() * mZoomFactor;
            QPixmap scaledPixmap = mPixmap.scaled( scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
            setPixmap( scaledPixmap );
            setFixedSize( scaledSize );
        }
    }

    adjustSize();
}


void
CardWidget::updateOverlay()
{
    bool preselectableMouseWithin = mPreselectable && mMouseWithin;
    bool preselectablePreselected = mPreselectable && mPreselected;

    bool overlayRequired = mDimmed || mHighlighted || mSelectedByOpponent || mSelectedByPlayer || preselectableMouseWithin || preselectablePreselected;

    if( !mOverlay && overlayRequired )
    {
        // === Create the overlay ===

        mOverlay = new CardWidget_Overlay( this );
        mOverlay->show();

        // Mouse tracking is needed to detect movement for overlay.
        setMouseTracking( true );
    }

    if( mOverlay && !overlayRequired )
    {
        // Mouse tracking no longer needed,
        setMouseTracking( false );

        // === Destroy the overlay ===

        mOverlay->deleteLater();
        mOverlay = nullptr;
    }

    if( mOverlay )
    {
        mOverlay->update();
    }
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
CardWidget::mousePressEvent( QMouseEvent* event )
{
    if( mOverlay && mOverlay->inPreselectRegion( event->pos() ) )
    {
        emit preselectRequested();
    }
    else if( event->modifiers() & Qt::ShiftModifier )
    {
        emit moveRequested();
    }
}


void
CardWidget::mouseDoubleClickEvent( QMouseEvent* event )
{
    if( mOverlay && mOverlay->inPreselectRegion( event->pos() ) )
    {
        // Do nothing if double-clicking in preselect region
        return;
    }

    emit selectRequested();
}


void
CardWidget::mouseMoveEvent( QMouseEvent *event )
{
    // Update the tooltip.
    if( mOverlay && mOverlay->inPreselectRegion( event->pos() ) )
    {
        QToolTip::showText( event->globalPos(), tr("Select this card when timer expires"), this );
    }
    else
    {
        QToolTip::showText( event->globalPos(), mToolTipStr, this );
    }
}


void
CardWidget::handleImageLoaded( int multiverseId, const QImage &image )
{
    mLogger->debug( "image {} loaded", multiverseId );

    if( mCardDataSharedPtr->getMultiverseId() == multiverseId )
    {
        mPixmap = QPixmap::fromImage( image );
        updateScaling();
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

    // Set the default tooltip string.
    mToolTipStr.clear();
    if( !mPixmap.isNull() )
    {
        // If this is a split card, always show a rotated tooltip.  Otherwise
        // show a normal tooltip if the view is zoomed out.
        if( mCardDataSharedPtr->isSplit() )
        {
            QTransform transform;
            transform.rotate( 90 );
            QPixmap rotatedPixmap = mPixmap.transformed( transform );
            mToolTipStr = qtutils::getPixmapAsHtmlText( rotatedPixmap );
        }
        else if( mZoomFactor < 1.0f )
        {
            mToolTipStr = qtutils::getPixmapAsHtmlText( mPixmap );
        }
    }

    // Set the tooltip.  Note that 'this' is necessary for the tooltip to
    // disappear when the mouse leaves the widget.
    QToolTip::showText( QCursor::pos(), mToolTipStr, this );

    // May need to create the overlay based on mouse entry.
    updateOverlay();

    QLabel::enterEvent( event );
}


void
CardWidget::leaveEvent( QEvent* event )
{
    mMouseWithin = false;

    // The tooltip string can be very large, so it's cleared here to
    // avoid huge memory allocations.
    mToolTipStr.clear();

    // May need to destroy the overlay based on mouse exit.
    updateOverlay();

    QLabel::leaveEvent( event );
}


bool
CardWidget::event( QEvent *event )
{
    if( event->type() == QEvent::ToolTip )
    {
        // This widget manages its own tooltips.
        return true;
    }
    return QLabel::event( event );
}


/*
 * CardWidget_Overlay
 */


CardWidget_Overlay::CardWidget_Overlay( CardWidget* parent )
  : OverlayWidget( parent ),
    mParentCardWidget( parent )
{
    setAttribute( Qt::WA_TranslucentBackground );

    // Qt handles mouse events strangely when widgets are overlaid; pass
    // everything through and handle mouse events/tracking in parent.
    setAttribute( Qt::WA_TransparentForMouseEvents );
}


bool
CardWidget_Overlay::inPreselectRegion( const QPoint& pos ) const
{
    return mPreselectRect.contains( pos );
}


void
CardWidget_Overlay::resizeEvent( QResizeEvent* resizeEvent )
{
    const QRectF widgetRectF( rect() );

    // Make some size-based calculations here for mouse tracking and painting.
    const int margin = widgetRectF.height() * 0.02f;
    const int preselectSideLen = widgetRectF.height() * 0.09f;
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

    // 'Selected' banner rectangle.
    mBannerRectF.setWidth( widgetRectF.width() * 0.95f );
    mBannerRectF.setHeight( widgetRectF.height() * 0.05f );
    mBannerRectF.moveCenter( widgetRectF.center() );

    // Allow parent class to handle resize event.
    OverlayWidget::resizeEvent( resizeEvent );
}


void
CardWidget_Overlay::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    p.setRenderHint( QPainter::Antialiasing );

    if( mParentCardWidget->isDimmed() )
    {
        // Fill the whole overlay with a transparent white layer.
        p.fillRect( rect(), QColor( 255, 255, 255, 128 ) );
    }

    if( mParentCardWidget->isHighlighted() )
    {
        const QRectF widgetRectF( rect() );
        QPen pen( Qt::green );
        pen.setWidth( widgetRectF.width() * 0.02f );
        p.setPen( pen );
        p.setBrush( QColor( 0, 255, 0, 32 ) );
        const qreal radius = widgetRectF.width() * 0.05f;
        p.drawRoundedRect( widgetRectF, radius, radius );
    }

    if( mParentCardWidget->isPreselectable() )
    {
        // draw background shadow
        p.setPen( Qt::NoPen );
        p.setBrush( mBackgroundRadialGradient );
        p.drawEllipse( mPreselectRectF );

        // draw icon background
        if( mParentCardWidget->isPreselected() )
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
    }

    auto paintBannerFn = [this, &p]( const QColor&  bannerPenColor,
                                     const QColor&  bannerBrushColor,
                                     const QString& bannerText ) {
            p.setPen( bannerPenColor );
            p.setBrush( bannerBrushColor );
            p.drawRect( mBannerRectF );

            QFont fnt;
            fnt.setPointSize( mBannerRectF.height() * 0.5f );
            p.setFont( fnt );
            p.setPen( Qt::white );
            p.drawText( mBannerRectF, Qt::AlignCenter, bannerText );
        };

    if( mParentCardWidget->isSelectedByOpponent() )
    {
        paintBannerFn( Qt::red, QColor( 255, 0, 0, 192 ), tr("SELECTED BY OPPONENT") );
    }
    else if( mParentCardWidget->isSelectedByPlayer() )
    {
        paintBannerFn( QColor( 0, 128, 0 ), QColor( 0, 128, 0, 192 ), tr("SELECTED") );
    }

    OverlayWidget::paintEvent( event );
}
