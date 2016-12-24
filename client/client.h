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
class QAction;
class QStateMachine;
class QState;
class QTimer;
class QDialog;
class QSplitter;
QT_END_NAMESPACE

class ClientToastOverlay;
class ClientSettings;
class AllSetsUpdater;
class ImageCache;
class ImageLoaderFactory;
class CommanderPane;
class TickerWidget;
class ServerViewWidget;
class ConnectDialog;
class CreateRoomDialog;
class RoomConfigAdapter;
class DraftSidebar;
class ReadySplash;
class TickerPlayerReadyWidget;
class TickerPlayerHashesWidget;
class TickerPlayerStatusWidget;
class ServerConnection;

#include "messages.pb.h"
#include "Logging.h"
#include "clienttypes.h"
#include "BasicLandCardDataMap.h"
#include "BasicLandQuantities.h"
#include "Decklist.h"
#include "SimpleVersion.h"
#include "RoomStateAccumulator.h"

class Client : public QMainWindow
{
    Q_OBJECT

public:

    Client( ClientSettings*             settings,
            const AllSetsDataSharedPtr& allSetsData,
            AllSetsUpdater*             allSetsUpdater,
            ImageCache*                 mImageCache,
            const Logging::Config&      loggingConfig = Logging::Config(),
            QWidget*                    parent = 0 );

    virtual ~Client();

public slots:
    void updateAllSetsData( const AllSetsDataSharedPtr& allSetsDataSharedPtr );

private slots:

    void handleMessageFromServer( const proto::ServerToClientMsg& msg );
    void handleSocketError(QAbstractSocket::SocketError socketError);

    void handleConnectAction();
    void handleDisconnectAction();
    void handleDeckStatsAction();
    void handleSaveDeckAction();
    void handleCheckClientUpdateAction();
    void handleUpdateCardsAction();
    void handleAboutAction();

    void handleCardZoneMoveRequest( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData, const CardZoneType& destCardZone );
    void handleCardZoneMoveAllRequest( const CardZoneType& srcCardZone, const CardZoneType& destCardZone );
    void handleCardPreselected( const CardDataSharedPtr& cardData );
    void handleCardSelected( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData );
    void handleBasicLandQuantitiesUpdate( const CardZoneType& srcCardZone, const BasicLandQuantities& qtys );

    // Handling for signals from ServerViewWidget.
    void handleJoinRoomRequest( int roomId, const QString& password );
    void handleCreateRoomRequest();
    void handleServerChatMessageGenerated( const QString& text );

    // Handling for room-related signals.
    void handleReadyUpdate( bool ready );
    void handleRoomLeave();
    void handleRoomChatMessageGenerated( const QString& text );

    void handleKeepAliveTimerTimeout();

protected:

    virtual void moveEvent( QMoveEvent* event ) override;
    virtual void resizeEvent( QResizeEvent* event ) override;
    virtual void closeEvent( QCloseEvent* event ) override;

private:

    void initStateMachine();

    // Create card data shared pointer from set code and name.  Internally
    // handles cases of null AllSetsData or unknown cards in AllSetsData.
    CardDataSharedPtr createCardData( const std::string& setCode, const std::string& name );

    void connectToServer( const QString& host, int port );
    void disconnectFromServer();

    void processMessageFromServer( const proto::LoginRsp& rsp );
    void processMessageFromServer( const proto::RoomCapabilitiesInd& ind );
    void processMessageFromServer( const proto::JoinRoomSuccessRspInd& rspInd );
    void processMessageFromServer( const proto::PlayerInventoryInd& ind );
    void processMessageFromServer( const proto::RoomChairsInfoInd& ind );
    void processMessageFromServer( const proto::RoomChairsDeckInfoInd& ind );
    void processMessageFromServer( const proto::RoomStageInd& ind );

    void addPlayerInventoryUpdateDraftedCardMove( proto::PlayerInventoryUpdateInd* ind,
                                                  const CardDataSharedPtr&         cardData,
                                                  const CardZoneType&              srcCardZone,
                                                  const CardZoneType&              destCardZone );

    void processCardSelected( const proto::Card& card, bool autoSelected );
    void processCardZoneMoveRequest( const CardDataSharedPtr& cardData, const CardZoneType& srcCardZone, const CardZoneType& destCardZone );
    void processCardListChanged( const CardZoneType& cardZone );

    void clearTicker();

    Decklist buildDecklist();

signals:

    // Network connection event signals; these are emitted internally and
    // slotted into the network connection state machine.
    void eventNetworkAvailable();
    void eventClientUpdateChecked();
    void eventConnecting();
    void eventConnectingAborted();
    void eventConnectionError();
    void eventLoggedIn();
    void eventJoinedRoom();
    void eventDepartedRoom();
    void eventDisconnecting();

private:

    ClientSettings*      mSettings;
    AllSetsDataSharedPtr mAllSetsData;
    AllSetsUpdater*      mAllSetsUpdater;
    ImageCache*          mImageCache;
    ImageLoaderFactory*  mImageLoaderFactory;

    // Network connection state machine objects.
    QStateMachine* mStateMachine;
    QState* mStateInitializing;
    QState* mStateUpdating;
    QState* mStateUpdatingClient;     // child of Updating
    QState* mStateUpdatingAllSets;    // child of Updating
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

    DraftSidebar* mDraftSidebar;

    CommanderPane* mLeftCommanderPane;
    CommanderPane* mRightCommanderPane;

    QSplitter* mSplitter;

    ClientToastOverlay* mToastOverlay;

    TickerWidget* mTickerWidget;
    TickerPlayerReadyWidget* mTickerPlayerReadyWidget;
    TickerPlayerStatusWidget* mTickerPlayerStatusWidget;
    TickerPlayerHashesWidget* mTickerPlayerHashesWidget;

    QLabel* mDraftStatusLabel;
    QLabel* mConnectionStatusLabel;

    QAction* mConnectAction;
    QAction* mDisconnectAction;
    QAction* mLeaveRoomAction;

    ConnectDialog* mConnectDialog;
    CreateRoomDialog* mCreateRoomDialog;
    QMessageBox* mAlertMessageBox;

    ReadySplash* mReadySplash;

    // These are the master lists of cards in each zone.  The commander tabs
    // are populated from these.
    QList<CardDataSharedPtr> mCardsList[CARD_ZONE_TYPE_COUNT];

    // The server can send empty or unknown set codes to the client.  When
    // that happens we try to find a valid set code so we can fetch images
    // and reference card stats.  But responses to the server require using
    // its original set code, so a map holds those associations.
    // The map is held as a shared pointer because the management of the
    // map can extend beyond the lifetime of the Client object and weak
    // pointers are used to avoid dereferencing it after Client is destroyed.
    using CardServerSetCodeMap = QMap<CardData*,std::string>;
    std::shared_ptr<CardServerSetCodeMap> mCardServerSetCodeMap;

    BasicLandCardDataMap mBasicLandCardDataMap;
    std::map<CardZoneType,BasicLandQuantities> mBasicLandQtysMap;

    ServerConnection* mServerConn;
    quint16 mIncomingMsgHeader;

    QTimer* mKeepAliveTimer;

    QString mUserName;
    QString mServerName;
    QString mServerVersion;

    int mChairIndex;
    int currentPackId;
    bool mRoundTimerEnabled;
    bool mRoomStageRunning;
    bool mCurrentRound;
    std::shared_ptr<RoomConfigAdapter> mRoomConfigAdapter;
    RoomStateAccumulator mRoomStateAccumulator;

    CardZoneType mDraftedCardDestZone;
    std::string mCreatedRoomPassword;

    bool mUnsavedChanges;

    Logging::Config mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
