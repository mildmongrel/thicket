#include "DraftTimerWidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QVariant>
#include <QStyle>

// The default background is white to hide the white corners on JPG cards
// returned by gatherer. 
const static QString gStyleSheet =
    "QLabel { border-radius: 4px; background-color: lightgray; color: black; }"
    "QLabel[alert=\"true\"] { background-color: #FF2828; color: white; }";
const static QString gLargeStyleSheet =
    "QLabel { font: bold; border-radius: 4px; background-color: lightgray; color: black; }"
    "QLabel[alert=\"true\"] { background-color: #FF2828; color: white; }";


DraftTimerWidget::DraftTimerWidget( SizeType size, int alertThreshold, QWidget* parent )
  : QLabel( parent ),
    mAlertThreshold( alertThreshold ),
    mAlerted( false )
{
    setAlignment( Qt::AlignCenter );

    setStyleSheet( size == SIZE_LARGE ? gLargeStyleSheet : gStyleSheet );

    QFont fnt = font();
    if( fnt.pointSize() > -1 )
    {
        fnt.setPointSize( fnt.pointSize() + (size == SIZE_LARGE ? 4 : 0) );
    }
    else
    {
        fnt.setPixelSize( fnt.pixelSize() + (size == SIZE_LARGE ? 4 : 0) );
    }
    setFont( fnt );
    QFontMetrics fm( fnt );

    setFixedWidth( fm.width( "0000" ) );
    setFixedHeight( fm.height() + (size == SIZE_LARGE ? 4 : 2 ) );
}
 

void
DraftTimerWidget::setCount( int count )
{
    // Set text and alerting status.
    bool alert = false;
    if( count >= 0 )
    {
        setText( QString::number( count ) );

        // Alert if alerting threshold enabled and count is equal or below.
        alert = (mAlertThreshold >= 0) && (count <= mAlertThreshold);
    }
    else
    {
        setText( QString() );
    }

    // Only update if a change in status has occurred; updating is expensive.
    if( (alert && !mAlerted) || (!alert && mAlerted) )
    {
        // Change the property, which will allow a different look according
        // to the stylesheet setup.  Need to take some special actions for
        // this to work: https://wiki.qt.io/Dynamic_Properties_and_Stylesheets
        setProperty( "alert", alert ? "true" : "false" );
        style()->unpolish( this );
        style()->polish( this );
        update();
    }

    mAlerted = alert;
}
