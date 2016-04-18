#include "BasicLandControlWidget.h"

#include <QSpinBox>
#include <QHBoxLayout>
#include "SizedSvgWidget.h"

static const std::map<BasicLandType,QString> sSvgResourceMap = {
    { BASIC_LAND_PLAINS,   ":/white-mana-symbol.svg" },
    { BASIC_LAND_ISLAND,   ":/blue-mana-symbol.svg"  },
    { BASIC_LAND_SWAMP,    ":/black-mana-symbol.svg" },
    { BASIC_LAND_MOUNTAIN, ":/red-mana-symbol.svg"   },
    { BASIC_LAND_FOREST,   ":/green-mana-symbol.svg" } };

static const QSize BASIC_LAND_MANA_SYMBOL_SIZE( 15,15 );

BasicLandControlWidget::BasicLandControlWidget( QWidget *parent )
  : QWidget( parent )
{
    // This function pointer is required for below connections because
    // QSpinBox::valueChanged is overloaded and the compiler can't
    // disambiguate the different overloads when connecting signals.
    void (QSpinBox:: *valueChangedSignal)(int) = &QSpinBox::valueChanged;

    // Add each basic land widget.
    QHBoxLayout *basicLandLayout = new QHBoxLayout();
    basicLandLayout->setContentsMargins( 0, 0, 0, 0 );
    for( auto basic : gBasicLandTypeArray )
    {
        // Put space before every item but the first.
        if( basic != gBasicLandTypeArray[0] )
        {
            basicLandLayout->addSpacing( 10 );
        }

        SizedSvgWidget *manaWidget = new SizedSvgWidget( BASIC_LAND_MANA_SYMBOL_SIZE );
        manaWidget->load( sSvgResourceMap.at( basic ) );
        basicLandLayout->addWidget( manaWidget );

        mBasicLandSpinBoxMap[basic] = new QSpinBox();

        basicLandLayout->addWidget( mBasicLandSpinBoxMap[basic] );
        connect( mBasicLandSpinBoxMap[basic], valueChangedSignal,
                 [this, basic] (int qty)
                 {
                     mBasicLandQtys.setQuantity( basic, qty );
                     emit basicLandQuantitiesUpdate( mBasicLandQtys );
                 });
    }

    setLayout( basicLandLayout );
}


void
BasicLandControlWidget::decrementBasicLandQuantity( const BasicLandType& basic )
{
    int currentQty = mBasicLandQtys.getQuantity( basic );
    if( currentQty > 0 )
    {
        int newQty = currentQty - 1;
        mBasicLandQtys.setQuantity( basic, newQty );
        mBasicLandSpinBoxMap[basic]->setValue( newQty );
    }
}


void
BasicLandControlWidget::setBasicLandQuantities( const BasicLandQuantities& basicLandQtys )
{
    // Make sure the setting will be new.
    if( mBasicLandQtys == basicLandQtys ) return;

    mBasicLandQtys = basicLandQtys;

    // Update each spinbox.  Disable signaling and emit a single setting at the end.
    for( auto basic : gBasicLandTypeArray )
    {
        bool val = mBasicLandSpinBoxMap[basic]->blockSignals( true );
        mBasicLandSpinBoxMap[basic]->setValue( mBasicLandQtys.getQuantity( basic ) );
        mBasicLandSpinBoxMap[basic]->blockSignals( val );
    }

    emit basicLandQuantitiesUpdate( mBasicLandQtys );
}
