#include "TickerChildWidget.h"

#include <QBoxLayout>

TickerChildWidget::TickerChildWidget( int tickerHeight, QWidget* parent )
  : QWidget( parent ),
    mTickerHeight( tickerHeight ),
    mLayout( 0 )
{
    mLayout = new QHBoxLayout( this );

    QMargins margins = mLayout->contentsMargins();
    margins.setBottom( 0 );
    margins.setTop( 0 );
    mLayout->setContentsMargins( margins );

    // Adding a strut guarantees height so that an 'empty' ticker widget
    // will be centered within the ticker.  When the layout is modified,
    // the ticker will have properly placed this widget.
    mLayout->addStrut( mTickerHeight );
}


TickerChildWidget::~TickerChildWidget()
{}
