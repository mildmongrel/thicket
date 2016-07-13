#include "RoomViewWidget.h"

#include <QLabel>
#include <QGridLayout>
#include <QTreeWidget>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QLineEdit>

#include "RoomConfigAdapter.h"

static const int PLAYER_TREE_COLUMN_NAME            = 1;
static const int PLAYER_TREE_COLUMN_STATE           = 2;
static const int PLAYER_TREE_COLUMN_COCKATRICE_HASH = 3;


RoomViewWidget::RoomViewWidget( const Logging::Config& loggingConfig,
                                QWidget*               parent )
  : QWidget( parent ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    QGridLayout *outerLayout = new QGridLayout();
    setLayout( outerLayout );

    mRoomTitleLabel = new QLabel();
    mRoomTitleLabel->setAlignment( Qt::AlignCenter );
    // Put some space above the title
    mRoomTitleLabel->setContentsMargins( 0, 10, 0, 0 );

    mRoomDescLabel = new QLabel();
    mRoomDescLabel->setAlignment( Qt::AlignCenter );
    // Put some space below the description
    mRoomDescLabel->setContentsMargins( 0, 0, 0, 15 );

    QGroupBox* playerStatusGroupBox = new QGroupBox( "Player Status" );
    QVBoxLayout* playerStatusLayout = new QVBoxLayout();
    playerStatusGroupBox->setLayout( playerStatusLayout );

    mPlayersTreeWidget = new QTreeWidget();
    mPlayersTreeWidget->setRootIsDecorated( false );
    mPlayersTreeWidget->setColumnCount( 4 );
    mPlayersTreeWidget->setHeaderLabels( { "#", "Name", "State", "Cockatrice Hash" } );
    // Not selectable, no focus rectangles
    mPlayersTreeWidget->setSelectionMode( QAbstractItemView::NoSelection );
    mPlayersTreeWidget->setFocusPolicy( Qt::NoFocus );

    QHBoxLayout* playerStatusButtonLayout = new QHBoxLayout();

    mReadyButton = new QPushButton( "&Ready" );
    mReadyButton->setCheckable( true );
    connect( mReadyButton, &QPushButton::toggled, this, &RoomViewWidget::handleReadyButtonToggled );

    mLeaveButton = new QPushButton( "&Leave Room" );
    connect( mLeaveButton, &QPushButton::clicked, this, &RoomViewWidget::handleLeaveButtonClicked );

    playerStatusButtonLayout->addStretch();
    playerStatusButtonLayout->addWidget( mReadyButton );
    playerStatusButtonLayout->addSpacing( 30 );
    playerStatusButtonLayout->addWidget( mLeaveButton );
    playerStatusButtonLayout->addStretch();

    playerStatusLayout->addWidget( mPlayersTreeWidget );
    playerStatusLayout->addLayout( playerStatusButtonLayout );

    QGroupBox* chatGroupBox = new QGroupBox( "Room Chat" );
    QVBoxLayout* chatLayout = new QVBoxLayout();
    chatGroupBox->setLayout( chatLayout );

    mChatListWidget = new QListWidget();
    mChatListWidget->setSelectionMode( QAbstractItemView::NoSelection );
    mChatListWidget->setFocusPolicy( Qt::NoFocus );
    mChatLineEdit = new QLineEdit();
    connect( mChatLineEdit, &QLineEdit::returnPressed, this, &RoomViewWidget::handleChatReturnPressed );

    chatLayout->addWidget( mChatListWidget );
    chatLayout->addWidget( mChatLineEdit );

    outerLayout->addWidget( mRoomTitleLabel,      0, 0 );
    outerLayout->addWidget( mRoomDescLabel,       1, 0 );
    outerLayout->addWidget( playerStatusGroupBox, 2, 0 );
    outerLayout->addWidget( chatGroupBox,         3, 0 );
}
 

void
RoomViewWidget::setRoomConfig( const std::shared_ptr<RoomConfigAdapter>& roomConfig )
{
    QString title = QString::fromStdString( roomConfig->getName() );
    mRoomTitleLabel->setText( "<b>" + title + "</b>" );

    QStringList roomSetCodes;
    for( const auto& setCode : roomConfig->getSetCodes() )
    {
        roomSetCodes.push_back( QString::fromStdString( setCode ) );
    }

    QString desc;
    if( roomConfig->isBoosterDraft() )
    {
        desc = QString( "Booster Draft\n%1" ).arg( roomSetCodes.join( "/" ) );
    }
    else if( roomConfig->isSealedDraft() )
    {
        desc = QString( "Sealed (%1)" ).arg( roomSetCodes.join( "/" ) );
    }
    mRoomDescLabel->setText( desc );
}


void
RoomViewWidget::setChairCount( int chairCount )
{
    mPlayersTreeWidget->clear();
    for( int i = 0; i < chairCount; ++i )
    {
        QTreeWidgetItem* item = new QTreeWidgetItem( { QString::number(i), "", "" } );
        mPlayersTreeWidget->addTopLevelItem( item );
    }
}


void
RoomViewWidget::setPlayerInfo( int chairIndex, const QString& name, bool isBot, const QString& state )
{
    QTreeWidgetItem* item = mPlayersTreeWidget->topLevelItem( chairIndex );
    item->setText( PLAYER_TREE_COLUMN_NAME, name );
    item->setText( PLAYER_TREE_COLUMN_STATE, state );
}


void
RoomViewWidget::setPlayerCockatriceHash( int chairIndex, const QString& hash )
{
    QTreeWidgetItem* item = mPlayersTreeWidget->topLevelItem( chairIndex );
    item->setText( PLAYER_TREE_COLUMN_COCKATRICE_HASH, hash );
}


void
RoomViewWidget::clearPlayers()
{
    mPlayersTreeWidget->clear();
}


void
RoomViewWidget::reset()
{
    mRoomTitleLabel->clear();
    mPlayersTreeWidget->clear();
    mChatListWidget->clear();
    mChatLineEdit->clear();
    mReadyButton->setChecked( false );
}


void
RoomViewWidget::addChatMessage( const QString& user, const QString& message )
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
RoomViewWidget::handleReadyButtonToggled( bool ready )
{
    mLogger->debug( "ready toggle! ready={}", ready );
    emit readyUpdate( ready );
}


void
RoomViewWidget::handleLeaveButtonClicked()
{
    mLogger->debug( "leave clicked!" );
    emit leave();
}


void
RoomViewWidget::handleChatReturnPressed()
{
    mLogger->debug( "chat return pressed!" );
    if( !mChatLineEdit->text().isEmpty() )
    {
        emit chatMessageGenerated( mChatLineEdit->text() );
        mChatLineEdit->clear();
    }
}
