#include "ServerViewWidget.h"

#include <QGridLayout>
#include <QBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QLabel>
#include <QTreeWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QInputDialog>
#include <QHeaderView>

#include "RoomConfigAdapter.h"

static const int ROOM_TREE_COLUMN_NAME    = 0;
static const int ROOM_TREE_COLUMN_PLAYERS = 2;

static const QString EMPTY_ANNOUNCEMENTS_STR = QT_TR_NOOP("<center>-- No Server Announcements --</center>");

ServerViewWidget::ServerViewWidget( const Logging::Config& loggingConfig,
                                    QWidget*               parent )
  : QWidget( parent ),
    mJoinRoomEnabled( false ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    QGridLayout* outerLayout = new QGridLayout();
    setLayout( outerLayout );

    mAnnouncements = new QTextEdit();
    mAnnouncements->setReadOnly( true );
    mAnnouncements->setText( EMPTY_ANNOUNCEMENTS_STR );

    QGroupBox* roomsGroupBox = new QGroupBox( "Draft Rooms" );
    QVBoxLayout* roomsLayout = new QVBoxLayout();
    roomsGroupBox->setLayout( roomsLayout );

    mRoomTreeWidget = new QTreeWidget();
    mRoomTreeWidget->setRootIsDecorated( false );
    mRoomTreeWidget->setColumnCount( 4 );
    mRoomTreeWidget->setHeaderLabels( { "Name", "Sets", "Players", "Password" } );
    mRoomTreeWidget->header()->resizeSection( ROOM_TREE_COLUMN_NAME, 350 );
    connect( mRoomTreeWidget, &QTreeWidget::itemSelectionChanged, this, &ServerViewWidget::handleRoomTreeSelectionChanged );

    mRoomTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( mRoomTreeWidget, &QTreeWidget::customContextMenuRequested, this, &ServerViewWidget::handleRoomTreeCustomContextMenu );

    QHBoxLayout* roomsButtonsLayout = new QHBoxLayout();

    mJoinRoomButton = new QPushButton( "&Join Room" );
    mJoinRoomButton->setEnabled( mJoinRoomEnabled );
    connect( mJoinRoomButton, &QPushButton::clicked, this, &ServerViewWidget::handleJoinRoomButtonClicked );

    mCreateRoomButton = new QPushButton( "&Create Room" );
    mCreateRoomButton->setEnabled( false );
    connect( mCreateRoomButton, &QPushButton::clicked, this, &ServerViewWidget::handleCreateRoomButtonClicked );

    roomsButtonsLayout->addStretch();
    roomsButtonsLayout->addWidget( mJoinRoomButton );
    roomsButtonsLayout->addSpacing( 30 );
    roomsButtonsLayout->addWidget( mCreateRoomButton );
    roomsButtonsLayout->addStretch();

    roomsLayout->addWidget( mRoomTreeWidget );
    roomsLayout->addLayout( roomsButtonsLayout );

    mUsersListWidget = new QListWidget();
    mUsersListWidget->setSelectionMode( QAbstractItemView::NoSelection );
    mUsersListWidget->setFocusPolicy( Qt::NoFocus );

    QGroupBox* chatGroupBox = new QGroupBox( "Server Chat" );
    QVBoxLayout* chatLayout = new QVBoxLayout();
    chatGroupBox->setLayout( chatLayout );

    mChatListWidget = new QListWidget();
    mChatListWidget->setSelectionMode( QAbstractItemView::NoSelection );
    mChatListWidget->setFocusPolicy( Qt::NoFocus );
    mChatLineEdit = new QLineEdit();
    connect( mChatLineEdit, &QLineEdit::returnPressed, this, &ServerViewWidget::handleChatReturnPressed );

    chatLayout->addWidget( mChatListWidget );
    chatLayout->addWidget( mChatLineEdit );

    outerLayout->addWidget( mAnnouncements, 0, 0, 2, 1 );
    outerLayout->addWidget( roomsGroupBox, 0, 1 );
    outerLayout->addWidget( chatGroupBox, 1, 1 );
    outerLayout->addWidget( mUsersListWidget, 0, 2, 2, 1 );

    outerLayout->setColumnStretch( 0, 2 );
    outerLayout->setColumnStretch( 1, 5 );
    outerLayout->setColumnStretch( 2, 1 );
    outerLayout->setRowStretch( 0, 2 );
    outerLayout->setRowStretch( 1, 1 );
}
 

void
ServerViewWidget::setAnnouncements( const QString& text )
{
    mAnnouncements->setText( text.isEmpty() ? EMPTY_ANNOUNCEMENTS_STR : text );
}


void
ServerViewWidget::addRoom( const std::shared_ptr<RoomConfigAdapter>& roomConfigAdapter )
{
    const unsigned int chairCount = roomConfigAdapter->getChairCount();
    const bool passwordProtected = roomConfigAdapter->isPasswordProtected();
    const unsigned int roomId = roomConfigAdapter->getRoomId();
    const QString roomName = QString::fromStdString( roomConfigAdapter->getName() );

    QStringList roomSetCodes;
    for( const auto& setCode : roomConfigAdapter->getBasicSetCodes() )
    {
        roomSetCodes.push_back( QString::fromStdString( setCode ) );
    }

    QString playersStr = QString( "0/%1" ).arg( chairCount );
    QString passwordProtectedStr = passwordProtected ? "yes" : "no";

    int treeRowIndex = mRoomTreeWidget->topLevelItemCount();  // last item
    bool itemSelected = false;

    // Delete the item if it exists, and set up to insert at same index.
    if( mRoomIdToRoomDataMap.contains( roomId ) )
    {
        mLogger->warn( "addRoom: roomId {} already exists, overwriting", roomId );
        treeRowIndex = mRoomIdToRoomDataMap[roomId].treeRowIndex;
        QTreeWidgetItem* item = mRoomTreeWidget->takeTopLevelItem( treeRowIndex );
        itemSelected = item->isSelected();
        delete item;
    }

    QTreeWidgetItem* item = new QTreeWidgetItem(
            { roomName, roomSetCodes.join( "/" ), playersStr, passwordProtectedStr } );
    mRoomTreeWidget->insertTopLevelItem( treeRowIndex, item );
    item->setSelected( itemSelected );

    mRoomTreeRowToRoomIdMap[treeRowIndex] = roomId;

    mRoomIdToRoomDataMap[roomId] = { chairCount, passwordProtected, treeRowIndex };
}


void
ServerViewWidget::updateRoomPlayerCount( int roomId, int playerCount )
{
    if( mRoomIdToRoomDataMap.contains( roomId ) )
    {
        const int chairCount = mRoomIdToRoomDataMap[roomId].chairCount;
        const int treeRowIndex = mRoomIdToRoomDataMap[roomId].treeRowIndex;
        QTreeWidgetItem* item = mRoomTreeWidget->topLevelItem( treeRowIndex );

        QString playersStr = QString( "%1/%2" ).arg( playerCount ).arg( chairCount );

        item->setText( ROOM_TREE_COLUMN_PLAYERS, playersStr );
    }
}


void
ServerViewWidget::removeRoom( int roomId )
{
    if( mRoomIdToRoomDataMap.contains( roomId ) )
    {
        int treeRowIndex = mRoomIdToRoomDataMap[roomId].treeRowIndex;
        QTreeWidgetItem* item = mRoomTreeWidget->takeTopLevelItem( treeRowIndex );
        delete item;

        mRoomIdToRoomDataMap.remove( roomId );

        // Any tree row indices after the one just removed have now shifted
        // by one.  Fix all applicable data entries and rebuild the tree row
        // map from scratch.
        mRoomTreeRowToRoomIdMap.clear();
        auto end = mRoomIdToRoomDataMap.end();
        for( auto iter = mRoomIdToRoomDataMap.begin(); iter != end; ++iter )
        {
            if( iter.value().treeRowIndex > treeRowIndex )
            {
                iter.value().treeRowIndex--;
            }
            mRoomTreeRowToRoomIdMap[treeRowIndex] = iter.key();
        }
    }
    else
    {
        mLogger->warn( "removeRoom: unknown roomId {}", roomId );
    }
}


void
ServerViewWidget::clearRooms()
{
    mRoomTreeWidget->clear();
    mRoomIdToRoomDataMap.clear();
    mRoomTreeRowToRoomIdMap.clear();
}


void
ServerViewWidget::enableJoinRoom( bool enable )
{
    mJoinRoomEnabled = enable;
    processRoomTreeSelection();
}


void
ServerViewWidget::enableCreateRoom( bool enable )
{
    mCreateRoomButton->setEnabled( enable );
}


void
ServerViewWidget::addUser( const QString& name )
{
    mUsersListWidget->addItem( name );
    mUsersListWidget->sortItems();
}


void
ServerViewWidget::removeUser( const QString& name )
{
    //qDeleteAll( mUsersListWidget->findItems( QString::fromStdString(name), Qt::MatchFixedString ) );
    QList<QListWidgetItem*> items = mUsersListWidget->findItems( name, Qt::MatchExactly );
    for( auto item : items )
    {
        mUsersListWidget->takeItem( mUsersListWidget->row( item ) );
        delete item;
    }
}


void
ServerViewWidget::clearUsers()
{
    mUsersListWidget->clear();
}


void
ServerViewWidget::addChatMessageItem( const QString& user, const QString& message )
{
    mChatListWidget->addItem( "[" + user + "] " + message );

    // Don't allow the list to grow unbounded
    if( mChatListWidget->count() > 1000 )
    {
        delete mChatListWidget->takeItem( 0 );
    }

    mChatListWidget->scrollToBottom();
}


void
ServerViewWidget::handleRoomTreeSelectionChanged()
{
    processRoomTreeSelection();
}


void
ServerViewWidget::handleRoomTreeCustomContextMenu( const QPoint& point )
{
    mLogger->debug( "context menu" );

    QModelIndex index = mRoomTreeWidget->indexAt(point);

    if( !index.isValid() )
    {
        // clicked on empty area
        mLogger->debug( "invalid index (empty area)" );
        return;
    }

    // Set up a pop-up menu.
    QMenu menu;
    QAction *joinAction = menu.addAction( "Join" );
    joinAction->setEnabled( mJoinRoomEnabled );

    // Execute the menu and act on result.
    QAction *result = menu.exec( mRoomTreeWidget->mapToGlobal( point ) );

    if( result == 0 ) return;
    else if( result == joinAction )
    {
        tryJoinRoom();
    }
}


void
ServerViewWidget::handleJoinRoomButtonClicked()
{
    mLogger->debug( "join click! roomId={}", mSelectedRoomId );
    tryJoinRoom();
}


void
ServerViewWidget::tryJoinRoom()
{
    QString password;
    if( mRoomIdToRoomDataMap.contains( mSelectedRoomId ) &&
        mRoomIdToRoomDataMap[mSelectedRoomId].passwordProtected )
    {
        bool ok;
        password = QInputDialog::getText( this, tr("Password Required"),
                                          tr("Password:"), QLineEdit::Password, "", &ok);
        if( !ok ) return;
    }

    emit joinRoomRequest( mSelectedRoomId, password );
}


void
ServerViewWidget::handleCreateRoomButtonClicked()
{
    mLogger->debug( "create click!" );
    emit createRoomRequest();
}


void
ServerViewWidget::handleChatReturnPressed()
{
    mLogger->debug( "chat return pressed!" );
    if( !mChatLineEdit->text().isEmpty() )
    {
        emit chatMessageGenerated( mChatLineEdit->text() );
        mChatLineEdit->clear();
    }
}


void
ServerViewWidget::processRoomTreeSelection()
{
    // get selected items
    // if size > 0, enable button, else disable button
    QList<QTreeWidgetItem*> selectedItems = mRoomTreeWidget->selectedItems();
    if( selectedItems.empty() )
    {
        mJoinRoomButton->setEnabled( false );
    }
    else
    {
        QTreeWidgetItem* item = selectedItems[0];
        int index = mRoomTreeWidget->indexOfTopLevelItem( item );
        if( mRoomTreeRowToRoomIdMap.contains( index ) )
        {
            mSelectedRoomId = mRoomTreeRowToRoomIdMap[index];
            mJoinRoomButton->setEnabled( mJoinRoomEnabled );
        }
        else
        {
            mLogger->warn( "selection change: no roomId for row {}", index );
            mJoinRoomButton->setEnabled( false );
        }
    }
}
