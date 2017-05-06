#include "TickerPostRoundTimerWidget.h"

#include <QBoxLayout>
#include <QLabel>

#include "qtutils_widget.h"
#include "RoomStateAccumulator.h"


TickerPostRoundTimerWidget::TickerPostRoundTimerWidget( int tickerHeight, QWidget* parent )
  : TickerChildWidget( tickerHeight, parent )
{
    mLabel = new QLabel();
    mLabel->setContentsMargins( 0, 0, 0, 0 );
    mLayout->addWidget( mLabel );
}


void
TickerPostRoundTimerWidget::setSecondsRemaining( int secs )
{
    mLabel->setText( tr("Waiting for next round: ") + QString::number( secs ) );
    adjustSize();
}
