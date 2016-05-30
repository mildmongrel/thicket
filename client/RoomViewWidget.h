#ifndef ROOMVIEWWIDGET_H
#define ROOMVIEWWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QTreeWidget;
class QPushButton;
class QLineEdit;
class QListWidget;
QT_END_NAMESPACE

class RoomConfigAdapter;

#include "Logging.h"

// This is the widget that occupies the room tab of the client window.
class RoomViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RoomViewWidget( const Logging::Config&  loggingConfig = Logging::Config(),
                             QWidget*                parent = 0 );

    void setRoomConfig( const std::shared_ptr<RoomConfigAdapter>& roomConfig );

    void setChairCount( int chairCount );

    void setPlayerInfo( int chairIndex, const QString& name, bool isBot, const QString& state );

    void setPlayerCockatriceHash( int chairIndex, const QString& hash );

    void clearPlayers();
    void reset();

    void addChatMessage( const QString& user, const QString& message );

signals:

    // User has updated their ready status.
    void readyUpdate( int ready );

    // User wants to leave the room.
    void leave();

    // User generated a chat message.
    void chatMessageGenerated( const QString& message );

private slots:

    void handleReadyButtonToggled( bool ready );
    void handleLeaveButtonClicked();
    void handleChatReturnPressed();

private:

    QLabel*       mRoomTitleLabel;
    QLabel*       mRoomDescLabel;
    QTreeWidget*  mPlayersTreeWidget;
    QPushButton*  mReadyButton;
    QPushButton*  mLeaveButton;

    QListWidget* mChatListWidget;
    QLineEdit*   mChatLineEdit;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};


#endif  // ROOMVIEWWIDGET_H
