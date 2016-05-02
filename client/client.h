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
class QProgressBar;
class QFrame;
class QTcpSocket;
class QAction;
class QStateMachine;
class QState;
class QTimer;
QT_END_NAMESPACE

class ClientSettings;
class AllSetsData;
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

class Client : public QMainWindow
{
    Q_OBJECT

public:

    Client( ClientSettings*        settings,
            AllSetsData*           allSetsData,
            ImageCache*            mImageCache,
            const Logging::Config& loggingConfig = Logging::Config(),
            QWidget*               parent = 0 );

private slots:

    void readFromServer();
    void handleSocketError(QAbstractSocket::SocketError socketError);

    void handleConnectAction();
    void handleDisconnectAction();
    void handleSaveDeckAction();
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

    virtual void closeEvent( QCloseEvent* event ) override;

    void connectToServer( const QString& host, int port );
    void disconnectFromServer();

    void handleMessageFromServer( const thicket::ServerToClientMsg& msg );
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
    AllSetsData* mAllSetsData;
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
    QAction* mSaveDeckAction;
    QAction* mAboutAction;

    ConnectDialog* mConnectDialog;
    CreateRoomDialog* mCreateRoomDialog;

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
    int mCurrentRound;
    std::shared_ptr<RoomConfigAdapter> mRoomConfigAdapter;

    CardZoneType mDraftedCardDestZone;
    std::string mCreatedRoomPassword;

    Logging::Config mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};


//
// Subclasses of internal widgets with minor overrides for behavior.
// Would be nested classes but the Q_OBJECT macro doesn't work.
//

#include <QLabel>
class ImageLoader;

class Client_CardLabel : public QLabel
{
    Q_OBJECT

public:
    explicit Client_CardLabel( int muid, ImageLoaderFactory* imageLoaderFactory, QWidget* parent = 0 );
    void loadImage();

protected:
    virtual void enterEvent(QEvent* event) override;

private slots:
    void handleImageLoaded( int multiverseId, const QImage &image );

private:
    const int                 mMuid;
    ImageLoaderFactory* const mImageLoaderFactory;
    ImageLoader*              mImageLoader;
    QString                   mToolTipStr;
};


#endif
