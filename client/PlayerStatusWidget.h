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

    void setName( const QString& name );

    // Set true for active, false for departed.
    void setPlayerActive( bool active );

    // Set true if player is currently drafting cards, false if waiting.
    void setPlayerDrafting( bool active );

    void setPackQueueSize( int size );
    void setPackQueueSizeVisible( bool visible );

    void setTimeRemaining( int time );
    void setTimeRemainingVisible( bool visible );

private:

    void updateNameLabel();

    void updateCapsulesLookAndFeel();

    QString           mName;
    QLabel*           mNameLabel;
    CapsuleIndicator* mQueuedPacksCapsule;
    CapsuleIndicator* mTimeRemainingCapsule;
    bool              mDraftAlert;
    bool              mDrafting;
};

#endif  // PLAYERSTATUSWIDGET_H

