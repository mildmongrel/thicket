#include "client.h"

#include <QtWidgets>
#include <QtNetwork>
#include <QHBoxLayout>
#include <QStateMachine>
#include <QTimer>

#include "version.h"
#include "qtutils_core.h"
#include "qtutils_widget.h"
#include "ClientToastOverlay.h"
#include "ClientSettings.h"
#include "messages.pb.h"
#include "ImageCache.h"
#include "ImageLoaderFactory.h"
#include "AllSetsData.h"
#include "CommanderPane.h"
#include "CommanderPaneSettings.h"
#include "TickerWidget.h"
#include "ServerViewWidget.h"
#include "ConnectDialog.h"
#include "CreateRoomWizard.h"
#include "RoomConfigAdapter.h"
#include "DraftConfigAdapter.h"
#include "ProtoHelper.h"
#include "ClientProtoHelper.h"
#include "DeckStatsLauncher.h"
#include "WebServerInterface.h"
#include "ClientUpdateChecker.h"
#include "AllSetsUpdater.h"
#include "DraftSidebar.h"
#include "ReadySplash.h"
#include "TickerPlayerReadyWidget.h"
#include "TickerPlayerHashesWidget.h"
#include "TickerPlayerStatusWidget.h"
#include "TickerPostRoundTimerWidget.h"
#include "ServerConnection.h"
#include "SettingsDialog.h"

// Client protocol version.
static const SimpleVersion CLIENT_PROTOVERSION( proto::PROTOCOL_VERSION_MAJOR,
                                                proto::PROTOCOL_VERSION_MINOR );

// Keep-alive timer duration.
static const int KEEP_ALIVE_TIMER_SECS = 25;

// Helper function for logging protocol-type cards.
static std::ostream& operator<<( std::ostream& os, const proto::Card& card )
{
    os << '[' << card.set_code() << ',' << card.name() << ']';
    return os;
}

Client::Client( ClientSettings*             settings,
                const AllSetsDataSharedPtr& allSetsData,
                AllSetsUpdater*             allSetsUpdater,
                ImageCache*                 cardImageCache,
                ImageCache*                 expSymImageCache,
                const Logging::Config&      loggingConfig,
                QWidget*                    parent )
:   QMainWindow( parent ),
    mSettings( settings ),
    mAllSetsData( allSetsData ),
    mAllSetsUpdater( allSetsUpdater ),
    mConnectionEstablished( false ),
    mReadySplash( nullptr ),
    mCardServerSetCodeMap( new CardServerSetCodeMap() ),
    mChairIndex( -1 ),
    mRoundTimerEnabled( false ),
    mRoomStageRunning( false ),
    mPostRoundTimerActive( false ),
    mRoomRejoined( false ),
    mAwaitingRoomStateAfterRejoin( false ),
    mDraftedCardDestZone( CARD_ZONE_MAIN ),
    mUnsavedChanges( false ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    // Connect AllSetsUpdater update signal.  Note the finished signal is
    // connected in the state machine.
    connect( mAllSetsUpdater, SIGNAL(allSetsUpdated(const AllSetsDataSharedPtr&)), this, SLOT(updateAllSetsData(const AllSetsDataSharedPtr&)) );

    // Connect settings update signals.
    connect( mSettings, &ClientSettings::basicLandMultiverseIdsChanged, this, [this](const BasicLandMuidMap&) {
            updateBasicLandCardDataMap();
        } );

    mImageLoaderFactory = new ImageLoaderFactory( mAllSetsData,
                                                  cardImageCache,
                                                  settings->getCardImageUrlTemplate(),
                                                  expSymImageCache,
                                                  "http://gatherer.wizards.com/Handlers/Image.ashx?type=symbol&set=%setcode%&size=large&rarity=C",
                                                  this );

    mServerViewWidget = new ServerViewWidget( mLoggingConfig.createChildConfig( "serverview" ), this );
    connect( mServerViewWidget, &ServerViewWidget::joinRoomRequest, this, &Client::handleJoinRoomRequest );
    connect( mServerViewWidget, &ServerViewWidget::createRoomRequest, this, &Client::handleCreateRoomRequest );
    connect( mServerViewWidget, &ServerViewWidget::chatMessageGenerated, this, &Client::handleServerChatMessageGenerated );

    mLeftCommanderPane = new CommanderPane( CommanderPaneSettings( *mSettings, 0 ),
            { CARD_ZONE_BOOSTER_DRAFT, CARD_ZONE_GRID_DRAFT, CARD_ZONE_AUTO, CARD_ZONE_MAIN, CARD_ZONE_SIDEBOARD, CARD_ZONE_JUNK },
            mImageLoaderFactory, mLoggingConfig.createChildConfig( "LeftCmdrMain" ) );
    connect( mLeftCommanderPane, &CommanderPane::cardZoneMoveAllRequest,
             this,               &Client::handleCardZoneMoveAllRequest );
    connect(mLeftCommanderPane, SIGNAL(cardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)),
            this, SLOT(handleCardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)));
    connect(mLeftCommanderPane, SIGNAL(cardPreselected(const CardDataSharedPtr&)),
            this, SLOT(handleCardPreselected(const CardDataSharedPtr&)));
    connect(mLeftCommanderPane, SIGNAL(cardSelected(const CardZoneType&,const CardDataSharedPtr&)),
            this, SLOT(handleCardSelected(const CardZoneType&,const CardDataSharedPtr&)));
    connect(mLeftCommanderPane, SIGNAL(cardIndicesSelected(const CardZoneType&,const QList<int>&)),
            this, SLOT(handleCardIndicesSelected(const CardZoneType&,const QList<int>&)));
    connect(mLeftCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            this, SLOT(handleBasicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)));
    mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_BOOSTER_DRAFT, true );
    mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_GRID_DRAFT, true );
    mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_AUTO, true );
    mLeftCommanderPane->setCurrentCardZone( CARD_ZONE_MAIN );

    mRightCommanderPane = new CommanderPane( CommanderPaneSettings( *mSettings, 1 ),
            { CARD_ZONE_MAIN, CARD_ZONE_SIDEBOARD, CARD_ZONE_JUNK },
            mImageLoaderFactory, mLoggingConfig.createChildConfig( "RightCmdrMain" ) );
    connect( mRightCommanderPane, &CommanderPane::cardZoneMoveAllRequest,
             this,                &Client::handleCardZoneMoveAllRequest );
    connect(mRightCommanderPane, SIGNAL(cardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)),
            this, SLOT(handleCardZoneMoveRequest(const CardZoneType&,const CardDataSharedPtr&,const CardZoneType&)));
    connect(mRightCommanderPane, SIGNAL(cardPreselected(const CardDataSharedPtr&)),
            this, SLOT(handleCardPreselected(const CardDataSharedPtr&)));
    connect(mRightCommanderPane, SIGNAL(cardSelected(const CardZoneType&,const CardDataSharedPtr&)),
            this, SLOT(handleCardSelected(const CardZoneType&,const CardDataSharedPtr&)));
    connect(mRightCommanderPane, SIGNAL(cardIndicesSelected(const CardZoneType&,const QList<int>&)),
            this, SLOT(handleCardIndicesSelected(const CardZoneType&,const QList<int>&)));
    connect(mRightCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            this, SLOT(handleBasicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)));

    // Wire basic land quantity update signals from one pane to another for auto-updating.
    connect(mRightCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            mLeftCommanderPane, SLOT(setBasicLandQuantities(const CardZoneType&,const BasicLandQuantities&)));
    connect(mLeftCommanderPane, SIGNAL(basicLandQuantitiesUpdate(const CardZoneType&,const BasicLandQuantities&)),
            mRightCommanderPane, SLOT(setBasicLandQuantities(const CardZoneType&,const BasicLandQuantities&)));

    // Create server connection object and connect signals.  Note the
    // connect and disconnect signals are connected in the state machine.
    Logging::Config serverConnLoggingConfig = mLoggingConfig.createChildConfig( "serverconnection" );
    mServerConn = new ServerConnection( serverConnLoggingConfig, this );
    connect( mServerConn, &ServerConnection::protoMsgReceived,
             this,        &Client::handleMessageFromServer );
    connect( mServerConn, SIGNAL(error(QAbstractSocket::SocketError)),
             this,        SLOT(handleSocketError(QAbstractSocket::SocketError)));

    // Create keep-alive timer.
    mKeepAliveTimer = new QTimer( this );
    connect( mKeepAliveTimer, &QTimer::timeout, this, &Client::handleKeepAliveTimerTimeout );

    // Create server-aligned draft timer.
    mServerAlignedDraftTimer = new QTimer( this );
    connect( mServerAlignedDraftTimer, &QTimer::timeout, this, &Client::handleServerAlignedDraftTimerTimeout );

    QWidget* draftSidebarHolder = new QWidget();
    QVBoxLayout* draftSidebarLayout = new QVBoxLayout( draftSidebarHolder );

    mDraftSidebar = new DraftSidebar( mImageLoaderFactory, mLoggingConfig.createChildConfig( "draftsidebar" ), this );
    connect( mDraftSidebar, &DraftSidebar::chatMessageComposed, this, &Client::handleRoomChatMessageGenerated );

    QToolButton* sidebarButton = new QToolButton();
    sidebarButton->setAutoRaise( true );
    QHBoxLayout* sidebarButtonLayout = new QHBoxLayout();
    sidebarButtonLayout->addStretch();
    sidebarButtonLayout->addWidget( sidebarButton );
    sidebarButton->setArrowType( mDraftSidebar->isCompacted() ? Qt::RightArrow : Qt::LeftArrow );

    draftSidebarLayout->setContentsMargins( 0, 0, 5, 0 );
    draftSidebarLayout->addLayout( sidebarButtonLayout );
    draftSidebarLayout->addWidget( mDraftSidebar );

    // Splitter provides draggable separator between widgets.
    mSplitter = new QSplitter();
    mSplitter->addWidget( draftSidebarHolder );
    mSplitter->setStretchFactor( 0, 0 );
    mSplitter->setCollapsible( 0, false );
    mSplitter->addWidget( mLeftCommanderPane );
    mSplitter->setCollapsible( 1, false );
    mSplitter->addWidget( mRightCommanderPane );
    mSplitter->setCollapsible( 2, false );

    // Update the button arrow direction when the sidebar is compacted.
    connect( mDraftSidebar, &DraftSidebar::compacted, this, [=]() {
            sidebarButton->setArrowType( Qt::RightArrow );
        } );

    // Update the button arrow direction when the sidebar is expanded.
    connect( mDraftSidebar, &DraftSidebar::expanded, this, [=]() {
            sidebarButton->setArrowType( Qt::LeftArrow );
        } );

    // Handle sidebar button click.
    connect( sidebarButton, &QToolButton::clicked, this, [=]() {
            static int prevExpandedWidth = 0;
            const int sidebarIndex = mSplitter->indexOf( draftSidebarHolder );
            if( sidebarIndex < 0 ) return;

            int newSidebarWidth;
            if( mDraftSidebar->isCompacted() )
            {
                newSidebarWidth = (prevExpandedWidth > 0) ? prevExpandedWidth : draftSidebarHolder->sizeHint().width();
            }
            else
            {
                prevExpandedWidth = draftSidebarHolder->width();
                newSidebarWidth = draftSidebarHolder->minimumSizeHint().width();
            }

            QList<int> sizes = mSplitter->sizes();
            int diff = sizes[sidebarIndex] - newSidebarWidth;
            sizes[sidebarIndex] = newSidebarWidth;
            for( int i = 0; i < mSplitter->count(); ++i )
            {
                // Apply equal parts of difference to non-sidebar areas.
                if( i != sidebarIndex )
                    sizes[i] = sizes[i] + diff / (mSplitter->count() - 1);
            }
            mSplitter->setSizes( sizes );
        } );

    mTickerWidget = new TickerWidget();
    QFontMetrics fm( font() );
    mTickerWidget->setFixedHeight( fm.height() + 4 );
    mTickerWidget->start();

    mTickerPlayerReadyWidget = new TickerPlayerReadyWidget( mTickerWidget->getInteriorHeight() );
    mTickerPlayerStatusWidget = new TickerPlayerStatusWidget( mTickerWidget->getInteriorHeight() );
    mTickerPlayerHashesWidget = new TickerPlayerHashesWidget( mTickerWidget->getInteriorHeight() );
    mTickerPostRoundTimerWidget = new TickerPostRoundTimerWidget( mTickerWidget->getInteriorHeight() );

    mDraftViewWidget = new QWidget();
    QVBoxLayout* draftViewLayout = new QVBoxLayout();
    draftViewLayout->addWidget( mSplitter );
    draftViewLayout->setStretchFactor( mSplitter, 1 );
    draftViewLayout->addWidget( mTickerWidget );
    mDraftViewWidget->setLayout( draftViewLayout );

    mCentralTabWidget = new QTabWidget();
    mCentralTabWidget->setTabPosition( QTabWidget::West );
    mCentralTabWidget->addTab( mServerViewWidget, "Server" );

    connect( mCentralTabWidget, &QTabWidget::currentChanged, this, [=](int i) {
            // Show/hide the ready splash if necessary
            if( mReadySplash ) {
                if( mCentralTabWidget->widget( i ) == mDraftViewWidget ) {
                    mReadySplash->show();
                } else {
                    mReadySplash->hide();
                }
            }
        } );

    setCentralWidget( mCentralTabWidget );

    // Restore main window geometry if available.
    QByteArray geometryData = mSettings->getMainWindowGeometry();
    if( !geometryData.isEmpty() )
    {
        // Restore saved settings.
        mLogger->debug( "restoring window geometry from settings" );
        restoreGeometry( geometryData );
    }
    else
    {
        // This trick forces the window to do its layout so that the size()
        // call below will return a good result.  The window doesn't actually
        // do anything visible here because it's hidden again before
        // relinquishing to Qt's queue.
        show();
        layout()->invalidate();
        hide();

        // Center the window.
        mLogger->debug( "no window geometry settings, centering with default geometry" );
        setGeometry( QStyle::alignedRect(
                Qt::LeftToRight, Qt::AlignCenter, size(), QApplication::desktop()->screen()->rect() ) );
    }

    // Resize the splitter to previous state if possible.
    QByteArray splitterState = mSettings->getDraftTabSplitterState();
    if( !splitterState.isEmpty() )
    {
        // Restore saved settings.
        mSplitter->restoreState( splitterState );
    }

    // Create and show toast overlay, setting toasts to pop up just above the status bar.
    mToastOverlay = new ClientToastOverlay( this );
    QPoint br( -5, 0 - statusBar()->height() - 5 );
    mToastOverlay->setBottomRightOffset( br );
    mToastOverlay->show();

    // --- MENU ACTIONS ---

    QAction* checkClientUpdateAction = new QAction(tr("&Check for Client Update..."), this);
    checkClientUpdateAction->setStatusTip(tr("Check for updates to the client software"));
    connect(checkClientUpdateAction, SIGNAL(triggered()), this, SLOT(handleCheckClientUpdateAction()));

    QAction* updateCardsAction = new QAction(tr("&Update Card Data..."), this);
    updateCardsAction->setStatusTip(tr("Update the card database"));
    connect(updateCardsAction, SIGNAL(triggered()), this, SLOT(handleUpdateCardsAction()));

    QAction* settingsAction = new QAction(tr("&Settings..."), this);
    settingsAction->setStatusTip(tr("Configure client settings"));
    connect( settingsAction, &QAction::triggered, this, [this]() { mSettingsDialog->exec(); } );

    QAction* quitAction = new QAction(tr("&Quit"), this);
    quitAction->setStatusTip(tr("Quit the application"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

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

    QAction* deckStatsAction = new QAction(tr("&Analyze Deck on deckstats.net"), this);
    deckStatsAction->setStatusTip(tr("Analyze the current deck on deckstats.net"));
    connect(deckStatsAction, SIGNAL(triggered()), this, SLOT(handleDeckStatsAction()));

    QAction* saveDeckAction = new QAction(tr("&Save Deck..."), this);
    saveDeckAction->setStatusTip(tr("Save the current deck"));
    connect(saveDeckAction, SIGNAL(triggered()), this, SLOT(handleSaveDeckAction()));

    QAction* aboutAction = new QAction(tr("&About..."), this);
    aboutAction->setStatusTip(tr("About the appication"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(handleAboutAction()));

    // --- MENU ---

    QMenu *thicketMenu = menuBar()->addMenu(tr("&Thicket"));
    thicketMenu->addAction(checkClientUpdateAction);
    thicketMenu->addAction(updateCardsAction);
    thicketMenu->addSeparator();
    thicketMenu->addAction(settingsAction);
    thicketMenu->addSeparator();
    thicketMenu->addAction(quitAction);

    QMenu *serverMenu = menuBar()->addMenu(tr("&Server"));
    serverMenu->addAction(mConnectAction);
    serverMenu->addAction(mDisconnectAction);
    serverMenu->addAction(mLeaveRoomAction);

    QMenu *deckMenu = menuBar()->addMenu(tr("&Deck"));
    deckMenu->addAction(saveDeckAction);
    deckMenu->addAction(deckStatsAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);

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

    mConnectDialog = new ConnectDialog( mLoggingConfig.createChildConfig( "connectdialog" ),
                                        this );

    mSettingsDialog = new SettingsDialog( mSettings,
                                          mLoggingConfig.createChildConfig( "settingsdialog" ),
                                          this );

    // Set initial connect history from settings.
    QStringList servers;
    servers << mSettings->getConnectBuiltinServers();
    servers << mSettings->getConnectUserServers();
    mConnectDialog->setKnownServers( servers );

    mConnectDialog->setLastGoodServer( mSettings->getConnectLastGoodServer() );
    mConnectDialog->setLastGoodUsername( mSettings->getConnectLastGoodUsername() );

    mCreateRoomWizard = new CreateRoomWizard( mImageLoaderFactory,
                                              mLoggingConfig.createChildConfig( "createdialog" ),
                                              this );

    mAlertMessageBox = new QMessageBox( this );
    mAlertMessageBox->setWindowTitle( tr("Server Alert") );
    mAlertMessageBox->setWindowModality( Qt::NonModal );
    mAlertMessageBox->setIcon( QMessageBox::Warning );

    updateAllSetsData( allSetsData );

    initStateMachine();
}


Client::~Client()
{
    // The ticker widgets may be parentless; clean them up explicitly.
    mTickerPlayerReadyWidget->deleteLater();
    mTickerPlayerStatusWidget->deleteLater();
    mTickerPlayerHashesWidget->deleteLater();
    mTickerPostRoundTimerWidget->deleteLater();
}


void
Client::updateAllSetsData( const AllSetsDataSharedPtr& allSetsDataSharedPtr )
{
    mLogger->debug( "updating AllSetsData" );
    mAllSetsData = allSetsDataSharedPtr;

    // Basic land images are conjured from allsets data, so update those.
    updateBasicLandCardDataMap();
}


void
Client::initStateMachine()
{
    // Create outer states.
    mStateMachine = new QStateMachine( this );
    mStateInitializing = new QState();
    mStateUpdating = new QState();
    mStateDisconnected = new QState();
    mStateConnecting = new QState();
    mStateConnected = new QState();
    mStateDisconnecting = new QState();

    // Create mStateUpdating (was NetworkReady) substates.
    mStateUpdatingClient = new QState( mStateUpdating );
    mStateUpdatingAllSets = new QState( mStateUpdating );
    mStateUpdating->setInitialState( mStateUpdatingClient );

    // Create mStateConnected substates.
    mStateLoggedOut = new QState( mStateConnected );
    mStateLoggedIn = new QState( mStateConnected );
    mStateConnected->setInitialState( mStateLoggedOut );

    // Create mStateLoggedIn substates.
    mStateNotInRoom = new QState( mStateLoggedIn );
    mStateInRoom = new QState( mStateLoggedIn );
    mStateLoggedIn->setInitialState( mStateNotInRoom );

    mStateMachine->addState( mStateInitializing );
    mStateMachine->addState( mStateUpdating );
    mStateMachine->addState( mStateDisconnected );
    mStateMachine->addState( mStateConnecting );
    mStateMachine->addState( mStateConnected );
    mStateMachine->addState( mStateDisconnecting );

    mStateMachine->setInitialState( mStateInitializing );

    mStateInitializing->addTransition(               this, SIGNAL( eventNetworkAvailable() ),     mStateUpdating        );
    mStateUpdatingClient->addTransition(             this, SIGNAL( eventClientUpdateChecked() ),  mStateUpdatingAllSets );
    mStateUpdatingAllSets->addTransition( mAllSetsUpdater, SIGNAL( finished() ),                  mStateDisconnected    );
    mStateConnecting->addTransition(          mServerConn, SIGNAL( connected() ),                 mStateConnected       );
    mStateConnecting->addTransition(                 this, SIGNAL( eventConnectingAborted() ),    mStateDisconnected    );
    mStateConnecting->addTransition(                 this, SIGNAL( eventConnectionError() ),      mStateDisconnected    );
    mStateConnecting->addTransition(          mServerConn, SIGNAL( disconnected() ),              mStateDisconnected    );
    mStateLoggedOut->addTransition(                  this, SIGNAL( eventLoggedIn() ),             mStateLoggedIn        );
    mStateNotInRoom->addTransition(                  this, SIGNAL( eventJoinedRoom() ),           mStateInRoom          );
    mStateInRoom->addTransition(                     this, SIGNAL( eventDepartedRoom() ),         mStateNotInRoom       );
    mStateConnected->addTransition(                  this, SIGNAL( eventDisconnecting() ),        mStateDisconnecting   );
    mStateConnected->addTransition(           mServerConn, SIGNAL( disconnected() ),              mStateDisconnected    );
    mStateDisconnecting->addTransition(       mServerConn, SIGNAL( disconnected() ),              mStateDisconnected    );
    mStateDisconnected->addTransition(               this, SIGNAL( eventConnecting() ),           mStateConnecting      );

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
    connect( mStateUpdating, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered Updating" );
                 mConnectionStatusLabel->setText( tr("Updating...") );
             });
    connect( mStateUpdatingClient, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered UpdatingClient" );

                 ClientUpdateChecker* checker = new ClientUpdateChecker(
                         mLoggingConfig.createChildConfig( "initclientupdatechecker" ), this );
                 
                 // Forward the check-finished signal to our own event signal.
                 connect( checker, SIGNAL(finished()), this, SIGNAL(eventClientUpdateChecked()) );

                 // Check in background.  (deletes itself when complete)
                 checker->check( mSettings->getWebServiceBaseUrl(), QString::fromStdString( gClientVersion ), true );
             });
    connect( mStateUpdatingAllSets, &QState::entered,
             [this]
             {
                 mLogger->debug( "entered UpdatingAllSets" );

                 // Start the update check in background.  Will prompt if update available.
                 mAllSetsUpdater->start( true );
             });
    connect( mStateUpdating, &QState::exited,
             [this]
             {
                 mLogger->debug( "exited Updating" );

                 // Kick off a connection when updates are complete.  Defer
                 // it with the singleShot so that the state machine's event
                 // loop proceeeds and we enter the next state.
                 QTimer::singleShot(0, this, SLOT(handleConnectAction()));
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

                 mRoomStageRunning = false;

                 // Clear out the ticker (room stage indication will add widgets).
                 clearTicker();

                 // Enable leaving room.
                 mLeaveRoomAction->setEnabled( true );

                 // Can't join or create rooms once in a room.
                 mServerViewWidget->enableJoinRoom( false );
                 mServerViewWidget->enableCreateRoom( false );

                 // Create the tab for the draft and switch to it.
                 const QString roomName = QString::fromStdString( mRoomConfigAdapter->getName() );
                 mCentralTabWidget->addTab( mDraftViewWidget, roomName );
                 mCentralTabWidget->setCurrentWidget( mDraftViewWidget );

                 // Update the draft sidebar with room info.
                 mDraftSidebar->addRoomJoinMessage( mRoomConfigAdapter, mRoomRejoined );

                 if( !mRoomRejoined )
                 {
                     // Create the "Ready" splash dialog, center, and display.
                     mReadySplash = new ReadySplash(this);
                     connect( mReadySplash, &ReadySplash::ready, this, &Client::handleReadyUpdate );
                     const QPoint global = this->mapToGlobal( rect().center() );
                     mReadySplash->show();
                     mReadySplash->move( global.x() - mReadySplash->width() / 2, global.y() - mReadySplash->height() / 2 );

                     mTickerWidget->addPermanentWidget( mTickerPlayerReadyWidget );
                 }
                 else
                 {
                     // In the special cast of rejoining an active room, user is
                     // assumed to be ready; go straight to the draft tab.
                     mCentralTabWidget->setCurrentWidget( mDraftViewWidget );
                 }
             });
    connect( mStateInRoom, &QState::exited,
             [this]
             {
                 mLogger->debug( "exited InRoom" );

                 // Stop the server-aligned timer.
                 mServerAlignedDraftTimer->stop();

                 // Update the draft sidebar.
                 mDraftSidebar->addRoomLeaveMessage( mRoomConfigAdapter );

                 // Reset room state variables.
                 mRoomRejoined = false;
                 mAwaitingRoomStateAfterRejoin = false;

                 // Reset draft state, if any.
                 mCardsList[CARD_ZONE_BOOSTER_DRAFT].clear();
                 processCardListChanged( CARD_ZONE_BOOSTER_DRAFT );
                 mCardsList[CARD_ZONE_GRID_DRAFT].clear();
                 processCardListChanged( CARD_ZONE_GRID_DRAFT );

                 // Reset left CommanderPane (it's the only one initialized with a draft tab).
                 mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_BOOSTER_DRAFT, true );
                 mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_GRID_DRAFT, true );
                 mLeftCommanderPane->setCurrentCardZone( CARD_ZONE_MAIN );
                 mLeftCommanderPane->setDraftActive( false );
                 mLeftCommanderPane->setDraftAlert( false );
                 mLeftCommanderPane->setDraftTimeRemaining( -1 );
                 mLeftCommanderPane->setDraftQueuedPacks( -1 );

                 // Disable leaving room.
                 mLeaveRoomAction->setEnabled( false );

                 // OK to join and create rooms again.
                 mServerViewWidget->enableJoinRoom( true );
                 mServerViewWidget->enableCreateRoom( true );

                 // Remove the "Ready" splash dialog if it's still around.
                 if( mReadySplash )
                 {
                     mReadySplash->deleteLater();
                     mReadySplash = nullptr;
                 }

                 // Clear out any toast messages.
                 mToastOverlay->clearToasts();

                 // Remove the drafting tab and switch to the server tab.
                 mCentralTabWidget->removeTab( mCentralTabWidget->indexOf( mDraftViewWidget ) );
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
                 mServerViewWidget->clearUsers();
                 mServerViewWidget->clearChatMessages();
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
        if( !cardData )
        {
            // Couldn't find the card by name in the set specified by the
            // server, either because the server used a set code we don't
            // know about, or more likely an empty set code.  Try to find
            // a usable set code for the card name so we can reference
            // real card information.
            std::string newSetCode = mAllSetsData->findSetCode( name );
            if( !newSetCode.empty() )
            {
                // Create card data with the found set code.
                cardData = mAllSetsData->createCardData( newSetCode, name );

                // Important: the server will still be expecting its set
                // code when this card is referenced, so that set code
                // is saved into a map here.
                mCardServerSetCodeMap->insert( cardData, setCode );
            }
        }
    }

    if( !cardData )
    {
        // Could not create normally, so create a simple placeholder.
        cardData = new SimpleCardData( name, setCode );
    }

    // Create a shared pointer with a custom deleter that cleans up the
    // server set code association with the card from the map.  The weak
    // pointer to the map is used to avoid accessing the map when cards
    // are being cleaned up after the map is destroyed.
    std::weak_ptr<CardServerSetCodeMap> mapWeakPtr( mCardServerSetCodeMap );
    CardDataSharedPtr sptr( cardData, [mapWeakPtr]( CardData* ptr ) {
            std::shared_ptr<CardServerSetCodeMap> mapSharedPtr = mapWeakPtr.lock();
            if( mapSharedPtr ) mapSharedPtr->remove( ptr );
            delete ptr;
        } );

    return sptr;
}


void
Client::moveEvent( QMoveEvent* event )
{
    // Keep the "Ready" splash dialog centered.
    if( mReadySplash )
    {
        const QPoint global = this->mapToGlobal( rect().center() );
        mReadySplash->move( global.x() - mReadySplash->width() / 2, global.y() - mReadySplash->height() / 2 );
    }
}


void
Client::resizeEvent( QResizeEvent* event )
{
    // Keep the "Ready" splash dialog centered.
    if( mReadySplash )
    {
        const QPoint global = this->mapToGlobal( rect().center() );
        mReadySplash->move( global.x() - mReadySplash->width() / 2, global.y() - mReadySplash->height() / 2 );
    }
}


void
Client::closeEvent( QCloseEvent* event )
{
    mLogger->trace( "closeEvent" );

    // Bring up a confirmation dialog if the application isn't already in
    // the process of being shut down and there are unsaved changes.
    if( mStateMachine->isRunning() && mUnsavedChanges )
    {
        int result = QMessageBox::question( this, tr("Confirm Quit"),
                                   tr("There are unsaved changes - are you sure you want to quit?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No );
        if( result != QMessageBox::Yes )
        {
            event->ignore();
            return;
        }
    }

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
    else
    {
        // The state machine is stopped; safe to handle close event.
        // Before closing, save important window settings.

        QByteArray geometryData = saveGeometry();
        mSettings->setMainWindowGeometry( geometryData );

        QByteArray splitterState = mSplitter->saveState();
        mSettings->setDraftTabSplitterState( splitterState );

        QMainWindow::closeEvent( event );
    }
}


void
Client::connectToServer( const QString& host, int port )
{
    mIncomingMsgHeader = 0;
    mServerConn->abort();
    mServerConn->connectToHost( host, port );
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
    mServerConn->abort();
}


void
Client::updateBasicLandCardDataMap()
{
    mLogger->debug( "updating BasicLandCardDataMap" );

    // Set basic land card data to use card data from settings.
    const BasicLandMuidMap muidMap = mSettings->getBasicLandMultiverseIds();
    for( auto basic : gBasicLandTypeArray )
    {
        CardData* cardData = nullptr;
        if( mAllSetsData )
        {
            cardData = mAllSetsData->createCardData( muidMap.getMuid( basic ) );
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
Client::handleMessageFromServer( const proto::ServerToClientMsg& msg )
{
    if( msg.has_greeting_ind() )
    {
        const proto::GreetingInd& ind = msg.greeting_ind();
        mServerProtoVersion = SimpleVersion( ind.protocol_version_major(), ind.protocol_version_minor() );
        mLogger->debug( "GreetingInd: proto={}, name={}, version={}", stringify( mServerProtoVersion ),
               ind.server_name(), ind.server_version() );

        // A better client could be developed to communicate using an
        // older protocol, but that's way more work than it's worth now.
        // If the client major protocol version is newer than the server,
        // inform and disconnect - the server's ClientDownloadInfo
        // information will be out of date.
        if( CLIENT_PROTOVERSION.getMajor() > mServerProtoVersion.getMajor() )
        {
            const QString serverProtoStr = QString::fromStdString( stringify( mServerProtoVersion ) );
            const QString clientProtoStr = QString::fromStdString( stringify( CLIENT_PROTOVERSION ) );
            mLogger->warn( "Protocol incompatibility with server (newer client): server={}, client={}",
                    serverProtoStr, clientProtoStr );
            QMessageBox::critical( this, tr("Protocol Mismatch"),
                    tr("The server is too old for your client."
                       "<br>(Server protocol version %1, client protocol version %2)"
                       "<p>Downgrade your client or connect to a newer server instance."
                       "<p>(Alternate client versions can be found at the project <a href=\"%3\">download area</a>.)")
                       .arg( serverProtoStr )
                       .arg( clientProtoStr )
                       .arg( WebServerInterface::getRedirectDownloadsUrl( mSettings->getWebServiceBaseUrl() ) ) );
            disconnectFromServer();
            return;
        }

        mServerName = QString::fromStdString( ind.server_name() );
        mServerVersion = QString::fromStdString( ind.server_version() );
        mConnectionStatusLabel->setText( QString( "Connected to server " + mServerName +
                " (version " + mServerVersion + ")" ) );

        // Send login request.
        mLogger->debug( "Sending LoginReq, name={}", mConnectDialog->getUsername() );
        proto::ClientToServerMsg msg;
        proto::LoginReq* req = msg.mutable_login_req();
        mUserName = mConnectDialog->getUsername();
        req->set_name( mUserName.toStdString() );
        req->set_protocol_version_major( proto::PROTOCOL_VERSION_MAJOR );
        req->set_protocol_version_minor( proto::PROTOCOL_VERSION_MINOR );
        req->set_client_version( gClientVersion );
        mServerConn->sendProtoMsg( msg );
    }
    else if( msg.has_announcements_ind() )
    {
        mLogger->debug( "AnnouncementsInd" );
        const proto::AnnouncementsInd& ind = msg.announcements_ind();
        mServerViewWidget->setAnnouncements( QString::fromStdString( ind.text() ) );
    }
    else if( msg.has_alerts_ind() )
    {
        mLogger->debug( "AlertsInd" );
        const proto::AlertsInd& ind = msg.alerts_ind();
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
        processMessageFromServer( msg.login_rsp() );

    }
    else if( msg.has_chat_message_delivery_ind() )
    {
        const proto::ChatMessageDeliveryInd& ind = msg.chat_message_delivery_ind();
        mLogger->debug( "ChatMessageDeliveryInd: sender={}, scope={}, message={}",
                ind.sender(), ind.scope(), ind.text() );

        if( ind.scope() == proto::CHAT_SCOPE_ALL )
        {
            mServerViewWidget->addChatMessage( QString::fromStdString( ind.sender() ),
                    QString::fromStdString( ind.text() ) );
        }
        else if( ind.scope() == proto::CHAT_SCOPE_ROOM )
        {
            mDraftSidebar->addChatMessage( QString::fromStdString( ind.sender() ),
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
        const proto::RoomsInfoInd& ind = msg.rooms_info_ind();
        mLogger->debug( "RoomsInfoInd: addedRooms={}, deletedRooms={}, playerCounts={}",
                ind.added_rooms_size(), ind.removed_rooms_size(), ind.player_counts_size() );

        // Add any rooms in the message.
        for( int i = 0; i < ind.added_rooms_size(); ++i )
        {
            const proto::RoomsInfoInd::RoomInfo& roomInfo = ind.added_rooms( i );
            const proto::RoomConfig& roomConfig = roomInfo.room_config();
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
            const proto::RoomsInfoInd::PlayerCount& playerCount = ind.player_counts( i );
            mServerViewWidget->updateRoomPlayerCount( playerCount.room_id(),
                                                      playerCount.player_count() );
        }
    }
    else if( msg.has_users_info_ind() )
    {
        const proto::UsersInfoInd& ind = msg.users_info_ind();
        mLogger->debug( "UsersInfoInd: addedUsers={}, deletedUsers={}",
                ind.added_users_size(), ind.removed_users_size() );

        // Add any users in the message.
        for( int i = 0; i < ind.added_users_size(); ++i )
        {
            const proto::UsersInfoInd::UserInfo& userInfo = ind.added_users( i );
            const QString name = QString::fromStdString( userInfo.name() );
            mServerViewWidget->addUser( name );
        }

        // Delete any users in the message.
        for( int i = 0; i < ind.removed_users_size(); ++i )
        {
            const QString name = QString::fromStdString( ind.removed_users( i ) );
            mServerViewWidget->removeUser( name );
        }
    }
    else if( msg.has_create_room_success_rsp() )
    {
        const proto::CreateRoomSuccessRsp& rsp = msg.create_room_success_rsp();
        const int roomId = rsp.room_id();
        mLogger->debug( "CreateRoomSuccessRsp: roomId={}", roomId );

        // The room has been created on the server but it's up to the
        // client to join their own room.
        mLogger->debug( "Sending JoinRoomReq, roomId={}", roomId );
        proto::ClientToServerMsg msg;
        proto::JoinRoomReq* req = msg.mutable_join_room_req();
        req->set_room_id( roomId );
        req->set_password( mCreatedRoomPassword );
        mServerConn->sendProtoMsg( msg );
    }
    else if( msg.has_create_room_failure_rsp() )
    {
        const proto::CreateRoomFailureRsp& rsp = msg.create_room_failure_rsp();
        const proto::CreateRoomFailureRsp::ResultType result = rsp.result();
        mLogger->debug( "CreateRoomFailureRsp: result={}", result );

        // Bring up a warning dialog.
        const QMap<proto::CreateRoomFailureRsp::ResultType,QString> lookup = {
                { proto::CreateRoomFailureRsp::RESULT_INVALID_SET_CODE, "A set code was invalid." },
                { proto::CreateRoomFailureRsp::RESULT_NAME_IN_USE, "The room name is already in use." } };
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
        const proto::JoinRoomFailureRsp& rsp = msg.join_room_failure_rsp();
        const proto::JoinRoomFailureRsp::ResultType result = rsp.result();
        mLogger->debug( "JoinRoomFailureRsp: result={}", result );

        // Bring up a warning dialog.
        const QMap<proto::JoinRoomFailureRsp::ResultType,QString> lookup = {
                { proto::JoinRoomFailureRsp::RESULT_ROOM_FULL, "The room is full." },
                { proto::JoinRoomFailureRsp::RESULT_INVALID_PASSWORD, "Invalid password." } };
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
        const proto::RoomOccupantsInfoInd& ind = msg.room_occupants_info_ind();
        mLogger->debug( "RoomOccupantsInfoInd: id={} players={}", ind.room_id(), ind.players_size() );

        // Set player states to stale before updating.
        for( int i = 0; i < mRoomStateAccumulator.getChairCount(); ++i )
        {
            if( mRoomStateAccumulator.hasPlayerState( i ) )
            {
                mRoomStateAccumulator.setPlayerState( i, PLAYER_STATE_STALE );
            }
        }

        // Accumulate room state.
        for( int i = 0; i < ind.players_size(); ++i )
        {
            const proto::RoomOccupantsInfoInd::Player& player = ind.players( i );
            mRoomStateAccumulator.setPlayerName( player.chair_index(), player.name() );
            mRoomStateAccumulator.setPlayerState( player.chair_index(), convertPlayerState( player.state() ) );
        }

        // Update the ticker player ready widget.
        mTickerPlayerReadyWidget->update( mRoomStateAccumulator );
        mTickerPlayerStatusWidget->update( mRoomStateAccumulator );
    }
    else if( msg.has_public_state_ind() )
    {
        processMessageFromServer( msg.public_state_ind() );
    }
    else if( msg.has_booster_draft_state_ind() )
    {
        processMessageFromServer( msg.booster_draft_state_ind() );
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
        const proto::PlayerCurrentPackInd& ind = msg.player_current_pack_ind();
        mCurrentPackId = ind.pack_id();
        mLogger->debug( "Current pack ind: {}", mCurrentPackId );

        // Create new cards and add to the draft list.
        mCardsList[CARD_ZONE_BOOSTER_DRAFT].clear();
        for( int i = 0; i < ind.cards_size(); ++i )
        {
            const proto::Card& card = ind.cards(i);
            CardDataSharedPtr cardDataSharedPtr = createCardData( card.set_code(), card.name() );
            mCardsList[CARD_ZONE_BOOSTER_DRAFT].push_back( cardDataSharedPtr );
        }
        processCardListChanged( CARD_ZONE_BOOSTER_DRAFT );

        if( mSettings->getBeepOnNewPack() )
        {
            mLogger->debug( "beeping on new pack" );
            QApplication::beep();
        }
    }
    else if( msg.has_player_named_card_selection_rsp() )
    {
        const proto::PlayerNamedCardSelectionRsp& rsp = msg.player_named_card_selection_rsp();
        mLogger->debug( "PlayerNamedCardSelelectionRsp: result={} pack={} card={}",
                rsp.result(), rsp.pack_id(), rsp.card() );
        if( rsp.result() )
        {
            CardDataSharedPtr cardDataSharedPtr = createCardData( rsp.card().set_code(), rsp.card().name() );
            processCardSelected( cardDataSharedPtr, false );
            mDraftSidebar->addCardSelectMessage( cardDataSharedPtr, false );
        }
        else
        {
            mLogger->notice( "Selection request failed for card={}", rsp.card() );
        }
    }
    else if( msg.has_player_indexed_card_selection_rsp() )
    {
        const proto::PlayerIndexedCardSelectionRsp& rsp = msg.player_indexed_card_selection_rsp();
        QList<int> indices;
        for( auto idx : rsp.indices() ) indices.append( idx );
        mLogger->debug( "PlayerIndexedCardSelectionRsp: result={} pack={} indices={}",
                rsp.result(), rsp.pack_id(), stringify( indices ) );
        if( rsp.result() )
        {
            for( auto card : rsp.cards() )
            {
                CardDataSharedPtr cardDataSharedPtr = createCardData( card.set_code(), card.name() );
                processCardSelected( cardDataSharedPtr, false );
                mDraftSidebar->addCardSelectMessage( cardDataSharedPtr, false );
            }
        }
        else
        {
            mLogger->notice( "Selection request failed for indices={}", stringify( indices ) );
        }
    }
    else if( msg.has_player_auto_card_selection_ind() )
    {
        const proto::PlayerAutoCardSelectionInd& ind = msg.player_auto_card_selection_ind();
        mLogger->debug( "AutoCardSelInd: type={} pack={} card={}",
                ind.type(), ind.pack_id(), ind.card() );

        CardDataSharedPtr cardDataSharedPtr = createCardData( ind.card().set_code(), ind.card().name() );

        // For non-sealed drafts, pop up a toast when a card is auto-selected.
        DraftConfigAdapter draftConfigAdapter( mRoomConfigAdapter->getDraftConfig() );
        if( !draftConfigAdapter.isSealedRound( mCurrentRound ) )
        {
            mToastOverlay->addToast( "<b>" + QString::fromStdString( ind.card().name() ) + "</b> " + tr("auto-selected") );
            mDraftSidebar->addCardSelectMessage( cardDataSharedPtr, true );
        }

        processCardSelected( cardDataSharedPtr, true );
    }
    else
    {
        mLogger->warn( "Unrecognized message: {}", msg.msg_case() );
    }
}


void
Client::processMessageFromServer( const proto::LoginRsp& rsp )
{
    mLogger->debug( "LoginRsp: result={}", rsp.result() );

    // Check success/fail and take appropriate action.
    if( rsp.result() == proto::LoginRsp::RESULT_SUCCESS )
    {
        // Save successful connection information to settings and update dialog.
        const QString server = mConnectDialog->getServer();
        mSettings->addConnectUserServer( server );
        mSettings->setConnectLastGoodServer( server );
        mSettings->setConnectLastGoodUsername( mConnectDialog->getUsername() );
        mConnectDialog->addKnownServer( server );

        // Trigger machine state transition.
        emit eventLoggedIn();
    }
    else
    {
        mLogger->notice( "Failed to login!" );

        // Disconnect.  A retry could be attempted here while connected
        // but it's probably not worth it.
        disconnectFromServer();

        if( rsp.result() == proto::LoginRsp::RESULT_FAILURE_NAME_IN_USE )
        {
            QMessageBox::warning( this, tr("Login Failed"),
                    tr("Could not log in to server - name already in use.  Reconnect and try again.") );
        }
        else if( rsp.result() == proto::LoginRsp::RESULT_FAILURE_INCOMPATIBLE_PROTO_VER )
        {
            const QString serverProtoStr = QString::fromStdString( stringify( mServerProtoVersion ) );
            const QString clientProtoStr = QString::fromStdString( stringify( CLIENT_PROTOVERSION ) );
            mLogger->warn( "Protocol incompatibility with server (server rejected login): server={}, client={}",
                    serverProtoStr, clientProtoStr );
            QMessageBox::critical( this, tr("Protocol Mismatch"),
                    tr("The server rejected your client login due to incompatibility."
                       "<br>(Server protocol version %1, client protocol version %2)"
                       "<p>Visit the project <a href=\"%3\">download area</a> for a compatible client version.")
                       .arg( serverProtoStr )
                       .arg( clientProtoStr )
                       .arg( WebServerInterface::getRedirectDownloadsUrl( mSettings->getWebServiceBaseUrl() ) ) );
        }
        else
        {
            QMessageBox::warning( this, tr("Login Failed"),
                    // Other errors should be less common, so use a generic response.
                    tr("Could not log in to server - error %1.  Reconnect and try again.").arg( rsp.result() ) );
        }
    }
}


void
Client::processMessageFromServer( const proto::RoomCapabilitiesInd& ind )
{
    mLogger->debug( "RoomCapabilitiesInd" );
    std::vector<RoomCapabilitySetItem> sets;
    for( int i = 0; i < ind.sets_size(); ++i )
    {
        const proto::RoomCapabilitiesInd::SetCapability& set = ind.sets( i );
        mLogger->debug( "  code={} name={} boosterGen={}", set.code(), set.name(), set.booster_generation() );
        RoomCapabilitySetItem setItem = { set.code(), set.name(), set.booster_generation() };
        sets.push_back( setItem );
    }
    mCreateRoomWizard->setRoomCapabilitySets( sets );
}


void
Client::processMessageFromServer( const proto::JoinRoomSuccessRspInd& rspInd )
{
    mLogger->debug( "JoinRoomSuccessRspInd: roomId={} rejoin={} chairIdx={}", rspInd.room_id(), rspInd.rejoin(), rspInd.chair_idx() );

    // Clear out all zones when joining a room.
    for( auto zone : gCardZoneTypeArray )
    {
        mCardsList[zone].clear();
        processCardListChanged( zone );
    }

    mChairIndex = rspInd.chair_idx();
    mRoomRejoined = rspInd.rejoin();
    mAwaitingRoomStateAfterRejoin = rspInd.rejoin();
    mRoomConfigAdapter = std::make_shared<RoomConfigAdapter>( rspInd.room_id(), rspInd.room_config(),
           mLoggingConfig.createChildConfig( "roomconfigadapter" ) );

    mRoomStateAccumulator.reset();
    mRoomStateAccumulator.setChairCount( mRoomConfigAdapter->getChairCount() );

    // Trigger state machine update.
    emit eventJoinedRoom();
}


void
Client::processMessageFromServer( const proto::PlayerInventoryInd& ind )
{
    mLogger->debug( "PlayerInventoryInd" );

    // Iterate over each zone, processing differences between what is
    // currently in place and the final result in order to reduce load
    // of creating and loading new card data objects.
    for( proto::Zone invZone : gInventoryZoneArray )
    {
        CardZoneType zone = convertCardZone( invZone );
        mLogger->debug( "processing zone: {}", stringify( zone ) );

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
            const proto::PlayerInventoryInd::DraftedCard& draftedCard =ind.drafted_cards( i );
            const proto::Card& card = draftedCard.card();
            if( draftedCard.zone() == invZone )
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
            const proto::PlayerInventoryInd::BasicLandQuantity& basicLandQty = ind.basic_land_qtys( i );
            if( basicLandQty.zone() == invZone )
            {
                mLogger->debug( "basic land ({}) ({}): {}", stringify( basicLandQty.basic_land() ),
                        stringify( invZone ), basicLandQty.quantity() );
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
Client::processMessageFromServer( const proto::PublicStateInd& ind )
{
    mLogger->debug( "PublicStateInd: packId={}, activeChairIndex={}, timeRemainingSecs={}, millisUntilNextSec={}",
           ind.pack_id(), ind.active_chair_index(), ind.time_remaining_secs(), ind.millis_until_next_sec() );

    // Iterate through the card states and do the following:
    //   - Create new cards and add to the draft list
    //   - Create selected cards map
    //   - Notify if a new opponent selection is detected 
    mCardsList[CARD_ZONE_GRID_DRAFT].clear();
    QMap<CardDataSharedPtr,SelectedCardData> selectedCards;
    static QSet<int> selectedIndices;
    for( int i = 0; i < ind.card_states_size(); ++i )
    {
        const proto::PublicStateInd::CardState& state = ind.card_states( i );
        mLogger->debug( "  card={}, selectedChairIndex={}, selectedOrder={}",
                state.card().name(), state.selected_chair_index(), state.selected_order() );

        CardDataSharedPtr cardDataSharedPtr = createCardData( state.card().set_code(), state.card().name() );
        mCardsList[CARD_ZONE_GRID_DRAFT].push_back( cardDataSharedPtr );

        const int selectedChairIndex = state.selected_chair_index();
        if( selectedChairIndex >= 0 )
        {
            // Update selected cards map.
            SelectedCardData selCardData;
            selCardData.chairIndex = selectedChairIndex;
            selCardData.isOpponent = (selectedChairIndex != mChairIndex);
            if( mRoomStateAccumulator.hasPlayerName( selectedChairIndex ) )
            {
                selCardData.playerName = mRoomStateAccumulator.getPlayerName( selectedChairIndex );
            }
            selectedCards.insert( cardDataSharedPtr, selCardData );

            // Update selected indices tracking set and notify on an
            // opponent's selection.
            if( !selectedIndices.contains( i ) && (selectedChairIndex != mChairIndex) )
            {
                mDraftSidebar->addGameMessage( tr("%1 selected %2")
                                               .arg( QString::fromStdString( mRoomStateAccumulator.getPlayerName( selectedChairIndex ) ) )
                                               .arg( QString::fromStdString( state.card().name() ) ), DraftSidebar::MESSAGE_LEVEL_LOW );
            }
            selectedIndices.insert( i );
        }
        else
        {
            selectedIndices.remove( i );
        }
    }
    processCardListChanged( CARD_ZONE_GRID_DRAFT );

    // Set any unselectables for the grid draft zone.
    mLeftCommanderPane->setSelectedCards( CARD_ZONE_GRID_DRAFT, selectedCards );
    mRightCommanderPane->setSelectedCards( CARD_ZONE_GRID_DRAFT, selectedCards );

    // Re-align timer.
    mServerAlignedDraftTimer->stop();
    mServerAlignedDraftTimer->setSingleShot( true );
    mServerAlignedDraftTimer->start( ind.millis_until_next_sec() );

    // If current player is becoming active, beep on new pack if configured.
    if( (ind.active_chair_index() == mChairIndex) &&
        !mRoomStateAccumulator.isPublicDraftPlayerActive( mChairIndex ) &&
        mSettings->getBeepOnNewPack() )
    {
        mLogger->debug( "beeping on new pack" );
        QApplication::beep();
    }

    // Update room state.
    mRoomStateAccumulator.setPublicDraftType( true );
    mRoomStateAccumulator.clearPublicDraftPlayersActive();
    mRoomStateAccumulator.setPublicDraftPlayerActive( ind.active_chair_index() );
    for( int i = 0; i < mRoomStateAccumulator.getChairCount(); ++i )
    {
        mRoomStateAccumulator.setPlayerTimeRemaining( i, (i == ind.active_chair_index()) ? ind.time_remaining_secs() : -1 );
    }

    // Only need to update left CommanderPane because it's the only one initialized with a draft tab.
    const int timeRemaining = mRoomStateAccumulator.getPlayerTimeRemaining( mChairIndex );
    mLeftCommanderPane->setDraftAlert( (timeRemaining > 0) && (timeRemaining <= 10) );
    mLeftCommanderPane->setDraftTimeRemaining( timeRemaining );

    // Update the ticker player status widget with room state.
    mTickerPlayerStatusWidget->update( mRoomStateAccumulator );
}


void
Client::processMessageFromServer( const proto::BoosterDraftStateInd& ind )
{
    mLogger->debug( "BoosterDraftStateInd: chairs={}", ind.chairs_size() );

    // Update room state.
    mRoomStateAccumulator.setPublicDraftType( false );
    for( int i = 0; i < ind.chairs_size(); ++i )
    {
        const proto::BoosterDraftStateInd::Chair& chair = ind.chairs( i );
        const unsigned int queuedPacks = chair.queued_packs();
        const unsigned int timeRemaining = chair.time_remaining();
        const int chairIndex = chair.chair_index();

        mLogger->debug( "BoosterDraftStateInd: chair={} queuedPacks={}, timeRemaining={}",
                chairIndex, queuedPacks, timeRemaining );

        const bool timerActive = mRoundTimerEnabled && (queuedPacks > 0);

        mRoomStateAccumulator.setPlayerPackQueueSize( chair.chair_index(), queuedPacks );
        mRoomStateAccumulator.setPlayerTimeRemaining( chair.chair_index(), timerActive ? timeRemaining : -1 );

        if( chairIndex == mChairIndex )
        {
            if( timerActive )
            {
                // Only need to update left CommanderPane because it's the only one initialized with a draft tab.
                mLeftCommanderPane->setDraftAlert( (timeRemaining > 0) && (timeRemaining <= 10) );
                mLeftCommanderPane->setDraftTimeRemaining( timeRemaining );
                mLeftCommanderPane->setDraftQueuedPacks( queuedPacks );
            }
            else
            {
                // Only need to update left CommanderPane because it's the only one initialized with a draft tab.
                mLeftCommanderPane->setDraftAlert( false );
                mLeftCommanderPane->setDraftTimeRemaining( -1 );
                mLeftCommanderPane->setDraftQueuedPacks( -1 );
            }
        }
    }

    // Update the ticker player status widget with room state.
    mTickerPlayerStatusWidget->update( mRoomStateAccumulator );

}


void
Client::processMessageFromServer( const proto::RoomChairsDeckInfoInd& ind )
{
    mLogger->debug( "RoomChairsDeckInfoInd: chairs={}", ind.chairs_size() );

    // Log message contents.
    for( int i = 0; i < ind.chairs_size(); ++i )
    {
        const proto::RoomChairsDeckInfoInd::Chair& chair = ind.chairs( i );
        mLogger->debug( "RoomChairsDeckInfoInd: chair={} cockatriceHash={}, mwsHash={}",
                chair.chair_index(), chair.cockatrice_hash(), chair.mws_hash() );
    }

    // Accumulate room state with received hashes.
    for( int i = 0; i < ind.chairs_size(); ++i )
    {
        const proto::RoomChairsDeckInfoInd::Chair& chair = ind.chairs( i );
        mRoomStateAccumulator.setPlayerCockatriceHash( chair.chair_index(), chair.cockatrice_hash() );
    }

    // Update the ticker player hashes widget.
    mTickerPlayerHashesWidget->update( mRoomStateAccumulator );
}


void
Client::processMessageFromServer( const proto::RoomStageInd& ind )
{
    mLogger->debug( "RoomStageInd, stage={}", ind.stage() );

    // Always stop the server-aligned draft timer regardless of stage type;
    // it will be restarted as necessary if there is a post-round timer or
    // with draft-based messages after this one.
    mServerAlignedDraftTimer->stop();

    if( ind.stage() == proto::RoomStageInd::STAGE_COMPLETE )
    {
        // Clear out draft card area.
        mCardsList[CARD_ZONE_BOOSTER_DRAFT].clear();
        processCardListChanged( CARD_ZONE_BOOSTER_DRAFT );
        mCardsList[CARD_ZONE_GRID_DRAFT].clear();
        processCardListChanged( CARD_ZONE_GRID_DRAFT );

        // Allow the draft tab to disappear.
        mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_BOOSTER_DRAFT, true );
        mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_GRID_DRAFT, true );

        mLeftCommanderPane->setDraftActive( false );

        // Pop up a toast with context based on rejoin.
        if( mAwaitingRoomStateAfterRejoin )
        {
            mToastOverlay->addToast( tr("Rejoined draft (draft complete)") );
        }
        else
        {
            DraftConfigAdapter draftConfigAdapter( mRoomConfigAdapter->getDraftConfig() );
            const bool isSealed = draftConfigAdapter.isSealedRound( mCurrentRound );
            if( !isSealed )
            {
                mToastOverlay->addToast( tr("Draft complete") );
            }
        }

        // In the ticker, notify of draft complete and change to the hashes widget.
        clearTicker();
        mTickerWidget->enqueueOneShotWidget( new QLabel( tr("Draft Complete") ) );
        mTickerWidget->addPermanentWidget( mTickerPlayerHashesWidget );

        mDraftStatusLabel->setText( tr("Draft complete") );

        mRoomStageRunning = false;
    }
    else if( ind.stage() == proto::RoomStageInd::STAGE_RUNNING )
    {
        const unsigned int currentRound = ind.round_info().round();
        const int postRoundTimeRemainingMillis = ind.round_info().post_round_time_remaining_millis();

        mLogger->debug( "currentRound={}, postRoundTimeRemainingMillis={}", currentRound, postRoundTimeRemainingMillis );

        const bool postRoundTimerStarted = !mPostRoundTimerActive && (postRoundTimeRemainingMillis > 0);
        const bool postRoundTimerFinished = mPostRoundTimerActive && (postRoundTimeRemainingMillis <= 0);
        mPostRoundTimerActive = (postRoundTimeRemainingMillis > 0);

        // Set post-round timer seconds, which is millis rounded up to nearest second.
        const int postRoundTimeRemainingSecs = ((postRoundTimeRemainingMillis + 999) / 1000);
        mRoomStateAccumulator.setPostRoundTimeRemainingSecs( postRoundTimeRemainingSecs );

        // If this is a post-round timer stage indication, align and start timer.
        if( postRoundTimeRemainingMillis > 0 )
        {
            mServerAlignedDraftTimer->setSingleShot( true );
            mServerAlignedDraftTimer->start( postRoundTimeRemainingMillis % 1000 );
        }

        // If the draft just began, do some UI updates.
        if( !mRoomStageRunning )
        {
            // All players ready - get rid of the "Ready" splash dialog and the ready ticker widget.
            if( mReadySplash )
            {
                mReadySplash->deleteLater();
                mReadySplash = nullptr;
            }

            // Switch the view to the Draft tab.
            mCentralTabWidget->setCurrentWidget( mDraftViewWidget );
        }

        // If the draft just began or the round has been updated, make
        // round-type specific UI updates.
        if( !mRoomStageRunning || (mCurrentRound != currentRound) )
        {
            if( mRoomConfigAdapter )
            {
                DraftConfigAdapter draftConfigAdapter( mRoomConfigAdapter->getDraftConfig() );
                if( draftConfigAdapter.isBoosterRound( currentRound ) )
                {
                    // When a booster round starts, ensure the draft zone
                    // doesn't go away while drafting and switch there immediately.
                    mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_BOOSTER_DRAFT, false );
                    mLeftCommanderPane->setCurrentCardZone( CARD_ZONE_BOOSTER_DRAFT );
                }
                if( draftConfigAdapter.isGridRound( currentRound ) )
                {
                    // When a grid round starts, ensure the draft zone
                    // doesn't go away while drafting and switch there immediately.
                    mLeftCommanderPane->setHideIfEmpty( CARD_ZONE_GRID_DRAFT, false );
                    mLeftCommanderPane->setCurrentCardZone( CARD_ZONE_GRID_DRAFT );
                }
                else if( draftConfigAdapter.isSealedRound( currentRound ) )
                {
                    // Set the card zone to auto.  Even though it's normally
                    // hidden, this call will explicitly show it.
                    mLeftCommanderPane->setCurrentCardZone( CARD_ZONE_AUTO );
                }
            }
            else
            {
                mLogger->warn( "room configuration not initialized!" );
            }

            // Pop up a toast with context based on rejoin.
            QString statusStr;
            const int numRounds = mRoomConfigAdapter->getDraftConfig().rounds_size();
            if( numRounds > 1 )
            {
                statusStr = !mAwaitingRoomStateAfterRejoin ? tr("Draft round %1 started").arg( currentRound + 1 )
                                                           : tr("Rejoined draft (draft round %1 in progress)").arg( currentRound + 1 );
            }
            else
            {
                statusStr = !mAwaitingRoomStateAfterRejoin ? tr("Draft started") : tr("Rejoined draft (in progress)");
            }
            mToastOverlay->addToast( statusStr );
            mDraftSidebar->addGameMessage( statusStr );

            mCurrentRound = currentRound;
        }

        if( mRoomConfigAdapter )
        {
            mRoomStateAccumulator.setPassDirection( mRoomConfigAdapter->getPassDirection( currentRound ) );
            mRoundTimerEnabled = (mRoomConfigAdapter->getBoosterRoundSelectionTime( currentRound ) > 0);
        }
        else
        {
            mLogger->warn( "room configuration not initialized!" );
        }

        mTickerPlayerStatusWidget->update( mRoomStateAccumulator );
        mTickerPostRoundTimerWidget->setSecondsRemaining( mRoomStateAccumulator.getPostRoundTimeRemainingSecs() );
  
        // Update draft status label.
        mDraftStatusLabel->setText( tr("Draft round %1").arg( currentRound + 1 ) );

        // Clear the ticker if this is the first 'running' indication or
        // if the post-round timer has started or finished.
        if( !mRoomStageRunning || postRoundTimerStarted || postRoundTimerFinished )
        {
            clearTicker();
        }

        // If this is the first 'running' indication or the post-round timer
        // has finished, bring up the player status widget and inform the draft
        // CommanderPane to be active.  Otherwise if the post-round timer
        // started, bring up its widget and inactivate the draft pane.
        if( !mRoomStageRunning || postRoundTimerFinished )
        {
            mLeftCommanderPane->setDraftActive( true );
            mTickerWidget->addPermanentWidget( mTickerPlayerStatusWidget );
        }
        else if( postRoundTimerStarted )
        {
            mLeftCommanderPane->setDraftActive( false );
            mTickerWidget->addPermanentWidget( mTickerPostRoundTimerWidget );
        }

        mRoomStageRunning = true;
    }
    else
    {
        // Draft not complete and not running (unexpected).
        mLogger->warn( "ignoring RoomStageInd" );
    }

    // Clear rejoin flag.
    mAwaitingRoomStateAfterRejoin = false;
}


void
Client::processCardSelected( const CardDataSharedPtr& cardDataSharedPtr, bool autoSelected )
{
    // Card was selected, empty out draft lists.  (Safe even for non-draft tabs.)
    mCardsList[CARD_ZONE_BOOSTER_DRAFT].clear();
    processCardListChanged( CARD_ZONE_BOOSTER_DRAFT );
    mCardsList[CARD_ZONE_GRID_DRAFT].clear();
    processCardListChanged( CARD_ZONE_GRID_DRAFT );

    // Create new card data for the indicated card and add to the
    // destination zone.  Often the card data has already been created
    // and could be reused from the draft card list, but not always.
    // Creating is the simplest thing to do.
    CardZoneType destZone = autoSelected ? CARD_ZONE_AUTO : mDraftedCardDestZone;
    mCardsList[destZone].push_back( cardDataSharedPtr );

    mUnsavedChanges = true;

    processCardListChanged( destZone );
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

    // Can't move cards into the draft or auto zones.
    if( (destCardZone == CARD_ZONE_BOOSTER_DRAFT) ||
        (destCardZone == CARD_ZONE_GRID_DRAFT) ||
        (destCardZone == CARD_ZONE_AUTO) )
    {
        return;
    }

    // Moves from the draft zone are special - a request to the server needs
    // to go out before the move is allowed.
    if( srcCardZone == CARD_ZONE_BOOSTER_DRAFT )
    {
        mDraftedCardDestZone = destCardZone;
        mLogger->debug( "send draft selection" );
        proto::ClientToServerMsg msg;
        proto::PlayerNamedCardSelectionReq* req = msg.mutable_player_named_card_selection_req();
        req->set_pack_id( mCurrentPackId );

        // Init card data.  Set code must be what the server originally sent.
        proto::Card* card = req->mutable_card();
        card->set_name( cardData->getName() );
        card->set_set_code( mCardServerSetCodeMap->contains( cardData.get() ) ? mCardServerSetCodeMap->value( cardData.get() )
                                                                              : cardData->getSetCode() );
        proto::Zone destInventoryZone = convertCardZone( destCardZone );
        req->set_zone( destInventoryZone );
        mServerConn->sendProtoMsg( msg );
        return;
    }

    // Send an inventory update message to server.
    if( mServerConn->state() == QTcpSocket::ConnectedState )
    {
        mLogger->trace( "sendPlayerInventoryUpdateInd" );
        proto::ClientToServerMsg msg;
        proto::PlayerInventoryUpdateInd* ind = msg.mutable_player_inventory_update_ind();
        addPlayerInventoryUpdateDraftedCardMove( ind, cardData, srcCardZone, destCardZone );
        mServerConn->sendProtoMsg( msg );
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

    mUnsavedChanges = true;

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

    // Can't move all cards into or out of a draft zone.
    if( srcCardZone == CARD_ZONE_BOOSTER_DRAFT ) return;
    if( destCardZone == CARD_ZONE_BOOSTER_DRAFT ) return;
    if( srcCardZone == CARD_ZONE_GRID_DRAFT ) return;
    if( destCardZone == CARD_ZONE_GRID_DRAFT ) return;

    // Send an inventory update message to server.
    if( mServerConn->state() == QTcpSocket::ConnectedState )
    {
        mLogger->trace( "sendPlayerInventoryUpdateInd" );
        proto::ClientToServerMsg msg;
        proto::PlayerInventoryUpdateInd* ind = msg.mutable_player_inventory_update_ind();
        for( auto cardData : mCardsList[srcCardZone] )
        {
            addPlayerInventoryUpdateDraftedCardMove( ind, cardData, srcCardZone, destCardZone );
        }
        mServerConn->sendProtoMsg( msg );
    }

    // Put all source cards onto dest list, then clear source list.
    mCardsList[destCardZone].append( mCardsList[srcCardZone] );
    mCardsList[srcCardZone].clear();

    processCardListChanged( srcCardZone );
    processCardListChanged( destCardZone );
}


void
Client::handleCardPreselected( const CardDataSharedPtr& cardData )
{
    mLogger->debug( "handleCardPreselected: {}", cardData->getName() );
    proto::ClientToServerMsg msg;
    proto::PlayerNamedCardPreselectionInd* ind = msg.mutable_player_named_card_preselection_ind();
    ind->set_pack_id( mCurrentPackId );

    // Init card data.  Set code must be what the server originally sent.
    proto::Card* card = ind->mutable_card();
    card->set_name( cardData->getName() );
    card->set_set_code( mCardServerSetCodeMap->contains( cardData.get() ) ? mCardServerSetCodeMap->value( cardData.get() )
                                                                          : cardData->getSetCode() );
    mServerConn->sendProtoMsg( msg );
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
Client::handleCardIndicesSelected( const CardZoneType& srcCardZone, const QList<int>& indices )
{
    if( srcCardZone != CARD_ZONE_GRID_DRAFT )
    {
        mLogger->warn( "handleCardIndicesSelected from unexpected zone: {}", srcCardZone );
        return;
    }

    CommanderPane *senderCommanderPane = qobject_cast<CommanderPane *>( QObject::sender() );
    mLogger->debug( "handleCardIndicesSelected: {} {}->?", stringify(indices), srcCardZone );

    // Figure out the destination zone.
    CommanderPane *destCommanderPane = senderCommanderPane == mLeftCommanderPane ? mRightCommanderPane : mLeftCommanderPane;
    const CardZoneType destCardZone = destCommanderPane->getCurrentCardZone();
    mLogger->debug( "handleCardIndicesSelected: {} {}->{}", stringify(indices), srcCardZone, destCardZone );

    mDraftedCardDestZone = destCardZone;
    mLogger->debug( "send draft selection" );
    proto::ClientToServerMsg msg;
    proto::PlayerIndexedCardSelectionReq* req = msg.mutable_player_indexed_card_selection_req();
    req->set_pack_id( mCurrentPackId );
    for( auto idx : indices )
    {
        req->add_indices( idx );
    }
    proto::Zone destInventoryZone = convertCardZone( destCardZone );
    req->set_zone( destInventoryZone );
    mServerConn->sendProtoMsg( msg );
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
    if( mServerConn->state() == QTcpSocket::ConnectedState )
    {
        mLogger->trace( "sendPlayerInventoryUpdateInd" );
        proto::ClientToServerMsg msg;
        proto::PlayerInventoryUpdateInd* ind = msg.mutable_player_inventory_update_ind();
        for( BasicLandType basic : gBasicLandTypeArray )
        {
            int diff = qtys.getQuantity( basic ) - mBasicLandQtysMap[cardZone].getQuantity( basic );
            if( diff != 0 )
            {
                mLogger->debug( "  {}: {}", stringify( basic ), diff );
                proto::PlayerInventoryUpdateInd::BasicLandAdjustment* basicLandAdj =
                        ind->add_basic_land_adjustments();
                basicLandAdj->set_basic_land( convertBasicLand( basic ) );
                basicLandAdj->set_zone( convertCardZone( cardZone ) );
                basicLandAdj->set_adjustment( diff );
                mServerConn->sendProtoMsg( msg );
            }
        }
    }

    // Update local quantities.
    mBasicLandQtysMap[cardZone] = qtys;

    mUnsavedChanges = true;
}


void
Client::handleJoinRoomRequest( int roomId, const QString& password )
{
    // Check that we are connected and logged in, but not already in a room.
    if( mStateMachine->configuration().contains( mStateNotInRoom ) )
    {
        // Send request to join the room.
        mLogger->debug( "Sending JoinRoomReq, roomId={}", roomId );
        proto::ClientToServerMsg msg;
        proto::JoinRoomReq* req = msg.mutable_join_room_req();
        req->set_room_id( roomId );
        req->set_password( password.toStdString() );
        mServerConn->sendProtoMsg( msg );
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
        mCreateRoomWizard->restart();
        int result = mCreateRoomWizard->exec();
        if( result == QDialog::Accepted )
        {
            const QString passwordStr = mCreateRoomWizard->getPassword();

            proto::ClientToServerMsg msg;
            proto::CreateRoomReq* req = msg.mutable_create_room_req();
            if( !passwordStr.isEmpty() )
            {
                mCreatedRoomPassword = passwordStr.toStdString();
                req->set_password( mCreatedRoomPassword );
            }
            proto::RoomConfig* roomConfig = req->mutable_room_config();
            mCreateRoomWizard->fillRoomConfig( roomConfig );

            mLogger->debug( "sending CreateRoomReq" );
            mServerConn->sendProtoMsg( msg );
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
        proto::ClientToServerMsg msg;
        proto::ChatMessageInd* ind = msg.mutable_chat_message_ind();
        ind->set_scope( proto::CHAT_SCOPE_ALL );
        ind->set_text( text.toStdString() );
        mServerConn->sendProtoMsg( msg );
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
        proto::ClientToServerMsg msg;
        proto::PlayerReadyInd* ind = msg.mutable_player_ready_ind();
        ind->set_ready( ready );
        mServerConn->sendProtoMsg( msg );
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
        int result = QMessageBox::question( this, tr("Confirm Leave Room"),
                                   tr("Are you sure you want to leave the room?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No );
        if( result == QMessageBox::Yes )
        {
            mLogger->debug( "Sending RoomDepartInd" );
            proto::ClientToServerMsg msg;
            proto::DepartRoomInd* ind = msg.mutable_depart_room_ind();
            Q_UNUSED( ind );
            mServerConn->sendProtoMsg( msg );

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
        proto::ClientToServerMsg msg;
        proto::ChatMessageInd* ind = msg.mutable_chat_message_ind();
        ind->set_scope( proto::CHAT_SCOPE_ROOM );
        ind->set_text( text.toStdString() );
        mServerConn->sendProtoMsg( msg );
    }
    else
    {
        mLogger->debug( "room chat message ignored (invalid state)" );
    }
}


void
Client::handleServerAlignedDraftTimerTimeout()
{
    mLogger->debug( "server-aligned timer expired, isSingleShot={}", mServerAlignedDraftTimer->isSingleShot() );

    // Deduct a second from chairs with time remaining.
    for( int i = 0; i < mRoomStateAccumulator.getChairCount(); ++i )
    {
        if( mRoomStateAccumulator.hasPlayerTimeRemaining( i ) )
        {
            int timeRemaining = mRoomStateAccumulator.getPlayerTimeRemaining( i );
            if( timeRemaining > 0 )
            {
                mRoomStateAccumulator.setPlayerTimeRemaining( i, timeRemaining - 1 );
            }
        }
    }

    // Decrement post-round timer if active.
    int postRoundTimerSecs = mRoomStateAccumulator.getPostRoundTimeRemainingSecs();
    if( postRoundTimerSecs > 0 )
    {
        postRoundTimerSecs--;
        mLogger->info( "postRoundTimerSecs={}", postRoundTimerSecs );
        mRoomStateAccumulator.setPostRoundTimeRemainingSecs( postRoundTimerSecs );
        mTickerPostRoundTimerWidget->setSecondsRemaining( postRoundTimerSecs );
    }

    // Only need to update left CommanderPane because it's the only one initialized with a draft tab.
    const int timeRemaining = mRoomStateAccumulator.getPlayerTimeRemaining( mChairIndex );
    mLeftCommanderPane->setDraftAlert( (timeRemaining > 0) && (timeRemaining <= 10) );
    mLeftCommanderPane->setDraftTimeRemaining( timeRemaining );
    mTickerPlayerStatusWidget->update( mRoomStateAccumulator );

    // The timer is first started with a single-shot to handle a partial second.
    // After that it should be 1-second continuous.
    if( mServerAlignedDraftTimer->isSingleShot() )
    {
        mServerAlignedDraftTimer->setInterval( 1000 );
        mServerAlignedDraftTimer->setSingleShot( false );
        mServerAlignedDraftTimer->start();
    }
}


void
Client::addPlayerInventoryUpdateDraftedCardMove( proto::PlayerInventoryUpdateInd* ind,
                                                 const CardDataSharedPtr&         cardData,
                                                 const CardZoneType&              srcCardZone,
                                                 const CardZoneType&              destCardZone )
{
    proto::PlayerInventoryUpdateInd::DraftedCardMove* move =
            ind->add_drafted_card_moves();

    // Init card data.  Set code must be what the server originally sent.
    proto::Card* card = move->mutable_card();
    card->set_name( cardData->getName() );
    card->set_set_code( mCardServerSetCodeMap->contains( cardData.get() ) ? mCardServerSetCodeMap->value( cardData.get() )
                                                                          : cardData->getSetCode() );
    move->set_zone_from( convertCardZone( srcCardZone ) );
    move->set_zone_to( convertCardZone( destCardZone ) );
}


void
Client::handleSocketError( QAbstractSocket::SocketError socketError )
{
    mLogger->debug( "socket error occurred: {}", mServerConn->errorString() );

    emit eventConnectionError();

    // Ensure the socket is reset.
    mServerConn->abort();

    if( socketError == QAbstractSocket::RemoteHostClosedError )
    {
        QMessageBox::warning( this, tr("Disconnected"),
                tr("The remote host closed the connection.") );
    }
    else if( socketError == QAbstractSocket::HostNotFoundError )
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
                tr("The following error occurred: %1.").arg(mServerConn->errorString()));
    }

    // Retry the connection action.
    QTimer::singleShot(0, this, SLOT(handleConnectAction()));
}


void
Client::handleKeepAliveTimerTimeout()
{
    mLogger->debug( "timer expired - sending keepalive msg to server" );
    proto::ClientToServerMsg msg;
    msg.mutable_keep_alive_ind();
    mServerConn->sendProtoMsg( msg );
}


void
Client::handleConnectAction()
{
    int result = mConnectDialog->exec();
    if( result == QDialog::Accepted )
    {
        connectToServer( mConnectDialog->getServerHost(), mConnectDialog->getServerPort() );
    }
}


void
Client::handleDisconnectAction()
{
    int result = QMessageBox::question( this, tr("Confirm Disconnect"),
                               tr("Are you sure you want to disconnect?"),
                               QMessageBox::Yes | QMessageBox::No,
                               QMessageBox::No );
    if( result == QMessageBox::Yes )
    {
        disconnectFromServer();
    }
}


void
Client::handleDeckStatsAction()
{
    mLogger->debug( "Analyzing deck" );

    Decklist decklist = buildDecklist();
    DeckStatsLauncher* launcher = new DeckStatsLauncher( this );
    launcher->launch( decklist );  // launcher deletes itself when complete
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

    // Save the decklist.
    QTextStream out( &file );
    Decklist decklist = buildDecklist();
    out << QString::fromStdString( decklist.getFormattedString( Decklist::FORMAT_DEC ) );

    mUnsavedChanges = false;
}


void
Client::handleCheckClientUpdateAction()
{
    mLogger->debug( "Checking for client update" );
    ClientUpdateChecker* checker = new ClientUpdateChecker(
            mLoggingConfig.createChildConfig( "clientupdatechecker" ), this );
    checker->check( mSettings->getWebServiceBaseUrl(), QString::fromStdString( gClientVersion ) );
    // checker deletes itself when complete
}


void
Client::handleUpdateCardsAction()
{
    // Start the updater.  If an update occurs it will emit a signal that
    // is hooked up to our update slot.
    mAllSetsUpdater->start();
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
Client::clearTicker()
{
    // Clear out all permanent widgets from the ticker.  They are not
    // deleted because they are managed by this class.
    QWidget* widget = mTickerWidget->takePermanentWidgetAt( 0 );
    while( widget != nullptr )
    {
        widget = mTickerWidget->takePermanentWidgetAt( 0 );
    }
}


Decklist
Client::buildDecklist()
{
    Decklist decklist;

    // Add basic lands to the decklist.
    std::set<std::string> priorityCardNames;
    for( auto basic : gBasicLandTypeArray )
    {
        const std::string cardName = mBasicLandCardDataMap.getCardData( basic )->getName();
        decklist.addCard( cardName, Decklist::ZONE_MAIN,
                mBasicLandQtysMap[CARD_ZONE_MAIN].getQuantity( basic ) );
        decklist.addCard( cardName, Decklist::ZONE_SIDEBOARD,
                mBasicLandQtysMap[CARD_ZONE_SIDEBOARD].getQuantity( basic ) );
        priorityCardNames.insert( cardName );
    }
    decklist.setPriorityCardNames( priorityCardNames );

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

    return decklist;
}

