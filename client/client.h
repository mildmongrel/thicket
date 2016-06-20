#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QAbstractSocket>
#include <QListWidget>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTabWidget;
class QLabel;
class QHBoxLayout;
class QGridLayout;
class QMessageBox;
class QFrame;
class QTcpSocket;
class QAction;
class QStateMachine;
class QState;
class QTimer;
class QDialog;
QT_END_NAMESPACE

class ClientSettings;
class AllSetsUpdateDialog;
class ImageCache;
class ImageLoaderFactory;
class PlayerStatusWidget;
class SizedSvgWidget;
class CommanderPane;
class TickerWidget;
class ServerViewWidget;
class RoomViewWidget;
class ConnectDialog;
class CreateRoomDialog;
class RoomConfigAdapter;

#include "messages.pb.h"
#include "Logging.h"
#include "clienttypes.h"
#include "BasicLandCardDataMap.h"
#include "BasicLandQuantities.h"
#include "Decklist.h"
#include "SimpleVersion.h"

class Client : public QMainWindow
{
    Q_OBJECT

public:

    Client( ClientSettings*             settings,
            const AllSetsDataSharedPtr& allSetsData,
            AllSetsUpdateDialog*        allSetsUpdateDialog,
            ImageCache*                 mImageCache,
            const Logging::Config&      loggingConfig = Logging::Config(),
            QWidget*                    parent = 0 );

public slots:
    void updateAllSetsData( const AllSetsDataSharedPtr& allSetsDataSharedPtr );

private slots:

    void readFromServer();
    void handleSocketError(QAbstractSocket::SocketError socketError);

    void handleConnectAction();
    void handleDisconnectAction();
    void handleDeckStatsAction();
    void handleSaveDeckAction();
    void handleUpdateCardsAction();
    void handleAboutAction();

    void handleCardZoneMoveRequest( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData, const CardZoneType& destCardZone );
    void handleCardZoneMoveAllRequest( const CardZoneType& srcCardZone, const CardZoneType& destCardZone );
    void handleCardSelected( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData );
    void handleBasicLandQuantitiesUpdate( const CardZoneType& srcCardZone, const BasicLandQuantities& qtys );

    // Handling for signals from ServerViewWidget.
    void handleJoinRoomRequest( int roomId, const QString& password );
    void handleCreateRoomRequest();
    void handleServerChatMessageGenerated( const QString& text );

    // Handling for signals from RoomViewWidget.
    void handleReadyUpdate( bool ready );
    void handleRoomLeave();
    void handleRoomChatMessageGenerated( const QString& text );

    void handleKeepAliveTimerTimeout();

private:

    void initStateMachine();

    // Create card data shared pointer from set code and name.  Internally
    // handles cases of null AllSetsData or unknown cards in AllSetsData.
    CardDataSharedPtr createCardData( const std::string& setCode, const std::string& name );

    virtual void closeEvent( QCloseEvent* event ) override;

    void connectToServer( const QString& host, int port );
    void disconnectFromServer();

    void handleMessageFromServer( const thicket::ServerToClientMsg& msg );
    void processMessageFromServer( const thicket::LoginRsp& rsp );
    void processMessageFromServer( const thicket::RoomCapabilitiesInd& ind );
    void processMessageFromServer( const thicket::JoinRoomSuccessRspInd& rspInd );
    void processMessageFromServer( const thicket::PlayerInventoryInd& ind );
    void processMessageFromServer( const thicket::RoomChairsInfoInd& ind );
    void processMessageFromServer( const thicket::RoomChairsDeckInfoInd& ind );
    void processMessageFromServer( const thicket::RoomStageInd& ind );

    static void addPlayerInventoryUpdateDraftedCardMove( thicket::PlayerInventoryUpdateInd* ind,
                                                         const CardDataSharedPtr&           cardData,
                                                         const CardZoneType&                srcCardZone,
                                                         const CardZoneType&                destCardZone );

    bool sendProtoMsg( const thicket::ClientToServerMsg& protoMsg, QTcpSocket* tcpSocket );
    void processCardSelected( const thicket::Card& card, bool autoSelected );
    void processCardZoneMoveRequest( const CardDataSharedPtr& cardData, const CardZoneType& srcCardZone, const CardZoneType& destCardZone );
    void processCardListChanged( const CardZoneType& cardZone );

    void clearTicker();

    Decklist buildDecklist();

signals:

    // Network connection event signals; these are emitted internally and
    // slotted into the network connection state machine.
    void eventNetworkAvailable();
    void eventConnecting();
    void eventConnectingAborted();
    void eventConnectionError();
    void eventLoggedIn();
    void eventJoinedRoom();
    void eventDepartedRoom();
    void eventDisconnecting();

private:

    ClientSettings* mSettings;
    AllSetsDataSharedPtr mAllSetsData;
    AllSetsUpdateDialog* mAllSetsUpdateDialog;
    ImageCache* mImageCache;
    ImageLoaderFactory* mImageLoaderFactory;

    // Network connection state machine objects.
    QStateMachine* mStateMachine;
    QState* mStateInitializing;
    QState* mStateNetworkReady;
    QState* mStateDisconnected;
    QState* mStateConnecting;
    QState* mStateConnected;
    QState* mStateLoggedOut;          // child of Connected
    QState* mStateLoggedIn;           // child of Connected
    QState* mStateNotInRoom;          // child of LoggedIn
    QState* mStateInRoom;             // child of LoggedIn
    QState* mStateDisconnecting;

    // State information outside of connection state machine.
    bool mConnectionEstablished;

    SimpleVersion mServerProtoVersion;

    QTabWidget* mCentralTabWidget;
    QWidget* mDraftViewWidget;
    ServerViewWidget* mServerViewWidget;
    RoomViewWidget* mRoomViewWidget;

    CommanderPane* mLeftCommanderPane;
    CommanderPane* mRightCommanderPane;

    TickerWidget* mTickerWidget;

    QMap<int,PlayerStatusWidget*> mPlayerStatusWidgetMap;
    SizedSvgWidget* mPassDirLeftWidget;
    SizedSvgWidget* mPassDirRightWidget;
    QList<SizedSvgWidget*> mPassDirWidgetList;
    QGridLayout* mPlayerStatusLayout;

    QWidget* mTickerPlayerStatusWidget;
    QHBoxLayout* mTickerPlayerStatusLayout;

    QLabel* mDraftStatusLabel;
    QLabel* mConnectionStatusLabel;

    QAction* mConnectAction;
    QAction* mDisconnectAction;
    QAction* mLeaveRoomAction;

    ConnectDialog* mConnectDialog;
    CreateRoomDialog* mCreateRoomDialog;
    QMessageBox* mAlertMessageBox;

    // These are the master lists of cards in each zone.  The commander tabs
    // are populated from these.
    QList<CardDataSharedPtr> mCardsList[CARD_ZONE_TYPE_COUNT];

    BasicLandCardDataMap mBasicLandCardDataMap;
    std::map<CardZoneType,BasicLandQuantities> mBasicLandQtysMap;

    QTcpSocket* mTcpSocket;
    quint16 mIncomingMsgHeader;

    QTimer* mKeepAliveTimer;

    QString mUserName;
    QString mServerName;
    QString mServerVersion;

    int mChairIndex;
    int currentPackId;
    bool mRoundTimerEnabled;
    bool mRoomStageRunning;
    std::shared_ptr<RoomConfigAdapter> mRoomConfigAdapter;

    CardZoneType mDraftedCardDestZone;
    std::string mCreatedRoomPassword;

    bool mUnsavedChanges;

    Logging::Config mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
