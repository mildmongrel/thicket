#ifndef SERVERVIEWWIDGET_H
#define SERVERVIEWWIDGET_H

#include <QWidget>
#include <QMap>
#include <vector>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QListWidget;
class QTreeWidget;
class QTextBrowser;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

class RoomConfigAdapter;

#include "Logging.h"

// This is the widget that occupies the server tab of the client window.
class ServerViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ServerViewWidget( const Logging::Config&  loggingConfig = Logging::Config(),
                               QWidget*                parent = 0 );

    void setAnnouncements( const QString& text );

    void addRoom( const std::shared_ptr<RoomConfigAdapter>& roomConfigAdapter );

    // Update player count in a room.
    void updateRoomPlayerCount( int roomId, int playerCount );

    // Remove a room.
    void removeRoom( int roomId );

    void clearRooms();

    void enableJoinRoom( bool enable );
    void enableCreateRoom( bool enable );

    // User control.
    void addUser( const QString& name );
    void removeUser( const QString& name );
    void clearUsers();

    void addChatMessage( const QString& user, const QString& message );
    void clearChatMessages();

signals:

    // User requested to join a room.
    void joinRoomRequest( int roomId, const QString& password );

    // User requested to create a room.
    void createRoomRequest();

    // User generated a chat message.
    void chatMessageGenerated( const QString& message );

private slots:

    void handleRoomTreeSelectionChanged();
    void handleRoomTreeCustomContextMenu( const QPoint& point );
    void handleJoinRoomButtonClicked();
    void handleCreateRoomButtonClicked();
    void handleChatReturnPressed();

private:

    void processRoomTreeSelection();
    void tryJoinRoom();

    QTextEdit* mAnnouncements;

    QTreeWidget*  mRoomTreeWidget;
    QMap<int,int> mRoomTreeRowToRoomIdMap;

    struct RoomData
    {
        unsigned int chairCount;
        bool         passwordProtected;
        int          treeRowIndex;
    };
    QMap<int,RoomData> mRoomIdToRoomDataMap;

    QPushButton* mJoinRoomButton;
    QPushButton* mCreateRoomButton;

    QListWidget* mUsersListWidget;

    QTextBrowser* mChatTextBrowser;
    QLineEdit*    mChatLineEdit;

    bool mJoinRoomEnabled;

    int mSelectedRoomId;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};


#endif  // SERVERVIEWWIDGET_H
