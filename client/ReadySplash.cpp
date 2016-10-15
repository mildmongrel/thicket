#include "ReadySplash.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QLabel>

#include "SizedSvgWidget.h"

static const QString RESOURCE_SVG_CANCEL_BRIGHT( ":/cancel-bright.svg" );
static const QString RESOURCE_SVG_CANCEL_DIM( ":/cancel-dim.svg" );
static const QString RESOURCE_SVG_APPROVE_BRIGHT( ":/approve-bright.svg" );


ReadySplash::ReadySplash( QWidget* parent )
  : QWidget( parent )
{
    setWindowFlags( Qt::SplashScreen );

    QVBoxLayout* layout = new QVBoxLayout( this );

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins( 0, 0, 0, 0 );

    const QString READY_TEXT( tr("Waiting for all players to be ready...") );
    const QString NOT_READY_TEXT( tr("Select \"Ready\" when you are ready to draft") );

    QLabel *label = new QLabel( NOT_READY_TEXT );
    label->setAlignment( Qt::AlignCenter );

    QPushButton* readyButton = new QPushButton( tr("Ready") );
    readyButton->setCheckable( true );
    readyButton->setAutoExclusive( true );

    QPushButton* notReadyButton = new QPushButton( tr("Not Ready") );
    notReadyButton->setCheckable( true );
    notReadyButton->setAutoExclusive( true );
    notReadyButton->toggle();

    SizedSvgWidget* readyIndWidget = new SizedSvgWidget( QSize( readyButton->sizeHint().height(), readyButton->sizeHint().height() ) );
    readyIndWidget->load( RESOURCE_SVG_CANCEL_BRIGHT );

    // Timer to blink when not ready.
    QTimer *timer = new QTimer(this);
    connect( timer, &QTimer::timeout, this, [=]() {
        if( isVisible() && notReadyButton->isChecked() ) {
            mBlinkState = !mBlinkState;
            readyIndWidget->load( mBlinkState ? RESOURCE_SVG_CANCEL_BRIGHT : RESOURCE_SVG_CANCEL_DIM );
        }
    } );
    mBlinkState = true;
    timer->start(500);

    connect( readyButton, &QPushButton::toggled, this, [=](bool isReady) {
            if( isReady ) {
                label->setText( READY_TEXT );
                readyIndWidget->load( RESOURCE_SVG_APPROVE_BRIGHT );
                timer->stop();
            } else {
                label->setText( NOT_READY_TEXT );
                readyIndWidget->load( RESOURCE_SVG_CANCEL_BRIGHT );
                mBlinkState = true;
                timer->start( 500 );
            }
            emit ready( isReady );
        } );

    buttonLayout->addStretch();
    buttonLayout->addWidget( readyButton );
    buttonLayout->addSpacing( 15 );
    buttonLayout->addWidget( readyIndWidget );
    buttonLayout->addSpacing( 15 );
    buttonLayout->addWidget( notReadyButton );
    buttonLayout->addStretch();

    layout->addSpacing( 15 );
    layout->addWidget( label );
    layout->addLayout( buttonLayout );
    layout->addSpacing( 15 );
}

