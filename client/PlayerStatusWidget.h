#ifndef PLAYERSTATUSWIDGET_H
#define PLAYERSTATUSWIDGET_H

#include <QWidget>
#include <QLabel>

class CapsuleIndicator;

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class PlayerStatusWidget : public QWidget
{
    Q_OBJECT

public:

    PlayerStatusWidget( int height, QWidget* parent = 0 );

    void setName( const QString& name )
    {
        mNameLabel->setText( createNameLabelString( name ) );
        updateGeometry();
    }

    // Set true for active, false for departed.
    void setPlayerActive( bool active );

    void setPackQueueSize( int size );

    void setTimeRemaining( int time );

private:

    static QString createNameLabelString( const QString& name );

void updateCapsulesLookAndFeel();

    QLabel*           mNameLabel;
    CapsuleIndicator* mQueuedPacksCapsule;
    CapsuleIndicator* mTimeRemainingCapsule;
    bool              mDraftAlert;
};

#endif  // PLAYERSTATUSWIDGET_H

