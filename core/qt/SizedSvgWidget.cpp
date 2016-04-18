#include "SizedSvgWidget.h"
#include <QSvgRenderer>


SizedSvgWidget::SizedSvgWidget( const QSize& size, Qt::AspectRatioMode mode, QWidget* parent )
  : QSvgWidget( parent ),
    mScalingSize( size ),
    mAspectRatioMode( mode )
{
    setSize( getAdjustedSize() );
}


void
SizedSvgWidget::setScaling( const QSize& size, Qt::AspectRatioMode mode )
{
    mScalingSize = size;
    mAspectRatioMode = mode;

    setSize( getAdjustedSize() );
}

void
SizedSvgWidget::setSize( const QSize& size )
{
    if( size.isValid() )
    {
        setMinimumSize( size );
        setMaximumSize( size );
    }
}


void
SizedSvgWidget::load( const QString& file )
{
    QSvgWidget::load( file );
    setSize( getAdjustedSize() );
}


void
SizedSvgWidget::load( const QByteArray& contents )
{
    QSvgWidget::load( contents );
    setSize( getAdjustedSize() );
}


QSize
SizedSvgWidget::getAdjustedSize() const
{
    if( mScalingSize.isValid() )
    {
        QSize size = renderer()->defaultSize();
        size.scale( mScalingSize, mAspectRatioMode );
        return size;
    }
    else
    {
        return QSvgWidget::sizeHint();
    }
}
