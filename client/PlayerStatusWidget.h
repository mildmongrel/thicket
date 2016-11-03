#ifndef PLAYERSTATUSWIDGET_H
#define PLAYERSTATUSWIDGET_H

#include <QWidget>
#include <QLabel>

#include "DraftTimerWidget.h"

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class PlayerStatusWidget : public QWidget
{
    Q_OBJECT

public:

    PlayerStatusWidget( const QString& name = "", QWidget* parent = 0 );

    void setName( const QString& name )
    {
        mNameLabel->setText( createNameLabelString( name ) );
        updateGeometry();
    }

    // Set true for active, false for departed.
    void setPlayerActive( bool active );

    void setPackQueueSize( int size );

    void setTimeRemaining( int time )
    {
        mDraftTimerWidget->setCount( time );
    }

private:

    static QString createNameLabelString( const QString& name );

    QLabel*           mNameLabel;
    QLabel*           mPackQueueLabel;
    DraftTimerWidget* mDraftTimerWidget;
};

#endif  // PLAYERSTATUSWIDGET_H

