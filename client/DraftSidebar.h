#ifndef DRAFTSIDEBAR_H
#define DRAFTSIDEBAR_H

#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QTextEdit;
QT_END_NAMESPACE

#include "Logging.h"

class DraftTimerWidget;
class RoomConfigAdapter;

class DraftSidebar : public QStackedWidget
{
    Q_OBJECT

public:

    enum MessageLevel
    {
        MESSAGE_LEVEL_LOW,
        MESSAGE_LEVEL_NORMAL,
        MESSAGE_LEVEL_HIGH
    };

    DraftSidebar( const Logging::Config& loggingConfig = Logging::Config(),
                  QWidget*               parent = 0 );

    void addRoomJoinMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig, bool rejoin );
    void addRoomLeaveMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig );
    void addCardSelectMessage( const QString& name, bool autoSelected );
    void addChatMessage( const QString& user, const QString& message );
    void addGameMessage( const QString& message, MessageLevel level = MESSAGE_LEVEL_NORMAL );

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

    void updateUnreadChatIndicator();

    QWidget* mExpandedWidget;
    QWidget* mCompactWidget;

    QTextEdit* mChatView;

    int     mUnreadChatMessages;
    QLabel* mCompactChatLabel;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // DRAFTSIDEBAR_H

