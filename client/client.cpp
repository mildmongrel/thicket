#include "client.h"

#include <QtWidgets>
#include <QtNetwork>
#include <QHBoxLayout>
#include <QStateMachine>
#include <QTimer>

#include "version.h"
#include "qtutils_core.h"
#include "qtutils_widget.h"
#include "ClientSettings.h"
#include "AllSetsUpdateDialog.h"
#include "messages.pb.h"
#include "ImageCache.h"
#include "ImageLoaderFactory.h"
#include "AllSetsData.h"
#include "CommanderPane.h"
#include "CommanderPaneSettings.h"
#include "SizedSvgWidget.h"
#include "PlayerStatusWidget.h"
#include "TickerWidget.h"
#include "ServerViewWidget.h"
#include "RoomViewWidget.h"
#include "ConnectDialog.h"
#include "CreateRoomDialog.h"
#include "RoomConfigAdapter.h"
#include "Decklist.h"
#include "ProtoHelper.h"
#include "ClientProtoHelper.h"


// Keep-alive timer duration.
static const int KEEP_ALIVE_TIMER_SECS = 25;

// Pass direction SVG resources.
static const QString RESOURCE_SVG_ARROW_LEFT( ":/arrow-left.svg" );
static const QString RESOURCE_SVG_ARROW_RIGHT( ":/arrow-right.svg" );
static const QString RESOURCE_SVG_ARROW_CW_LEFT( ":/arrow-cw-left.svg" );
static const QString RESOURCE_SVG_ARROW_CCW_LEFT( ":/arrow-ccw-left.svg" );
static const QString RESOURCE_SVG_ARROW_CW_RIGHT( ":/arrow-cw-right.svg" );
static const QString RESOURCE_SVG_ARROW_CCW_RIGHT( ":/arrow-ccw-right.svg" );

// Helper function for logging protocol-type cards.
static std::ostream& operator<<( std::ostream& os, const thicket::Card& card )
{
    os << '[' << card.set_code() << ',' << card.name() << ']';
    return os;
}

Client::Client( ClientSettings*             settings,
                const AllSetsDataSharedPtr& allSetsData,
                AllSetsUpdateDialog*        allSetsUpdateDialog,
                ImageCache*                 imageCache,
                const Logging::Config&      loggingConfig,
                QWidget*                    parent )
:   QMainWindow( parent ),
    mSettings( settings ),
    mAllSetsData( allSetsData ),
    mAllSetsUpdateDialog( allSetsUpdateDialog ),
    mImageCache( imageCache ),
    mConnectionEstablished( false ),
    mChairIndex( -1 ),
    mRoundTimerEnabled( false ),
    mDraftedCardDestZone( CARD_ZONE_MAIN ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    mImageLoaderFactory = new ImageLoaderFactory( imageCache,
            settings->getCardImageUrlTemplate(), this );

    mPlayerStatusLayout = new QGridLayout();
    mPlayerStatusLayout->setVerticalSpacing( 0 );

    mServerViewWidget = new ServerViewWidget( mLoggingConfig.createChildConfig( "serverview" ), this );
    connect( mServerViewWidget, &ServerViewWidget::joinRoomRequest, this, &Client::handleJoinRoomRequest );
    connect( mServerViewWidget, &ServerViewWidget::createRoomRequest, this, &Client::handleCreateRoomRequest );
    connect( mServerViewWidget, &ServerViewWidget::chatMessageGenerated, this, &Client::handleServerChatMessageGenerated );

    mRoomViewWidget = new RoomViewWidget( mLoggingConfig.createChildConfig( "roomview" ), this );
    connect( mRoomViewWidget, &RoomViewWidget::readyUpdate, this, &Client::handleReadyUpdate );
    connect( mRoomViewWidget, &RoomViewWidget::leave, this, &Client::handleRoomLeave );
    connect( mRoomViewWidget, &RoomViewWidget::chatMessageGenerated, this, &Client::handleRoomChatMessageGenerated );

    mLeftCommanderPane = new CommanderPane( CommanderPaneSettings( *mSettings, 0 ),
            { CARD_ZONE_DRAFT, CARD_ZONE_MAIN, CARD_ZONE_SIDEBOARD, CARD_ZONE_JUNK },
            mImageLoaderFactory, mLoggingConfig.createChildConfig( "LeftCmdrMain" ) );
    connect( mLeftCommanderPane, &CommanderPane::cardZoneMoveAllRequest,
             this,               &Client::handleCardZoneMoveAllRequest );
    connect(mLeftCommanderPane, SIGNAL(cardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)),
            this, SLOT(handleCardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)));
    connect(mLeftCommanderPane, SIGNAL(cardSelected(const CardZoneType&,const CardDataSharedPtr&)),
            this, SLOT(handleCardSelected(const CardZoneType&,const CardDataSharedPtr&)));
    connect(mLeftCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            this, SLOT(handleBasicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)));

    mRightCommanderPane = new CommanderPane( CommanderPaneSettings( *mSettings, 1 ),
            { CARD_ZONE_MAIN, CARD_ZONE_SIDEBOARD, CARD_ZONE_JUNK },
            mImageLoaderFactory, mLoggingConfig.createChildConfig( "RightCmdrMain" ) );
    connect( mRightCommanderPane, &CommanderPane::cardZoneMoveAllRequest,
             this,                &Client::handleCardZoneMoveAllRequest );
    connect(mRightCommanderPane, SIGNAL(cardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)),
            this, SLOT(handleCardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)));
    connect(mRightCommanderPane, SIGNAL(cardSelected(const CardZoneType&,const CardDataSharedPtr&)),
            this, SLOT(handleCardSelected(const CardZoneType&,const CardDataSharedPtr&)));
    connect(mRightCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            this, SLOT(handleBasicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)));

    // Wire basic land quantity update signals from one pane to another for auto-updating.
    connect(mRightCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            mLeftCommanderPane, SLOT(setBasicLandQuantities(const CardZoneType&,const BasicLandQuantities&)));
    connect(mLeftCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            mRightCommanderPane, SLOT(setBasicLandQuantities(const CardZoneType&,const BasicLandQuantities&)));

    // Create TCP socket and connect signals.  Note the connect and
    // disconnect signals are connected in the state machine.
    mTcpSocket = new QTcpSocket(this);
    connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(readFromServer()));
    connect(mTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(handleSocketError(QAbstractSocket::SocketError)));

    // Create keep-alive timer.
    mKeepAliveTimer = new QTimer( this );
    connect( mKeepAliveTimer, &QTimer::timeout, this, &Client::handleKeepAliveTimerTimeout );

    // Splitter provides draggable separator between two widgets.
    QSplitter *splitter = new QSplitter();
    splitter->addWidget( mLeftCommanderPane );
    splitter->addWidget( mRightCommanderPane );

    QLabel* tickerWelcomeWidget = new QLabel( "Welcome to Thicket" );

    mTickerPlayerStatusWidget = new QWidget();
    mTickerPlayerStatusLayout = new QHBoxLayout();
    // Setting the size constraint makes a HUGE difference when
    // adding/removing widgets to the layout; causes the widget to resize
    // properly within the tickerwidget.
    mTickerPlayerStatusLayout->setSizeConstraint( QLayout::SetFixedSize );
    QMargins margins = mTickerPlayerStatusLayout->contentsMargins();
    margins.setBottom( 0 );
    margins.setTop( 0 );
    mTickerPlayerStatusLayout->setContentsMargins( margins );
    mTickerPlayerStatusLayout->setSpacing( 15 );
    mTickerPlayerStatusWidget->setLayout( mTickerPlayerStatusLayout );

    mTickerWidget = new TickerWidget();
    PlayerStatusWidget tmpWidget;
    tmpWidget.adjustSize();
    mTickerWidget->setFixedHeight( tmpWidget.height() );
    mTickerWidget->addPermanentWidget( tickerWelcomeWidget );
    mTickerWidget->start();
 
    mDraftViewWidget = new QWidget();
    QVBoxLayout* draftViewLayout = new QVBoxLayout();
    draftViewLayout->addWidget( splitter );
    draftViewLayout->addWidget( mTickerWidget );
    mDraftViewWidget->setLayout( draftViewLayout );

    mCentralTabWidget = new QTabWidget();
    mCentralTabWidget->setTabPosition( QTabWidget::West );
    mCentralTabWidget->addTab( mDraftViewWidget, "Draft" );
    mCentralTabWidget->addTab( mServerViewWidget, "Server" );
    setCentralWidget( mCentralTabWidget );

    // --- MENU ACTIONS ---

    mConnectAction = new QAction(tr("&Connect..."), this);
    mConnectAction->setStatusTip(tr("Connect to server"));
    connect(mConnectAction, SIGNAL(triggered()), this, SLOT(handleConnectAction()));
    mConnectAction->setEnabled( false );

    mDisconnectAction = new QAction(tr("&Disconnect"), this);
    mDisconnectAction->setStatusTip(tr("Disconnect from server"));
    connect(mDisconnectAction, SIGNAL(triggered()), this, SLOT(handleDisconnectAction()));
    mDisconnectAction->setEnabled( false );

    mLeaveRoomAction = new QAction(tr("&Leave Room"), this);
    connect( mLeaveRoomAction, &QAction::triggered, this, &Client::handleRoomLeave );
    mLeaveRoomAction->setEnabled( false );

    mSaveDeckAction = new QAction(tr("&Save Deck..."), this);
    mSaveDeckAction->setStatusTip(tr("Save the current deck"));
    connect(mSaveDeckAction, SIGNAL(triggered()), this, SLOT(handleSaveDeckAction()));

    mUpdateCardsAction = new QAction(tr("&Update Card Data..."), this);
    mUpdateCardsAction->setStatusTip(tr("Update the card database"));
    connect(mUpdateCardsAction, SIGNAL(triggered()), this, SLOT(handleUpdateCardsAction()));

    mAboutAction = new QAction(tr("&About..."), this);
    mAboutAction->setStatusTip(tr("About the appication"));
    connect(mAboutAction, SIGNAL(triggered()), this, SLOT(handleAboutAction()));

    // --- MENU ---

    QMenu *thicketMenu = menuBar()->addMenu(tr("&Thicket"));
    thicketMenu->addAction(mUpdateCardsAction);

    QMenu *draftMenu = menuBar()->addMenu(tr("&Draft"));
    draftMenu->addAction(mConnectAction);
    draftMenu->addAction(mDisconnectAction);
    draftMenu->addAction(mLeaveRoomAction);
    draftMenu->addSeparator();
    draftMenu->addAction(mSaveDeckAction);

    QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(mAboutAction);

    // --- STATUS BAR ---

    mConnectionStatusLabel = new QLabel();
    statusBar()->addPermanentWidget( mConnectionStatusLabel );
    statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    statusBar()->addPermanentWidget(line);

    mDraftStatusLabel = new QLabel(tr("Draft not started"));
    statusBar()->addPermanentWidget( mDraftStatusLabel );

    setWindowTitle(tr("Thicket Client"));

    mConnectDialog = new ConnectDialog( mSettings->getConnectHost(),
                                        mSettings->getConnectPort(),
                                        mSettings->getConnectName(),
                                        mLoggingConfig.createChildConfig( "connectdialog" ),
                                        this );

    mCreateRoomDialog = new CreateRoomDialog( mLoggingConfig.createChildConfig( "createdialog" ),
                                              this );

    mAlertMessageBox = new QMessageBox( this );
    mAlertMessageBox->setWindowTitle( tr("Server Alert") );
    mAlertMessageBox->setWindowModality( Qt::NonModal );
    mAlertMessageBox->setIcon( QMessageBox::Warning );

    updateAllSetsData( allSetsData );

    initStateMachine();
}


void
Client::updateAllSetsData( const AllSetsDataSharedPtr& allSetsDataSharedPtr )
{
    mLogger->debug( "updating AllSetsData" );
    mAllSetsData = allSetsDataSharedPtr;

    // Set basic land card data to use card data from settings.
    for( auto basic : gBasicLandTypeArray )
    {
        CardData* cardData = nullptr;
        if( mAllSetsData )
        {
            cardData = mAllSetsData->createCardData( mSettings->getBasicLandMultiverseId( basic ) );
        }
        if( !cardData )
        {
            // Could not create normally, so use a simple placeholder.
            cardData = new SimpleCardData( stringify( basic ) );
        }
        mBasicLandCardDataMap.setCardData( basic, CardDataSharedPtr( cardData ) );
    }

    mLeftCommanderPane->setBasicLandCardDataMap( mBasicLandCardDataMap );
    mRightCommanderPane->setBasicLandCardDataMap( mBasicLandCardDataMap );
}


void
Client::initStateMachine()
{
    // Create outer states.
    mStateMachine = new QStateMachine( this );
    mStateInitializing = new QState();
    mStateNetworkReady = new QState();
    mStateDisconnected = new QState();
    mStateConnecting = new QState();
    mStateConnected = new QState();
    mStateDisconnecting = new QState();

    // Create mStateConnected substates.
    mStateLoggedOut = new QState( mStateConnected );
    mStateLoggedIn = new QState( mStateConnected );
    mStateConnected->setInitialState( mStateLoggedOut );

    // Create mStateLoggedIn substates.
    mStateNotInRoom = new QState( mStateLoggedIn );
    mStateInRoom = new QState( mStateLoggedIn );
    mStateLoggedIn->setInitialState( mStateNotInRoom );

    mStateMachine->addState( mStateInitializing );
    mStateMachine->addState( mStateNetworkReady );
    mStateMachine->addState( mStateDisconnected );
    mStateMachine->addState( mStateConnecting );
    mStateMachine->addState( mStateConnected );
    mStateMachine->addState( mStateDisconnecting );

    mStateMachine->setInitialState( mStateInitializing );

    mStateInitializing->addTransition(          this, SIGNAL( eventNetworkAvailable() ),      mStateNetworkReady          );
    mStateNetworkReady->addTransition(          this, SIGNAL( eventConnecting() ),            mStateConnecting            );
    mStateConnecting->addTransition(      mTcpSocket, SIGNAL( connected() ),                  mStateConnected             );
    mStateConnecting->addTransition(            this, SIGNAL( eventConnectingAborted() ),     mStateDisconnected          );
    mStateConnecting->addTransition(            this, SIGNAL( eventConnectionError() ),       mStateDisconnected          );
    mStateConnecting->addTransition(      mTcpSocket, SIGNAL( disconnected() ),               mStateDisconnected          );
    mStateLoggedOut->addTransition(             this, SIGNAL( eventLoggedIn() ),              mStateLoggedIn              );
    mStateNotInRoom->addTransition(             this, SIGNAL( eventJoinedRoom() ),            mStateInRoom                );
    mStateInRoom->addTransition(                this, SIGNAL( eventDepartedRoom() ),          mStateNotInRoom             );
    mStateConnected->addTransition(             this, SIGNAL( eventDisconnecting() ),         mStateDisconnecting         );
    mStateConnected->addTransition(       mTcpSocket, SIGNAL( disconnected() ),               mStateDisconnected          );
    mStateDisconnecting->addTransition(   mTcpSocket, SIGNAL( disconnected() ),               mStateDisconnected          );
    mStateDisconnected->addTransition(          this, SIGNAL( eventConnecting() ),            mStateConnecting            );

    connect( mStateInitializing, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered Initializing" );
                 mConnectionStatusLabel->setText( tr("Initializing...") );
                 
                 // Check if a network session is required.
                 QNetworkConfigurationManager manager;
                 if( manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired )
                 {
                     // If network session required, use the system default.
                     QNetworkConfiguration config = manager.defaultConfiguration();
     
                     QNetworkSession* networkSession = new QNetworkSession( config, this );
     
                     // Forward the session-opened signal to our own event signal.
                     connect( networkSession, SIGNAL(opened()), this, SIGNAL(eventNetworkAvailable()) );
                     mConnectionStatusLabel->setText( tr("Opening network session...") );
                     networkSession->open();
                 }
                 else
                 {
                     emit eventNetworkAvailable();
                 }
             });
    connect( mStateNetworkReady, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered NetworkReady" );
                 mConnectionStatusLabel->setText( tr("Ready") );
                 mConnectAction->setEnabled( true );
                 mDisconnectAction->setEnabled( false );

                 // If there isn't card data, ask the user if we should fetch it
                 if( !mAllSetsData )
                 {
                     QMessageBox::StandardButton response;
                     response = QMessageBox::question( this,
                             tr("No Card Data"),
                             tr("Card data not found.  Update now?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No );
                     if( response == QMessageBox::Yes )
                     {
                         handleUpdateCardsAction();
                     }
                 }

                 // Take action at startup as if user had initiated a connection.
                 handleConnectAction();
             });
    connect( mStateConnecting, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered Connecting" );
                 mConnectionEstablished = false;
                 mConnectionStatusLabel->setText( tr("Connecting...") );
                 mConnectAction->setEnabled( false );
                 mDisconnectAction->setEnabled( true );
             });
    connect( mStateConnected, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered Connected" );
                 mConnectionEstablished = true;
                 mConnectionStatusLabel->setText( tr("Connected") );
                 mConnectAction->setEnabled( false );
                 mDisconnectAction->setEnabled( true );
                 mKeepAliveTimer->start( KEEP_ALIVE_TIMER_SECS * 1000 );
             });
    connect( mStateLoggedOut, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered LoggedOut" );
             });
    connect( mStateLoggedIn, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered LoggedIn" );
                 mConnectionStatusLabel->setText( QString( "Connected as '" + mUserName + "' to server " + mServerName +
                         " (version " + mServerVersion + ")" ) );

                 // OK to join and create rooms now.
                 mServerViewWidget->enableJoinRoom( true );
                 mServerViewWidget->enableCreateRoom( true );

                 // Go to the server tab.
                 mCentralTabWidget->setCurrentWidget( mServerViewWidget );
             });
    connect( mStateNotInRoom, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered NotInRoom" );
             });
    connect( mStateInRoom, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered InRoom" );

                 // Clear out the ticker and display the player status widget.
                 clearTicker();
                 mTickerWidget->addPermanentWidget( mTickerPlayerStatusWidget );

                 // Enable leaving room.
                 mLeaveRoomAction->setEnabled( true );

                 // Can't join or create rooms once in a room.
                 mServerViewWidget->enableJoinRoom( false );
                 mServerViewWidget->enableCreateRoom( false );

                 // Reset the room view widget, create a new tab for it and switch there.
                 mRoomViewWidget->reset();
                 mRoomViewWidget->setRoomConfig( mRoomConfigAdapter );
                 mCentralTabWidget->addTab( mRoomViewWidget, "Room" );
                 mCentralTabWidget->setCurrentWidget( mRoomViewWidget );
             });
    connect( mStateInRoom, &QState::exited,
             [this]
             {
                 mLogger->debug( "exited InRoom" );

                 // Disable leaving room.
                 mLeaveRoomAction->setEnabled( false );

                 // OK to join and create rooms again.
                 mServerViewWidget->enableJoinRoom( true );
                 mServerViewWidget->enableCreateRoom( true );

                 // Remove the rooms tab and switch to the server tab.
                 mCentralTabWidget->removeTab( mCentralTabWidget->indexOf( mRoomViewWidget ) );
                 mCentralTabWidget->setCurrentWidget( mServerViewWidget );
             });
    connect( mStateDisconnecting, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered Disconnecting" );
                 mConnectionStatusLabel->setText( tr("Disconnecting...") );
                 mConnectAction->setEnabled( false );
                 mDisconnectAction->setEnabled( false );
                 mKeepAliveTimer->stop();
             });
    connect( mStateDisconnected, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered Disconnected" );
                 mConnectionStatusLabel->setText( tr("Not connected") );
                 mConnectAction->setEnabled( true );
                 mDisconnectAction->setEnabled( false );
                 mKeepAliveTimer->stop();

                 // Reset draft view area.
                 mCardsList[CARD_ZONE_DRAFT].clear();
                 processCardListChanged( CARD_ZONE_DRAFT );
                 mLeftCommanderPane->setDraftAlert( false );
                 mRightCommanderPane->setDraftAlert( false );
                 mLeftCommanderPane->setDraftQueuedPacks( -1 );
                 mRightCommanderPane->setDraftQueuedPacks( -1 );
                 mLeftCommanderPane->setDraftTickCount( -1 );
                 mRightCommanderPane->setDraftTickCount( -1 );

                 // Clear out ticker and notify of disconnect if a
                 // connection had been established.
                 if( mConnectionEstablished )
                 {
                     clearTicker();
                     mTickerWidget->enqueueOneShotWidget( new QLabel( "Disconnected" ) );
                 }

                 // Reset server view area.
                 mServerViewWidget->setAnnouncements( QString() );
                 mServerViewWidget->clearRooms();
                 mServerViewWidget->enableJoinRoom( false );
                 mServerViewWidget->enableCreateRoom( false );

                 // Server alerts don't apply anymore.
                 mAlertMessageBox->hide();
             });

    mStateMachine->start();
}


CardDataSharedPtr
Client::createCardData( const std::string& setCode, const std::string& name )
{
    CardData* cardData  = nullptr;
    if( mAllSetsData )
    {
        cardData = mAllSetsData->createCardData( setCode, name );
    }

    if( !cardData )
    {
        // Could not create normally, so create a simple placeholder.
        cardData = new SimpleCardData( name, setCode );
    }

    return CardDataSharedPtr( cardData );
}


void
Client::closeEvent( QCloseEvent* event )
{
    mLogger->trace( "closeEvent" );

    // The state machine is asynchronous to the GUI, so there are problems
    // with shutting down the GUI widget while the state machine is still
    // reaching into its member data.  Stop the state machine cleanly
    // before allowing the window to close.
    if( mStateMachine->isRunning() )
    {
        mLogger->debug( "stopping state machine" );

        // This will cause the state machine's stopped() signal to close
        // this widget, triggering closeEvent() again.
        connect( mStateMachine, &QStateMachine::stopped, this, &Client::close );
        mStateMachine->stop();

        // Don't allow the event to close the window yet.
        event->ignore();
    }
    QMainWindow::closeEvent( event );
}


void
Client::connectToServer( const QString& host, int port )
{
    mIncomingMsgHeader = 0;
    mTcpSocket->abort();
    mTcpSocket->connectToHost( host, port );
    emit eventConnecting();
}


void
Client::disconnectFromServer()
{
    mLogger->trace( "disconnectFromServer" );
    if( mStateMachine->configuration().contains( mStateConnecting ) )
    {
        // QTcpSocket doesn't signal disconnect() (or error()) if it
        // hasn't already connected, so this event is necessary.
        emit eventConnectingAborted();
    }
    else
    {
        emit eventDisconnecting();
    }
    mTcpSocket->abort();
}


void
Client::readFromServer()
{
    QDataStream in( mTcpSocket );
    in.setVersion( QDataStream::Qt_4_0 );

    int bytesAvail = mTcpSocket->bytesAvailable();
    mLogger->debug( "readFromServer: bytesAvail={}", bytesAvail );

    while( mTcpSocket->bytesAvailable() )
    {
        if( mIncomingMsgHeader == 0 ) {
            if( mTcpSocket->bytesAvailable() < (int)sizeof( mIncomingMsgHeader ) )
                return;

            in >> mIncomingMsgHeader;
        }

        const bool msgCompressed = mIncomingMsgHeader & 0x8000;
        const quint16 msgSize = mIncomingMsgHeader & 0x7FFF;

        if( mTcpSocket->bytesAvailable() < msgSize )
            return;

        QByteArray msgByteArray;
        msgByteArray.resize( msgSize );
        in.readRawData( msgByteArray.data(), msgSize );

        if( msgCompressed )
        {
            msgByteArray = qUncompress( msgByteArray );
            mLogger->debug( "read {} bytes, uncompressed to {} bytes",
                    msgSize, msgByteArray.size() );
        }

        thicket::ServerToClientMsg msg;
        bool msgParsed = msg.ParseFromArray( msgByteArray.data(), msgByteArray.size() );
        mIncomingMsgHeader = 0;

        if( !msgParsed )
        {
            mLogger->warn( "Failed to parse msg!" );
            continue;
        }

        handleMessageFromServer( msg );
    }
}


void
Client::handleMessageFromServer( const thicket::ServerToClientMsg& msg )
{
    if( msg.has_greeting_ind() )
    {
        const thicket::GreetingInd& ind = msg.greeting_ind();
        const unsigned int major = ind.protocol_version_major();
        const unsigned int minor = ind.protocol_version_minor();
        mLogger->debug( "GreetingInd: proto={}.{}, name={}, version={}", major, minor,
               ind.server_name(), ind.server_version() );

        // Inform and disconnect if the protocol versions aren't compatible.
        if( major != thicket::PROTOCOL_VERSION_MAJOR )
        {
            QString serverProtoStr = QString::number( major ) + "." + QString::number( minor );
            QString clientProtoStr = QString::number( thicket::PROTOCOL_VERSION_MAJOR ) + "." +
                    QString::number( thicket::PROTOCOL_VERSION_MINOR );
            mLogger->warn( "Protocol incompatibility with server: server={}, client={}",
                    serverProtoStr, clientProtoStr );
            QMessageBox::critical( this, tr("Protocol Mismatch"),
                    tr("Your client is incompatible with this server.\n\n"
                       "(Server protocol version %1, client protocol version %2).")
                       .arg( serverProtoStr ).arg( clientProtoStr ) );
            disconnectFromServer();
            return;
        }

        mServerName = QString::fromStdString( ind.server_name() );
        mServerVersion = QString::fromStdString( ind.server_version() );
        mConnectionStatusLabel->setText( QString( "Connected to server " + mServerName +
                " (version " + mServerVersion + ")" ) );

        // Connected to something valid.  Update the client settings with connection parameters.
        mSettings->setConnectHost( mConnectDialog->getHost() );
        mSettings->setConnectPort( mConnectDialog->getPort() );
        mSettings->setConnectName( mConnectDialog->getName() );

        // Send login request.
        mLogger->debug( "Sending LoginReq, name={}", mConnectDialog->getName() );
        thicket::ClientToServerMsg msg;
        thicket::LoginReq* req = msg.mutable_login_req();
        mUserName = mConnectDialog->getName();
        req->set_name( mUserName.toStdString() );
        sendProtoMsg( msg, mTcpSocket );
    }
    else if( msg.has_announcements_ind() )
    {
        mLogger->debug( "AnnouncementsInd" );
        const thicket::AnnouncementsInd& ind = msg.announcements_ind();
        mServerViewWidget->setAnnouncements( QString::fromStdString( ind.text() ) );
    }
    else if( msg.has_alerts_ind() )
    {
        mLogger->debug( "AlertsInd" );
        const thicket::AlertsInd& ind = msg.alerts_ind();
        const QString text = QString::fromStdString( ind.text() );
        if( !text.isEmpty() )
        {
            mAlertMessageBox->setText( text );
            mAlertMessageBox->show();
        }
        else
        {
            mAlertMessageBox->hide();
        }
    }
    else if( msg.has_login_rsp() )
    {
        const thicket::LoginRsp& rsp = msg.login_rsp();
        mLogger->debug( "LoginRsp: result={}", rsp.result() );

        // Check success/fail and take appropriate action.
        if( rsp.result() == thicket::LoginRsp::RESULT_SUCCESS )
        {
            // Trigger machine state transition.
            emit eventLoggedIn();
        }
        else
        {
            mLogger->notice( "Failed to login!" );

            // Disconnect.  A retry could be attempted here while connected
            // but it's probably not worth it.
            disconnectFromServer();

            if( rsp.result() == thicket::LoginRsp::RESULT_FAILURE_NAME_IN_USE )
            {
                QMessageBox::warning( this, tr("Login Failed"),
                        tr("Could not log in to server - name already in use.  Reconnect and try again.") );
            }
            else
            {
                QMessageBox::warning( this, tr("Login Failed"),
                        // Other errors should be less common, so use a generic response.
                        tr("Could not log in to server - error %1.  Reconnect and try again.").arg( rsp.result() ) );
            }
        }
    }
    else if( msg.has_chat_message_delivery_ind() )
    {
        const thicket::ChatMessageDeliveryInd& ind = msg.chat_message_delivery_ind();
        mLogger->debug( "ChatMessageDeliveryInd: sender={}, scope={}, message={}",
                ind.sender(), ind.scope(), ind.text() );

        if( ind.scope() == thicket::CHAT_SCOPE_ALL )
        {
            mServerViewWidget->addChatMessageItem( QString::fromStdString( ind.sender() ),
                    QString::fromStdString( ind.text() ) );
        }
        else if( ind.scope() == thicket::CHAT_SCOPE_ROOM )
        {
            mRoomViewWidget->addChatMessageItem( QString::fromStdString( ind.sender() ),
                    QString::fromStdString( ind.text() ) );
        }
        else
        {
            mLogger->warn( "chat scope {} not currently supported", ind.scope() );
        }

    }
    else if( msg.has_room_capabilities_ind() )
    {
        processMessageFromServer( msg.room_capabilities_ind() );
    }
    else if( msg.has_rooms_info_ind() )
    {
        const thicket::RoomsInfoInd& ind = msg.rooms_info_ind();
        mLogger->debug( "RoomsInfoInd: addedRooms={}, deletedRooms={}, playerCounts={}",
                ind.added_rooms_size(), ind.removed_rooms_size(), ind.player_counts_size() );

        // Add any rooms in the message.
        for( int i = 0; i < ind.added_rooms_size(); ++i )
        {
            const thicket::RoomsInfoInd::RoomInfo& roomInfo = ind.added_rooms( i );
            const thicket::RoomConfiguration& roomConfig = roomInfo.room_config();
            auto roomConfigAdapter = std::make_shared<RoomConfigAdapter>( roomInfo.room_id(), roomConfig,
                   mLoggingConfig.createChildConfig( "roomconfigadapter" ) );
            mServerViewWidget->addRoom( roomConfigAdapter );
        }

        // Delete any rooms in the message.
        for( int i = 0; i < ind.removed_rooms_size(); ++i )
        {
            const int roomId = ind.removed_rooms( i );
            mServerViewWidget->removeRoom( roomId );
        }

        // Update any player counts in the message.
        for( int i = 0; i < ind.player_counts_size(); ++i )
        {
            const thicket::RoomsInfoInd::PlayerCount& playerCount = ind.player_counts( i );
            mServerViewWidget->updateRoomPlayerCount( playerCount.room_id(),
                                                      playerCount.player_count() );
        }
    }
    else if( msg.has_create_room_success_rsp() )
    {
        const thicket::CreateRoomSuccessRsp& rsp = msg.create_room_success_rsp();
        const int roomId = rsp.room_id();
        mLogger->debug( "CreateRoomSuccessRsp: roomId={}", roomId );

        // The room has been created on the server but it's up to the
        // client to join their own room.
        mLogger->debug( "Sending JoinRoomReq, roomId={}", roomId );
        thicket::ClientToServerMsg msg;
        thicket::JoinRoomReq* req = msg.mutable_join_room_req();
        req->set_room_id( roomId );
        req->set_password( mCreatedRoomPassword );
        sendProtoMsg( msg, mTcpSocket );
    }
    else if( msg.has_create_room_failure_rsp() )
    {
        const thicket::CreateRoomFailureRsp& rsp = msg.create_room_failure_rsp();
        const thicket::CreateRoomFailureRsp::ResultType result = rsp.result();
        mLogger->debug( "CreateRoomFailureRsp: result={}", result );

        // Bring up a warning dialog.
        const QMap<thicket::CreateRoomFailureRsp::ResultType,QString> lookup = {
                { thicket::CreateRoomFailureRsp::RESULT_INVALID_SET_CODE, "A set code was invalid." },
                { thicket::CreateRoomFailureRsp::RESULT_INVALID_PACK_GENERATION_TYPE, "The pack generation method was invalid." },
                { thicket::CreateRoomFailureRsp::RESULT_NAME_IN_USE, "The room name is already in use." } };
        QString warningMsg = lookup.contains( result ) ? lookup[result] :
                tr("Error %1.").arg( result );
        QMessageBox::warning( this, tr("Failed to Create Room"), warningMsg );
    }
    else if( msg.has_join_room_success_rspind() )
    {
        processMessageFromServer( msg.join_room_success_rspind() );
    }
    else if( msg.has_join_room_failure_rsp() )
    {
        const thicket::JoinRoomFailureRsp& rsp = msg.join_room_failure_rsp();
        const thicket::JoinRoomFailureRsp::ResultType result = rsp.result();
        mLogger->debug( "JoinRoomFailureRsp: result={}", result );

        // Bring up a warning dialog.
        const QMap<thicket::JoinRoomFailureRsp::ResultType,QString> lookup = {
                { thicket::JoinRoomFailureRsp::RESULT_ROOM_FULL, "The room is full." },
                { thicket::JoinRoomFailureRsp::RESULT_INVALID_PASSWORD, "Invalid password." } };
        QString warningMsg = lookup.contains( result ) ? lookup[result] :
                tr("Error %1.").arg( result );
        QMessageBox::warning( this, tr("Failed to Join Room"), warningMsg );
    }
    else if( msg.has_player_inventory_ind() )
    {
        processMessageFromServer( msg.player_inventory_ind() );
    }
    else if( msg.has_room_occupants_info_ind() )
    {
        const thicket::RoomOccupantsInfoInd& ind = msg.room_occupants_info_ind();
        mLogger->debug( "RoomOccupantsInfoInd: id={} players={}", ind.room_id(), ind.players_size() );

        // Update room view widget.
        mRoomViewWidget->clearPlayers();
        mRoomViewWidget->setChairCount( mRoomConfigAdapter->getChairCount() );
        for( int i = 0; i < ind.players_size(); ++i )
        {
            const thicket::RoomOccupantsInfoInd::Player& player = ind.players( i );
            QString state;
            switch( player.state() )
            {
                case thicket::RoomOccupantsInfoInd::Player::STATE_STANDBY:  state = "standby";  break;
                case thicket::RoomOccupantsInfoInd::Player::STATE_READY:    state = "ready";    break;
                case thicket::RoomOccupantsInfoInd::Player::STATE_ACTIVE:   state = "active";   break;
                case thicket::RoomOccupantsInfoInd::Player::STATE_DEPARTED: state = "departed"; break;
                default: state = "unknown";
            }

            mRoomViewWidget->setPlayerInfo( player.chair_index(), QString::fromStdString( player.name() ), player.is_bot(), state );
        }

        // Clear player status widgets data structures and layout.
        mPlayerStatusWidgetMap.clear();
        mPassDirWidgetList.clear();
        qtutils::clearLayout( mTickerPlayerStatusLayout );

        const int fh = qtutils::getDefaultFontHeight();
        const int dim = (fh * 3) / 4;  // arrows are 3/4 default font size
        const QSize size( dim, dim );
        mPassDirLeftWidget = new SizedSvgWidget( size );
        mPassDirLeftWidget->setContentsMargins( 0, 0, 0, 0 );
        mTickerPlayerStatusLayout->addWidget( mPassDirLeftWidget );

        for( int i = 0; i < ind.players_size(); ++i )
        {
            if( i > 0 )
            {
                // Place arrow widget
                SizedSvgWidget* passDirWidget = new SizedSvgWidget( size );
                mPassDirWidgetList.push_back( passDirWidget );
                mTickerPlayerStatusLayout->addWidget( passDirWidget );
            }

            // Place player widget.
            const thicket::RoomOccupantsInfoInd::Player& player = ind.players( i );
            PlayerStatusWidget *playerStatusWidget = new PlayerStatusWidget( QString::fromStdString( player.name() ) );
            if( player.state() == thicket::RoomOccupantsInfoInd::Player::STATE_DEPARTED )
            {
                playerStatusWidget->setPlayerActive( false );
            }
            mPlayerStatusWidgetMap.insert( player.chair_index(), playerStatusWidget );
            mTickerPlayerStatusLayout->addWidget( playerStatusWidget );
        }

        mPassDirRightWidget = new SizedSvgWidget( size );
        mPassDirRightWidget->setContentsMargins( 0, 0, 0, 0 );
        mTickerPlayerStatusLayout->addWidget( mPassDirRightWidget );
    }
    else if( msg.has_room_chairs_info_ind() )
    {
        processMessageFromServer( msg.room_chairs_info_ind() );
    }
    else if( msg.has_room_chairs_deck_info_ind() )
    {
        processMessageFromServer( msg.room_chairs_deck_info_ind() );
    }
    else if( msg.has_room_stage_ind() )
    {
        processMessageFromServer( msg.room_stage_ind() );
    }
    else if( msg.has_player_current_pack_ind() )
    {
        const thicket::PlayerCurrentPackInd& ind = msg.player_current_pack_ind();
        currentPackId = ind.pack_id();
        mLogger->debug( "Current pack ind: {}", currentPackId );

        // Create new cards and add to the draft list.
        mCardsList[CARD_ZONE_DRAFT].clear();
        for( int i = 0; i < ind.cards_size(); ++i )
        {
            const thicket::Card& card = ind.cards(i);
            CardDataSharedPtr cardDataSharedPtr = createCardData( card.set_code(), card.name() );
            mCardsList[CARD_ZONE_DRAFT].push_back( cardDataSharedPtr );
        }
        processCardListChanged( CARD_ZONE_DRAFT );
    }
    else if( msg.has_player_card_selection_rsp() )
    {
        const thicket::PlayerCardSelectionRsp& rsp = msg.player_card_selection_rsp();
        mLogger->debug( "CardSelRsp: result={} pack={} card={}",
                rsp.result(), rsp.pack_id(), rsp.card() );
        if( rsp.result() )
        {
            processCardSelected( rsp.card(), false );
        }
        else
        {
            mLogger->notice( "Selection request failed for card={}", rsp.card() );
        }
    }
    else if( msg.has_player_auto_card_selection_ind() )
    {
        const thicket::PlayerAutoCardSelectionInd& ind = msg.player_auto_card_selection_ind();
        mLogger->debug( "AutoCardSelInd: type={} pack={} card={}",
                ind.type(), ind.pack_id(), ind.card() );
        processCardSelected( ind.card(), true );
    }
    else
    {
        mLogger->warn( "Unrecognized message: {}", msg.msg_case() );
    }
}


void
Client::processMessageFromServer( const thicket::RoomCapabilitiesInd& ind )
{
    mLogger->debug( "RoomCapabilitiesInd" );
    std::vector<RoomCapabilitySetItem> sets;
    for( int i = 0; i < ind.sets_size(); ++i )
    {
        const thicket::RoomCapabilitiesInd::SetCapability& set = ind.sets( i );
        mLogger->debug( "  code={} name={} boosterGen={}", set.code(), set.name(), set.booster_generation() );
        RoomCapabilitySetItem setItem = { set.code(), set.name(), set.booster_generation() };
        sets.push_back( setItem );
    }
    mCreateRoomDialog->setRoomCapabilitySets( sets );
}


void
Client::processMessageFromServer( const thicket::JoinRoomSuccessRspInd& rspInd )
{
    mLogger->debug( "JoinRoomSuccessRspInd: roomId={} rejoin={} chairIdx={}", rspInd.room_id(), rspInd.rejoin(), rspInd.chair_idx() );

    // Clear out all zones when joining a room.
    for( auto zone : gCardZoneTypeArray )
    {
        mCardsList[zone].clear();
        processCardListChanged( zone );
    }

    mChairIndex = rspInd.chair_idx();
    mRoomConfigAdapter = std::make_shared<RoomConfigAdapter>( rspInd.room_id(), rspInd.room_config(),
           mLoggingConfig.createChildConfig( "roomconfigadapter" ) );

    // Trigger state machine update.
    emit eventJoinedRoom();

    if( rspInd.rejoin() )
    {
        // In the special cast of rejoining an active room, go straight to the draft tab.
        mCentralTabWidget->setCurrentWidget( mDraftViewWidget );
    }
}


void
Client::processMessageFromServer( const thicket::PlayerInventoryInd& ind )
{
    mLogger->debug( "PlayerInventoryInd" );

    // Iterate over each zone, processing differences between what is
    // currently in place and the final result in order to reduce load
    // of creating and loading new card data objects.
    for( CardZoneType zone : { CARD_ZONE_MAIN, CARD_ZONE_SIDEBOARD, CARD_ZONE_JUNK } )
    {
        mLogger->debug( "processing zone: {}", stringify( zone ) );

        thicket::Zone tz = convertCardZone( zone );

        // Assemble the "before" card list.
        std::vector<SimpleCardData> beforeCardList;
        for( auto card : mCardsList[zone] )
        {
            beforeCardList.push_back(
                    SimpleCardData( card->getName(), card->getSetCode() ) );
        }

        // Assemble the "after" card list.
        std::vector<SimpleCardData> afterCardList;
        for( int i = 0; i < ind.drafted_cards_size(); ++i )
        {
            const thicket::PlayerInventoryInd::DraftedCard& draftedCard =ind.drafted_cards( i );
            const thicket::Card& card = draftedCard.card();
            if( draftedCard.zone() == tz )
            {
                afterCardList.push_back( SimpleCardData( card.name(), card.set_code() ) );
            }
        }

        // Create a list of the intersection of both lists.
        std::sort( beforeCardList.begin(), beforeCardList.end() );
        std::sort( afterCardList.begin(), afterCardList.end());
        std::vector<SimpleCardData> intersectionCardList;
        std::set_intersection( afterCardList.begin(), afterCardList.end(),
                               beforeCardList.begin(), beforeCardList.end(),
                               std::back_inserter( intersectionCardList ) );

        // Create a list of all extra cards to be removed. (before - intersection = extra)
        std::vector<SimpleCardData> extraCardList;
        std::set_difference( beforeCardList.begin(), beforeCardList.end(),
                             intersectionCardList.begin(), intersectionCardList.end(),
                             std::back_inserter( extraCardList ) );

        // Remove extra cards from local card list.
        for( auto card : extraCardList )
        {
            auto equalityFunc = [&card] ( const CardDataSharedPtr& c ) {
                return (*c == card); };

            mLogger->debug( "removing card: {}", card.getName() );
            auto iter = std::find_if( mCardsList[zone].begin(),
                                      mCardsList[zone].end(), equalityFunc );
            if( iter != mCardsList[zone].end() )
            {
                mCardsList[zone].erase( iter );
            }
        }

        // Create a list of all new cards to be added locally. (join - intersection = extra)
        std::vector<SimpleCardData> newCardList;
        std::set_difference( afterCardList.begin(), afterCardList.end(),
                             intersectionCardList.begin(), intersectionCardList.end(),
                             std::back_inserter( newCardList ) );

        // Add all new cards to the local card zone.
        for( auto card : newCardList )
        {
            mLogger->debug( "adding card  {}", card.getName() );
            CardDataSharedPtr cardDataSharedPtr = createCardData( card.getSetCode(), card.getName() );
            mCardsList[zone].push_back( cardDataSharedPtr );
        }

        // Assume the zone was updated.
        processCardListChanged( zone );

        //
        // Update basic lands for the zone.
        //

        BasicLandQuantities qtys;
        for( int i = 0; i < ind.basic_land_qtys_size(); ++i )
        {
            const thicket::PlayerInventoryInd::BasicLandQuantity& basicLandQty = ind.basic_land_qtys( i );
            if( basicLandQty.zone() == tz )
            {
                mLogger->debug( "basic land ({}) ({}): {}", stringify( basicLandQty.basic_land() ),
                        stringify( tz ), basicLandQty.quantity() );
                qtys.setQuantity( convertBasicLand( basicLandQty.basic_land() ),
                        basicLandQty.quantity() );
            }
        }

        mBasicLandQtysMap[zone] = qtys;
        mLeftCommanderPane->setBasicLandQuantities( zone, qtys );
        mRightCommanderPane->setBasicLandQuantities( zone, qtys );
    }
}

void
Client::processMessageFromServer( const thicket::RoomChairsInfoInd& ind )
{
    mLogger->debug( "RoomChairsInfoInd: chairs={}", ind.chairs_size() );

    // Update player status widgets with info.
    for( int i = 0; i < ind.chairs_size(); ++i )
    {
        const thicket::RoomChairsInfoInd::Chair& chair = ind.chairs( i );
        const unsigned int queuedPacks = chair.queued_packs();
        const unsigned int timeRemaining = chair.time_remaining();
        const int chairIndex = chair.chair_index();

        mLogger->debug( "RoomChairsInfoInd: chair={} queuedPacks={}, timeRemaining={}",
                chairIndex, queuedPacks, timeRemaining );

        if( mPlayerStatusWidgetMap.contains( chairIndex ) )
        {
            PlayerStatusWidget *widget = mPlayerStatusWidgetMap[chairIndex];
            widget->setPackQueueSize( queuedPacks );
            widget->setTimeRemaining( mRoundTimerEnabled && (queuedPacks > 0) ? timeRemaining : -1 );
        }
        else
        {
            mLogger->warn( "chair info for unknown player index {}!", chairIndex );
        }

        if( chairIndex == mChairIndex )
        {
            if( mRoundTimerEnabled && (queuedPacks > 0) )
            {
                mLeftCommanderPane->setDraftTickCount( timeRemaining );
                mRightCommanderPane->setDraftTickCount( timeRemaining );
                mLeftCommanderPane->setDraftAlert( (timeRemaining > 0) && (timeRemaining <= 10) );
                mRightCommanderPane->setDraftAlert( (timeRemaining > 0) && (timeRemaining <= 10) );

                mLeftCommanderPane->setDraftQueuedPacks( queuedPacks );
                mRightCommanderPane->setDraftQueuedPacks( queuedPacks );

            }
            else
            {
                mLeftCommanderPane->setDraftTickCount( -1 );
                mRightCommanderPane->setDraftTickCount( -1 );
                mLeftCommanderPane->setDraftAlert( false );
                mRightCommanderPane->setDraftAlert( false );

                mLeftCommanderPane->setDraftQueuedPacks( -1 );
                mRightCommanderPane->setDraftQueuedPacks( -1 );
            }
        }
    }
}


void
Client::processMessageFromServer( const thicket::RoomChairsDeckInfoInd& ind )
{
    mLogger->debug( "RoomChairsDeckInfoInd: chairs={}", ind.chairs_size() );

    // Update player status widgets with info.
    for( int i = 0; i < ind.chairs_size(); ++i )
    {
        const thicket::RoomChairsDeckInfoInd::Chair& chair = ind.chairs( i );
        const int chairIndex = chair.chair_index();
        const std::string cockatriceHash = chair.cockatrice_hash();
        const std::string mwsHash = chair.mws_hash();

        mLogger->debug( "RoomChairsDeckInfoInd: chair={} cockatriceHash={}, mwsHash={}",
                chairIndex, cockatriceHash, mwsHash );
        mRoomViewWidget->setPlayerCockatriceHash( chairIndex, QString::fromStdString( cockatriceHash ) );
    }
}


void
Client::processMessageFromServer( const thicket::RoomStageInd& ind )
{
    mLogger->debug( "RoomStageInd, round={}, complete={}", ind.round(), ind.complete() );
    mCurrentRound = ind.round();
    if( ind.complete() )
    {
        // === Draft complete ===

        // Clear out draft card area.
        mCardsList[CARD_ZONE_DRAFT].clear();
        processCardListChanged( CARD_ZONE_DRAFT );

        mDraftStatusLabel->setText( "Draft Complete" );

        clearTicker();
        mTickerWidget->addPermanentWidget( new QLabel( "Draft Complete" ) );
    }
    else if( mCurrentRound >= 0 )
    {
        // === Draft running ===

        // If the draft just begun, switch the view to the Draft tab.
        if( mCurrentRound == 0 )
        {
            mCentralTabWidget->setCurrentWidget( mDraftViewWidget );
        }

        bool currentRoundClockwise = false;
        if( mRoomConfigAdapter )
        {
            currentRoundClockwise = mRoomConfigAdapter->isRoundClockwise( mCurrentRound );
            mRoundTimerEnabled = (mRoomConfigAdapter->getRoundTime( mCurrentRound ) > 0);
        }
        else
        {
            mLogger->warn( "room configuration not initialized!" );
        }

        // Update pass direction indicators.
        if( currentRoundClockwise )
        {
            mPassDirLeftWidget->hide();
            mPassDirRightWidget->load( RESOURCE_SVG_ARROW_CCW_LEFT );
            mPassDirRightWidget->show();
        }
        else
        {
            mPassDirRightWidget->hide();
            mPassDirLeftWidget->load( RESOURCE_SVG_ARROW_CW_RIGHT );
            mPassDirLeftWidget->show();
        }

        for( int i = 0; i < mPassDirWidgetList.size(); ++i )
        {
            mPassDirWidgetList[i]->load( currentRoundClockwise ?
                    RESOURCE_SVG_ARROW_RIGHT : RESOURCE_SVG_ARROW_LEFT );
        }
  
        // Update draft status indicators.
        QString statusStr = "Draft round " + QString::number( mCurrentRound + 1 );
        mDraftStatusLabel->setText( statusStr );
    }
    else
    {
        // Draft not complete and not running (unexpected).
        mLogger->warn( "ignoring RoomStateInd" );
    }
}


void
Client::processCardSelected( const thicket::Card& card, bool autoSelected )
{
    // Card was selected, empty out draft list.
    mCardsList[CARD_ZONE_DRAFT].clear();
    processCardListChanged( CARD_ZONE_DRAFT );

    // Create new card data for the indicated card and add to the
    // destination zone.  Often the card data has already been created
    // and could be reused from the draft card list, but not always.
    // Creating is the simplest thing to do.
    CardDataSharedPtr cardDataSharedPtr = createCardData( card.set_code(), card.name() );
    mCardsList[mDraftedCardDestZone].push_back( cardDataSharedPtr );

    // If the destination zone wasn't main, the server needs to know about
    // the implicit move to another zone.
    if( mTcpSocket->state() == QTcpSocket::ConnectedState )
    {
        if( mDraftedCardDestZone != CARD_ZONE_MAIN )
        {
            mLogger->trace( "sendPlayerInventoryUpdateInd" );
            thicket::ClientToServerMsg msg;
            thicket::PlayerInventoryUpdateInd* ind = msg.mutable_player_inventory_update_ind();
            addPlayerInventoryUpdateDraftedCardMove( ind, cardDataSharedPtr, CARD_ZONE_MAIN, mDraftedCardDestZone );
            sendProtoMsg( msg, mTcpSocket );
        }
    }

    processCardListChanged( mDraftedCardDestZone );

    // Pop up the card on the ticker.
    Client_CardLabel* cardLabel = new Client_CardLabel( cardDataSharedPtr->getMultiverseId(), mImageLoaderFactory, this );
    cardLabel->loadImage( /*mImageCache*/ );
    cardLabel->setText( "Selection:   <b>" + QString::fromStdString( card.name() ) +
            "</b>" + (autoSelected ? " (auto-selected)" : "") );
    mTickerWidget->enqueueOneShotWidget( cardLabel );
}


void
Client::processCardListChanged( const CardZoneType& cardZone )
{
    mLeftCommanderPane->setCards( cardZone, mCardsList[cardZone] );
    mRightCommanderPane->setCards( cardZone, mCardsList[cardZone] );
}


void
Client::processCardZoneMoveRequest( const CardDataSharedPtr& cardData, const CardZoneType& srcCardZone, const CardZoneType& destCardZone )
{
    if( srcCardZone == destCardZone ) return;

    // Can't move cards into the draft zone.
    if( destCardZone == CARD_ZONE_DRAFT ) return;

    // Moves from the draft zone are special - a request to the server needs
    // to go out before the move is allowed.
    if( srcCardZone == CARD_ZONE_DRAFT )
    {
        mDraftedCardDestZone = destCardZone;
        mLogger->debug( "send draft selection" );
        thicket::ClientToServerMsg msg;
        thicket::PlayerCardSelectionReq* req = msg.mutable_player_card_selection_req();
        req->set_pack_id( currentPackId );
        thicket::Card* card = req->mutable_card();
        card->set_name( cardData->getName() );
        card->set_set_code( cardData->getSetCode() );
        sendProtoMsg( msg, mTcpSocket );
        return;
    }

    // Send an inventory update message to server.
    if( mTcpSocket->state() == QTcpSocket::ConnectedState )
    {
        mLogger->trace( "sendPlayerInventoryUpdateInd" );
        thicket::ClientToServerMsg msg;
        thicket::PlayerInventoryUpdateInd* ind = msg.mutable_player_inventory_update_ind();
        addPlayerInventoryUpdateDraftedCardMove( ind, cardData, srcCardZone, destCardZone );
        sendProtoMsg( msg, mTcpSocket );
    }

    // Remove from source if it exists.  If this handler was called by a
    // lingering popup menu, it's possible (but unlikely) that the source
    // item has changed and isn't there anymore.
    bool removed = mCardsList[srcCardZone].removeOne( cardData );
    if( !removed )
    {
        mLogger->notice( "unable to move card, no longer in source zone" );
        return;
    }

    mCardsList[destCardZone].push_back( cardData );

    // Update changes to local card lists.
    processCardListChanged( srcCardZone );
    processCardListChanged( destCardZone );
}


void
Client::handleCardZoneMoveRequest( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData, const CardZoneType& destCardZone )
{
    mLogger->debug( "handleCardZoneMoveRequest: {} {}->{}", cardData->getName(), srcCardZone, destCardZone );
    processCardZoneMoveRequest( cardData, srcCardZone, destCardZone );
}


void
Client::handleCardZoneMoveAllRequest( const CardZoneType& srcCardZone, const CardZoneType& destCardZone )
{
    mLogger->debug( "handleCardZoneMoveAllRequest: {}->{}", srcCardZone, destCardZone );

    if( srcCardZone == destCardZone ) return;

    // Can't move all cards into or out of the draft zone.
    if( srcCardZone == CARD_ZONE_DRAFT ) return;
    if( destCardZone == CARD_ZONE_DRAFT ) return;

    // Send an inventory update message to server.
    if( mTcpSocket->state() == QTcpSocket::ConnectedState )
    {
        mLogger->trace( "sendPlayerInventoryUpdateInd" );
        thicket::ClientToServerMsg msg;
        thicket::PlayerInventoryUpdateInd* ind = msg.mutable_player_inventory_update_ind();
        for( auto cardData : mCardsList[srcCardZone] )
        {
            addPlayerInventoryUpdateDraftedCardMove( ind, cardData, srcCardZone, destCardZone );
        }
        sendProtoMsg( msg, mTcpSocket );
    }

    // Put all source cards onto dest list, then clear source list.
    mCardsList[destCardZone].append( mCardsList[srcCardZone] );
    mCardsList[srcCardZone].clear();

    processCardListChanged( srcCardZone );
    processCardListChanged( destCardZone );
}


void
Client::handleCardSelected( const CardZoneType& srcCardZone, const CardDataSharedPtr& cardData )
{
    CommanderPane *senderCommanderPane = qobject_cast<CommanderPane *>( QObject::sender() );
    mLogger->debug( "handleCardSelected: {} {}->?", cardData->getName(), srcCardZone );

    // Figure out the destination zone.
    CommanderPane *destCommanderPane = senderCommanderPane == mLeftCommanderPane ? mRightCommanderPane : mLeftCommanderPane;
    const CardZoneType destCardZone = destCommanderPane->getCurrentCardZone();
    mLogger->debug( "handleCardSelected: {} {}->{}", cardData->getName(), srcCardZone, destCardZone );

    processCardZoneMoveRequest( cardData, srcCardZone, destCardZone );
}


void
Client::handleBasicLandQuantitiesUpdate( const CardZoneType& cardZone, const BasicLandQuantities& qtys )
{
    mLogger->debug( "handleBasicLandQuantitiesUpdate: zone={} totalqty={}",
            cardZone, qtys.getTotalQuantity() );

    // Send an inventory update message to server.
    // OPTIMIZATION - every time the user clicks a land button one of
    // these small messages goes out.  These could be bundled and sent
    // as a single message after a certain amount of time expires.
    if( mTcpSocket->state() == QTcpSocket::ConnectedState )
    {
        mLogger->trace( "sendPlayerInventoryUpdateInd" );
        thicket::ClientToServerMsg msg;
        thicket::PlayerInventoryUpdateInd* ind = msg.mutable_player_inventory_update_ind();
        for( BasicLandType basic : gBasicLandTypeArray )
        {
            int diff = qtys.getQuantity( basic ) - mBasicLandQtysMap[cardZone].getQuantity( basic );
            if( diff != 0 )
            {
                mLogger->debug( "  {}: {}", stringify( basic ), diff );
                thicket::PlayerInventoryUpdateInd::BasicLandAdjustment* basicLandAdj =
                        ind->add_basic_land_adjustments();
                basicLandAdj->set_basic_land( convertBasicLand( basic ) );
                basicLandAdj->set_zone( convertCardZone( cardZone ) );
                basicLandAdj->set_adjustment( diff );
                sendProtoMsg( msg, mTcpSocket );
            }
        }
    }

    // Update local quantities.
    mBasicLandQtysMap[cardZone] = qtys;
}


void
Client::handleJoinRoomRequest( int roomId, const QString& password )
{
    // Check that we are connected and logged in, but not already in a room.
    if( mStateMachine->configuration().contains( mStateNotInRoom ) )
    {
        // Send request to join the room.
        mLogger->debug( "Sending JoinRoomReq, roomId={}", roomId );
        thicket::ClientToServerMsg msg;
        thicket::JoinRoomReq* req = msg.mutable_join_room_req();
        req->set_room_id( roomId );
        req->set_password( password.toStdString() );
        sendProtoMsg( msg, mTcpSocket );
    }
    else
    {
        mLogger->debug( "join room ignored (invalid state)" );
    }
}


void
Client::handleCreateRoomRequest()
{
    // Check that we are connected and logged in, but not already in a room.
    if( mStateMachine->configuration().contains( mStateNotInRoom ) )
    {
        // Bring up the create room dialog.
        int result = mCreateRoomDialog->exec();
        if( result == QDialog::Accepted )
        {
            const QString roomNameStr = mCreateRoomDialog->getRoomName();
            const QString passwordStr = mCreateRoomDialog->getPassword();
            const QString setCodesStr = mCreateRoomDialog->getSetCodes().join( "/" );
            const int chairCount = mCreateRoomDialog->getChairCount();
            const int botCount = mCreateRoomDialog->getBotCount();
            const int selectionTime = mCreateRoomDialog->getSelectionTime();
            mLogger->debug( "create room: name={} passwd={} chairCount={} botCount={} selectionTime={} sets={}",
                    roomNameStr,
                    passwordStr,
                    chairCount,
                    botCount,
                    selectionTime,
                    setCodesStr );

            mLogger->debug( "sending CreateRoomReq" );
            thicket::ClientToServerMsg msg;
            thicket::CreateRoomReq* req = msg.mutable_create_room_req();
            if( !passwordStr.isEmpty() )
            {
                mCreatedRoomPassword = passwordStr.toStdString();
                req->set_password( mCreatedRoomPassword );
            }
            thicket::RoomConfiguration* roomConfig = req->mutable_room_config();
            roomConfig->set_name( roomNameStr.toStdString() );
            roomConfig->set_password_protected( !passwordStr.isEmpty() );
            roomConfig->set_chair_count( chairCount );
            roomConfig->set_bot_count( botCount );

            for( int i = 0; i < 3; ++i )
            {
                thicket::RoomConfiguration::Round* round = roomConfig->add_rounds();
                thicket::RoomConfiguration::BoosterRoundConfiguration* boosterRoundConfig = round->mutable_booster_round_config();
                boosterRoundConfig->set_time( selectionTime );

                thicket::RoomConfiguration::CardBundle* bundle = boosterRoundConfig->add_card_bundles();
                bundle->set_set_code( mCreateRoomDialog->getSetCodes()[i].toStdString() );

                // These per-round options aren't currently available in the
                // dialog options, using typical draft settings.
                bundle->set_method( bundle->METHOD_BOOSTER );
                bundle->set_set_replacement( true );
                boosterRoundConfig->set_clockwise( (i%2) == 0 );
            }

            sendProtoMsg( msg, mTcpSocket );
        }
    }
    else
    {
        mLogger->debug( "create room ignored (invalid state)" );
    }
}


void
Client::handleServerChatMessageGenerated( const QString& text )
{
    // Check that we are connected and logged in.
    if( mStateMachine->configuration().contains( mStateLoggedIn ) )
    {
        // Send chat indication to server.
        mLogger->debug( "sending ChatMessageInd, text={}", text );
        thicket::ClientToServerMsg msg;
        thicket::ChatMessageInd* ind = msg.mutable_chat_message_ind();
        ind->set_scope( thicket::CHAT_SCOPE_ALL );
        ind->set_text( text.toStdString() );
        sendProtoMsg( msg, mTcpSocket );
    }
    else
    {
        mLogger->debug( "server chat message ignored (invalid state)" );
    }
}


void
Client::handleReadyUpdate( bool ready )
{
    // Check that we are connected, logged in, and in a room.
    if( mStateMachine->configuration().contains( mStateInRoom ) )
    {
        mLogger->debug( "Sending PlayerReadyInd, ready={}", ready );
        thicket::ClientToServerMsg msg;
        thicket::PlayerReadyInd* ind = msg.mutable_player_ready_ind();
        ind->set_ready( ready );
        sendProtoMsg( msg, mTcpSocket );
    }
    else
    {
        mLogger->debug( "handleReadyUpdate ignored (invalid state)" );
    }
}


void
Client::handleRoomLeave()
{
    // Check that we are connected, logged in, and in a room.
    if( mStateMachine->configuration().contains( mStateInRoom ) )
    {
        int result = QMessageBox::warning( this, tr("Leave Room"),
                                   tr("Are you sure you want to leave the room?"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Cancel );
        if( result == QMessageBox::Ok )
        {
            mLogger->debug( "Sending RoomDepartInd" );
            thicket::ClientToServerMsg msg;
            thicket::DepartRoomInd* ind = msg.mutable_depart_room_ind();
            Q_UNUSED( ind );
            sendProtoMsg( msg, mTcpSocket );

            // Trigger state machine update
            emit eventDepartedRoom();
        }
    }
    else
    {
        mLogger->debug( "handleRoomLeave ignored (invalid state)" );
    }
}


void
Client::handleRoomChatMessageGenerated( const QString& text )
{
    // Check that we are connected, logged in, and in a room.
    if( mStateMachine->configuration().contains( mStateInRoom ) )
    {
        // Send chat indication to server.
        mLogger->debug( "sending ChatMessageInd, text={}", text );
        thicket::ClientToServerMsg msg;
        thicket::ChatMessageInd* ind = msg.mutable_chat_message_ind();
        ind->set_scope( thicket::CHAT_SCOPE_ROOM );
        ind->set_text( text.toStdString() );
        sendProtoMsg( msg, mTcpSocket );
    }
    else
    {
        mLogger->debug( "room chat message ignored (invalid state)" );
    }
}


void
Client::addPlayerInventoryUpdateDraftedCardMove( thicket::PlayerInventoryUpdateInd* ind,
                                                 const CardDataSharedPtr&           cardData,
                                                 const CardZoneType&                srcCardZone,
                                                 const CardZoneType&                destCardZone )
{
    thicket::PlayerInventoryUpdateInd::DraftedCardMove* move =
            ind->add_drafted_card_moves();
    thicket::Card* card = move->mutable_card();
    card->set_name( cardData->getName() );
    card->set_set_code( cardData->getSetCode() );
    move->set_zone_from( convertCardZone( srcCardZone ) );
    move->set_zone_to( convertCardZone( destCardZone ) );
}


bool
Client::sendProtoMsg( const thicket::ClientToServerMsg& protoMsg, QTcpSocket* mTcpSocket )
{
    const int protoSize = protoMsg.ByteSize();

    QByteArray msgByteArray;
    msgByteArray.resize( protoSize );
    protoMsg.SerializeToArray( msgByteArray.data(), protoSize );

    // 16-bit header: 1 bit compression flag, 15 bits size.
    quint16 header = 0x0000;
    QByteArray* payloadMsgByteArrayPtr;

    const int COMPRESSION_MAX = 9;
    QByteArray compressedMsgByteArray = qCompress( msgByteArray, COMPRESSION_MAX );
    mLogger->debug( "serialized {} bytes, compressed to {} bytes", protoSize, compressedMsgByteArray.size() );
    if( compressedMsgByteArray.size() < protoSize )
    {
        // The compression resulted in a smaller payload.
        header |= 0x8000;
        payloadMsgByteArrayPtr = &compressedMsgByteArray;
    }
    else
    {
        mLogger->debug( "inefficient compression, sending uncompressed" );
        payloadMsgByteArrayPtr = &msgByteArray;
    }

    const int payloadSize = payloadMsgByteArrayPtr->size();
    if( payloadSize > 0x7FFF )
    {
        mLogger->error( "payload too large ({} bytes) to send!", payloadSize );
        return false;
    }
    header |= payloadSize;

    QByteArray block;
    block.resize( sizeof( header ) + payloadSize );
    QDataStream out( &block, QIODevice::WriteOnly );
    out.setVersion( QDataStream::Qt_4_0 );
    out << (quint16) header;
    out.writeRawData( payloadMsgByteArrayPtr->data(), payloadSize );

    bool writeResult = mTcpSocket->write( block );

    // Restart the keep-alive timer.
    if( writeResult ) mKeepAliveTimer->start( KEEP_ALIVE_TIMER_SECS * 1000 );

    return writeResult;
}


void
Client::handleSocketError( QAbstractSocket::SocketError socketError )
{
    if( socketError == QAbstractSocket::RemoteHostClosedError )
    {
        mLogger->debug( "remote host closed socket" );
    }
    else
    {
        emit eventConnectionError();

        // For any other type of error, ensure the socket is reset.
        mTcpSocket->abort();

        if( socketError == QAbstractSocket::HostNotFoundError )
        {
            QMessageBox::warning( this, tr("Host Not Found"),
                    tr("The host was not found. Please check the host name and port settings.") );
        }
        else if( socketError == QAbstractSocket::ConnectionRefusedError )
        {
            QMessageBox::warning( this, tr("Connection Refused"),
                    tr("The connection was refused by the peer.  "
                       "Check that the host name and port settings are correct.") );
        }
        else
        {
            QMessageBox::warning( this, tr("Thicket Client"),
                    tr("The following error occurred: %1.").arg(mTcpSocket->errorString()));
        }
    }

    // Retry the connection action.
    QTimer::singleShot(0, this, SLOT(handleConnectAction()));
}


void
Client::handleConnectAction()
{
    int result = mConnectDialog->exec();
    if( result == QDialog::Accepted )
    {
        connectToServer( mConnectDialog->getHost(), mConnectDialog->getPort() );
    }
}


void
Client::handleDisconnectAction()
{
    int result = QMessageBox::warning( this, tr("Disconnect"),
                               tr("Are you sure you want to disconnect?"),
                               QMessageBox::Ok | QMessageBox::Cancel,
                               QMessageBox::Cancel );
    if( result == QMessageBox::Ok )
    {
        disconnectFromServer();
    }
}

void
Client::handleSaveDeckAction()
{
    mLogger->debug( "Saving deck" );

    // Create a save file dialog.  Done explicitly rather than via the
    // QFileDialog static APIs to force a non-native dialog for windows.
    // Windows' native dialog halts the app event loop which causes
    // problems, most importantly pausing the QTimer sending a keep-
    // alive message to the server to keep the server from disconnecting us.
    QFileDialog dialog( this, tr("Save Deck"), QString(), tr("Deck Files (*.dec);;All Files (*.*)") );
    dialog.setAcceptMode( QFileDialog::AcceptSave );
    dialog.setOptions( QFileDialog::DontUseNativeDialog );
    dialog.setDefaultSuffix( ".dec" );

    int result = dialog.exec();

    if( result == QDialog::Rejected ) return;
    if( dialog.selectedFiles().empty() ) return;

    QString filename = dialog.selectedFiles().at(0);
    mLogger->debug( "saving file: {}", filename );

    QFile file( filename );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        mLogger->warn( "Unable to open file for writing!" );
        return;
    }

    QTextStream out( &file );
    Decklist decklist;

    // Add basic lands to the decklist.
    std::set<std::string> highPriorityCardNames;
    for( auto basic : gBasicLandTypeArray )
    {
        const std::string cardName = mBasicLandCardDataMap.getCardData( basic )->getName();
        decklist.addCard( cardName, Decklist::ZONE_MAIN,
                mBasicLandQtysMap[CARD_ZONE_MAIN].getQuantity( basic ) );
        decklist.addCard( cardName, Decklist::ZONE_SIDEBOARD,
                mBasicLandQtysMap[CARD_ZONE_SIDEBOARD].getQuantity( basic ) );
        highPriorityCardNames.insert( cardName );
    }

    // Add main cards.
    for( CardDataSharedPtr& cardData : mCardsList[CARD_ZONE_MAIN] )
    {
        decklist.addCard( cardData->getName() );
    }

    // Add sideboard cards.
    for( CardDataSharedPtr& cardData : mCardsList[CARD_ZONE_SIDEBOARD] )
    {
        decklist.addCard( cardData->getName(), Decklist::ZONE_SIDEBOARD );
    }

    // Save the decklist.
    out << QString::fromStdString( decklist.getFormattedString( Decklist::FORMAT_DEFAULT, highPriorityCardNames ) );
}


void
Client::handleUpdateCardsAction()
{
    int result = mAllSetsUpdateDialog->exec();
    if( result == QDialog::Accepted )
    {
        updateAllSetsData( mAllSetsUpdateDialog->getAllSetsData() );
    }
}


void
Client::handleAboutAction()
{
    QString about;
    about += tr("<b>Thicket Client</b>");
    about += tr("<br><i>version ") + QString::fromStdString( gClientVersion ) + "</i><hr>";
    about += tr("Please report bugs on the <a href=\"http://github.com/mildmongrel/thicket/issues\">project issues page</a>.");
    about += tr("<br>Email feedback to <a href=\"mailto:mildmongrel@gmail.com\">mildmongrel@gmail.com</a>.");
    about += tr("<p>Thanks to the owners and maintainers of the following projects:");
    about += tr("<br>MTG JSON, RapidJSON, spdlog, Google protobuf, version-git, and Catch.");
    about += tr("<p>Icon provided by game-icons.net.");
    about += tr("<p>Built with Qt.");

    QMessageBox::about( this, tr("About"), about );
}


void
Client::handleKeepAliveTimerTimeout()
{
    mLogger->debug( "timer expired - sending keepalive msg to server" );
    thicket::ClientToServerMsg msg;
    msg.mutable_keep_alive_ind();
    sendProtoMsg( msg, mTcpSocket );
}


void
Client::clearTicker()
{
    // Clear out all widgets from the ticker and delete them unless
    // they are the player status widget.
    QWidget* widget = mTickerWidget->takePermanentWidgetAt( 0 );
    while( widget != nullptr )
    {
        if( widget != mTickerPlayerStatusWidget )
        {
            widget->deleteLater();
        }
        widget = mTickerWidget->takePermanentWidgetAt( 0 );
    }
}


#include <QToolTip>
#include "ImageLoader.h"

Client_CardLabel::Client_CardLabel( int muid, ImageLoaderFactory* imageLoaderFactory, QWidget* parent )
      : QLabel( parent ),
        mMuid( muid ),
        mImageLoaderFactory( imageLoaderFactory ),
        mImageLoader( nullptr )
{}


void
Client_CardLabel::loadImage()
{
    if( mMuid < 0 ) return;

    if( mImageLoader != nullptr ) mImageLoader->deleteLater();

    mImageLoader = mImageLoaderFactory->createImageLoader(
            Logging::Config(), this );

    connect(mImageLoader, SIGNAL(imageLoaded(int, const QImage&)),
            this, SLOT(handleImageLoaded(int, const QImage&)));
    mImageLoader->loadImage( mMuid );
}


void
Client_CardLabel::handleImageLoaded( int multiverseId, const QImage &image )
{
    if( multiverseId == mMuid )
    {
        mToolTipStr = qtutils::getImageAsHtmlText( image );

        // Finished with the loader after this is called.
        mImageLoader->deleteLater();
    }
}

void
Client_CardLabel::enterEvent( QEvent* event )
{
    if( mToolTipStr.isEmpty() ) return;
    QToolTip::showText( QCursor::pos(), mToolTipStr );
}

