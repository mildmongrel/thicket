#ifndef DRAFTSIDEBAR_H
#define DRAFTSIDEBAR_H

#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QTextEdit;
QT_END_NAMESPACE

#include "Logging.h"

class DraftTimerWidget;
class CapsuleIndicator;
class RoomConfigAdapter;

class DraftSidebar : public QStackedWidget
{
    Q_OBJECT

public:

    DraftSidebar( const Logging::Config& loggingConfig = Logging::Config(),
                  QWidget*               parent = 0 );

    // Set the current room configuration.
    void setRoomConfig( const std::shared_ptr<RoomConfigAdapter>& roomConfig );

    void setDraftQueuedPacks( int packs );
    void setDraftTimeRemaining( int time );

    // Any time less than this and the timer is in alert mode.
    void setDraftTimeRemainingAlertThreshold( int time );

    void addChatMessage( const QString& user, const QString& message );

    bool isCompacted();

    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;

signals:
    void chatMessageComposed( const QString& str );
    void compacted();
    void expanded();

protected:
    virtual void resizeEvent( QResizeEvent *event ) override;

private:

    enum CapsuleModeType
    {
        CAPSULE_MODE_INACTIVE,
        CAPSULE_MODE_NORMAL,
        CAPSULE_MODE_ALERTED
    };

    void updateTimeRemainingCapsules();
    void setTimeRemainingCapsulesLookAndFeel( CapsuleModeType );
 
    void setQueuedPacksCapsulesLookAndFeel( CapsuleModeType );

    void updateUnreadChatIndicator();

    QWidget* mExpandedWidget;
    QWidget* mCompactWidget;

    QTextEdit* mChatView;

    CapsuleIndicator* mExpandedTimeRemainingCapsule;
    CapsuleIndicator* mExpandedQueuedPacksCapsule;
    CapsuleIndicator* mCompactTimeRemainingCapsule;
    CapsuleIndicator* mCompactQueuedPacksCapsule;


    CapsuleModeType mTimeRemainingCapsuleMode;
    int mTimeRemaining;
    int mTimeRemainingAlertThreshold;

    CapsuleModeType mQueuedPacksCapsuleMode;
    int mQueuedPacks;

    int     mUnreadChatMessages;
    QLabel* mCompactChatLabel;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // DRAFTSIDEBAR_H

