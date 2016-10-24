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
DraftSidebar::setRoomConfig( const std::shared_ptr<RoomConfigAdapter>& roomConfig )
{
    const QString title = QString::fromStdString( roomConfig->getName() );

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

    mChatView->append( "Joined Room:" );
    mChatView->setAlignment( Qt::AlignLeft );
    mChatView->append( QString() );
    mChatView->append( "<b>" + title + "</b>" );
    mChatView->setAlignment( Qt::AlignCenter );
    mChatView->append( desc );
    mChatView->setAlignment( Qt::AlignCenter );
    mChatView->append( QString() );
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
