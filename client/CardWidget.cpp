#include "CardWidget.h"

#include <QMouseEvent>
#include <QToolTip>

#include "qtutils.h"
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
 

QSize
CardWidget::sizeHint() const
{
    return mPixmap.isNull() ? mDefaultSize * mZoomFactor : QLabel::sizeHint();
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
    if( event->modifiers() & Qt::ShiftModifier )
    {
        emit shiftClicked();
    }
    else
    {
        emit clicked();
    }
}


void
CardWidget::mouseDoubleClickEvent( QMouseEvent* event )
{
    emit doubleClicked();
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
    if( mPixmap.isNull() ) return;

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


void
CardWidget::leaveEvent( QEvent* event )
{
}
