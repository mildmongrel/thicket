#include "PlayerStatusWidget.h"

#include <QHBoxLayout>
#include <QSvgRenderer>
#include <QPainter>

#include "CapsuleIndicator.h"

static const int ALERT_TIME_THRESHOLD = 10;

PlayerStatusWidget::PlayerStatusWidget( int height, QWidget* parent )
  : QWidget( parent ),
    mDraftAlert( false )
{
    mNameLabel = new QLabel();

    mQueuedPacksCapsule = new CapsuleIndicator( CapsuleIndicator::STYLE_MICRO, height, this );
    mQueuedPacksCapsule->setToolTip( tr("Packs queued for selection") );
    mQueuedPacksCapsule->setBorderBold( false );

    mTimeRemainingCapsule = new CapsuleIndicator( CapsuleIndicator::STYLE_MICRO, height, this );
    mTimeRemainingCapsule->setToolTip( tr("Time remaining to select a card") );
    mTimeRemainingCapsule->setBorderBold( false );

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins( 1, 1, 1, 1 );
    layout->addStretch( 1 );
    layout->addWidget( mNameLabel );
    layout->addWidget( mQueuedPacksCapsule );
    layout->addWidget( mTimeRemainingCapsule );
    setLayout( layout );

    // Adjust spacing after layout is set on widget otherwise it's not ready.
    layout->setSpacing( layout->spacing() / 2 );

    updateCapsulesLookAndFeel();

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
    mQueuedPacksCapsule->setValueText( (queueSize >= 0) ? QString::number( queueSize )
                                                        : QString() );
}


void
PlayerStatusWidget::setTimeRemaining( int time )
{
    mTimeRemainingCapsule->setValueText( (time >= 0) ? QString::number( time )
                                                     : QString() );
    bool oldDraftAlert = mDraftAlert;
    mDraftAlert = (time >= 0) && (time <= ALERT_TIME_THRESHOLD);
    if( mDraftAlert != oldDraftAlert ) updateCapsulesLookAndFeel();
}


QString
PlayerStatusWidget::createNameLabelString( const QString& name )
{
    return QString( "<b>" + name + "</b>:" );
}

void
PlayerStatusWidget::updateCapsulesLookAndFeel()
{
    if( !mQueuedPacksCapsule ) return;
    if( !mTimeRemainingCapsule ) return;

    QColor packsForeground( mDraftAlert ? 0xc0c0c0 : 0x404040 );
    QColor packsBackground( mDraftAlert ? 0xff2828 : 0xd0d0d0 );
    QColor timeForeground( mDraftAlert ? 0xffffff : 0x404040 );
    QColor timeBackground( mDraftAlert ? 0xff2828 : 0xaed581 );
    QColor border( mDraftAlert ? 0xff2828 : 0x404040 );

    mQueuedPacksCapsule->setBorderColor( border );
    mQueuedPacksCapsule->setBackgroundColor( packsBackground );
    mQueuedPacksCapsule->setTextColor( packsForeground );

    mTimeRemainingCapsule->setBorderColor( border );
    mTimeRemainingCapsule->setBackgroundColor( timeBackground );
    mTimeRemainingCapsule->setTextColor( timeForeground );
}

