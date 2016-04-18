#include "PlayerStatusWidget.h"

#include <QHBoxLayout>
#include <QSvgRenderer>
#include <QPainter>

#include "DraftTimerWidget.h"

static const QString RESOURCE_SVG_CARD_BACK( ":/card-back-landscape.svg" );
static QPixmap *sPackQueueBasePixmap = nullptr;


PlayerStatusWidget::PlayerStatusWidget( const QString& name, QWidget* parent )
  : QWidget( parent )
{
    mNameLabel = new QLabel();
    mNameLabel->setText( createNameLabelString( name ) );

    mPackQueueLabel = new QLabel();

    mDraftTimerWidget = new DraftTimerWidget( DraftTimerWidget::SIZE_NORMAL, 10 );

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins( 1, 1, 1, 1 );
    layout->addStretch( 1 );
    layout->addWidget( mNameLabel );
    layout->addWidget( mPackQueueLabel );
    layout->addWidget( mDraftTimerWidget );

    setLayout( layout );

    if( sPackQueueBasePixmap == nullptr )
    {
        QSvgRenderer renderer( RESOURCE_SVG_CARD_BACK );
        QSize size = renderer.defaultSize();
        QSize scalingSize( std::numeric_limits<int>::max(), mDraftTimerWidget->height() );
        size.scale( scalingSize, Qt::KeepAspectRatio );
        sPackQueueBasePixmap = new QPixmap( size );
        QPainter painter( sPackQueueBasePixmap );
        renderer.render( &painter, sPackQueueBasePixmap->rect() );
    }

    setPackQueueSize( 0 );
}


void
PlayerStatusWidget::setPlayerActive( bool active )
{
    mNameLabel->setEnabled( active );
}


void
PlayerStatusWidget::setPackQueueSize( int queueSize )
{
    QPixmap pm( *sPackQueueBasePixmap );
    QPainter painter;
    painter.begin( &pm );
    painter.setPen( QPen( Qt::white ) );
    painter.drawText( pm.rect(), Qt::AlignCenter, QString::number( queueSize ) );
    painter.end();
    mPackQueueLabel->setPixmap( pm );
}


QString
PlayerStatusWidget::createNameLabelString( const QString& name )
{
    return QString( "<b>" + name + "</b>:" );
}
