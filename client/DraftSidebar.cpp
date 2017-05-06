#include "DraftSidebar.h"

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSvgRenderer>
#include <QPainter>
#include <QResizeEvent>

#include "ChatEditWidget.h"
#include "RoomConfigAdapter.h"

static const int CAPSULE_HEIGHT = 36;

DraftSidebar::DraftSidebar( const Logging::Config& loggingConfig,
                            QWidget*               parent )
  : QStackedWidget( parent ),
    mUnreadChatMessages( 0 ),
    mLogger( loggingConfig.createLogger() )
{
    mChatView = new QTextEdit();
    mChatView->setReadOnly( true );

    // This keeps the messages from growing unbounded; it removes old
    // messages when limit is reached.
    mChatView->document()->setMaximumBlockCount( 1000 );

    ChatEditWidget* chatEdit = new ChatEditWidget();
    chatEdit->setFixedHeight( 50 );

    mCompactChatLabel = new QLabel();
    mCompactChatLabel->setAlignment( Qt::AlignCenter );
    updateUnreadChatIndicator();

    mExpandedWidget = new QWidget();
    QVBoxLayout* expandedWidgetLayout = new QVBoxLayout( mExpandedWidget );
    expandedWidgetLayout->addWidget( mChatView );
    expandedWidgetLayout->addWidget( chatEdit );

    expandedWidgetLayout->setContentsMargins( 0, 0, 0, 0 );

    mCompactWidget = new QWidget();
    QVBoxLayout* compactWidgetLayout = new QVBoxLayout( mCompactWidget );
    compactWidgetLayout->setAlignment( Qt::AlignCenter );
    compactWidgetLayout->addStretch();
    compactWidgetLayout->addSpacing( 100 );
    compactWidgetLayout->addWidget( mCompactChatLabel );
    compactWidgetLayout->addStretch();
    compactWidgetLayout->setContentsMargins( 0, 0, 0, 0 );

    addWidget( mExpandedWidget );
    addWidget( mCompactWidget );

    // Forward chat message composition signal.
    connect( chatEdit, &ChatEditWidget::messageComposed, this, &DraftSidebar::chatMessageComposed );
}


void
DraftSidebar::addRoomJoinMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig, bool rejoin )
{
    const QString pre = rejoin ? tr("Rejoined room") : tr("Joined room");
    const QString title = QString::fromStdString( roomConfig->getName() );

    QStringList roomSetCodes;
    for( const auto& setCode : roomConfig->getSetCodes() )
    {
        roomSetCodes.push_back( QString::fromStdString( setCode ) );
    }

    QString desc;
    if( roomConfig->isBoosterDraft() )
    {
        desc = tr("Booster Draft");
    }
    else if( roomConfig->isSealedDraft() )
    {
        desc = tr("Sealed Deck");
    }
    else if( roomConfig->isGridDraft() )
    {
        desc = tr("Grid Draft");
    }

    mChatView->append( QString("<i>%1 </i><b>%2</b><i>:</i>")
            .arg( pre )
            .arg( title ) );
    mChatView->append( QString("<i>&nbsp;&nbsp;&nbsp;%1 (%2)</i>")
            .arg( desc )
            .arg( roomSetCodes.join( "/" ) ) );
}


void
DraftSidebar::addRoomLeaveMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig )
{
    const QString title = QString::fromStdString( roomConfig->getName() );
    mChatView->append( QString("<i>%1 </i><b>%2</b>")
            .arg( tr("Left Room") )
            .arg( title ) );
    mChatView->append( QString() );
}


void
DraftSidebar::addCardSelectMessage( const QString& name, bool autoSelected )
{
    const QString selectedStr = autoSelected ? tr("*Auto-selected*") : tr("Selected");
    mChatView->append( QString("<i>%1</i> <b>%2</b>")
            .arg( selectedStr )
            .arg( name ) );
}


void
DraftSidebar::addChatMessage( const QString& user, const QString& message )
{
    mChatView->append( "<b><font color=\"Blue\">" + user + ":</font></b> " + message );
    mChatView->setAlignment( Qt::AlignLeft );

    // If the compact widget is showing, add to the unread messages.
    mUnreadChatMessages = (currentWidget() == mCompactWidget) ? mUnreadChatMessages + 1 : 0;
    updateUnreadChatIndicator();
}


void
DraftSidebar::addGameMessage( const QString& message, MessageLevel level )
{
    const QString formatOpen  = (level == MESSAGE_LEVEL_LOW) ? "<i><font color=\"Gray\">" : (level == MESSAGE_LEVEL_HIGH) ? "<i><b>"   : "<i>";
    const QString formatClose = (level == MESSAGE_LEVEL_LOW) ? "</i></font>"              : (level == MESSAGE_LEVEL_HIGH) ? "</i></b>" : "</i>";
    mChatView->append( QString("%1%2%3")
            .arg( formatOpen )
            .arg( message )
            .arg( formatClose ) );
    mChatView->setAlignment( Qt::AlignLeft );
}


bool
DraftSidebar::isCompacted()
{
    return (currentWidget() == mCompactWidget);
}


QSize
DraftSidebar::minimumSizeHint() const
{
    // This widget can be as small as the compact widget because it will
    // automatically switch to that widget when resized smaller.
    return mCompactWidget->minimumSizeHint();
}


QSize
DraftSidebar::sizeHint() const
{
    // This widget prefers to be the size of its expanded widget.
    return mExpandedWidget->sizeHint();
}


void
DraftSidebar::resizeEvent( QResizeEvent *event )
{
    const int MAX_COMPACT_WIDTH = 120;
    const int width = event->size().width();

    if( (width <= MAX_COMPACT_WIDTH) && (currentWidget() == mExpandedWidget) )
    {
        // Switch to compact view.
        mExpandedWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        mCompactWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setCurrentWidget( mCompactWidget );

        emit compacted();
    }
    else if( (width > MAX_COMPACT_WIDTH) && (currentWidget() == mCompactWidget) )
    {
        // Switch to expanded view.
        mCompactWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        mExpandedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setCurrentWidget( mExpandedWidget );

        mUnreadChatMessages = 0;
        updateUnreadChatIndicator();

        emit expanded();
    }
}


void
DraftSidebar::updateUnreadChatIndicator()
{
    mCompactChatLabel->setText( tr("%1 Unread<br>Message%2")
            .arg( mUnreadChatMessages )
            .arg( (mUnreadChatMessages != 1) ? "s" : "" ) );
    mCompactChatLabel->setStyleSheet( (mUnreadChatMessages > 0) ? "color: red; font: bold" : QString() );
}
