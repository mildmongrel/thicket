#include "DraftSidebar.h"

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSvgRenderer>
#include <QPainter>
#include <QResizeEvent>
#include <QToolTip>

#include "CardData.h"
#include "ChatEditWidget.h"
#include "RoomConfigAdapter.h"
#include "ImageLoaderFactory.h"
#include "CardImageLoader.h"
#include "qtutils_widget.h"

static const int CAPSULE_HEIGHT = 36;

DraftSidebar::DraftSidebar( ImageLoaderFactory*    imageLoaderFactory,
                            const Logging::Config& loggingConfig,
                            QWidget*               parent )
  : QStackedWidget( parent ),
    mUnreadChatMessages( 0 ),
    mLogger( loggingConfig.createLogger() )
{
    mChatBox = new ChatTextBrowser( imageLoaderFactory, loggingConfig.createChildConfig( "chatview" ) );

    ChatEditWidget* chatEdit = new ChatEditWidget();
    chatEdit->setFixedHeight( 50 );

    mCompactChatLabel = new QLabel();
    mCompactChatLabel->setAlignment( Qt::AlignCenter );
    updateUnreadChatIndicator();

    mExpandedWidget = new QWidget();
    QVBoxLayout* expandedWidgetLayout = new QVBoxLayout( mExpandedWidget );
    expandedWidgetLayout->addWidget( mChatBox );
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
    mChatBox->addRoomJoinMessage( roomConfig, rejoin );
}


void
DraftSidebar::addRoomLeaveMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig )
{
    mChatBox->addRoomLeaveMessage( roomConfig );
}


void
DraftSidebar::addCardSelectMessage( const CardDataSharedPtr& cardDataSharedPtr, bool autoSelected )
{
    mChatBox->addCardSelectMessage( cardDataSharedPtr, autoSelected );
}


void
DraftSidebar::addChatMessage( const QString& user, const QString& message )
{
    mChatBox->addChatMessage( user, message );

    // If the compact widget is showing, add to the unread messages.
    mUnreadChatMessages = (currentWidget() == mCompactWidget) ? mUnreadChatMessages + 1 : 0;
    updateUnreadChatIndicator();
}


void
DraftSidebar::addGameMessage( const QString& message, MessageLevel level )
{
    const QString formatOpen  = (level == MESSAGE_LEVEL_LOW) ? "<i><font color=\"Gray\">" : (level == MESSAGE_LEVEL_HIGH) ? "<i><b>"   : "<i>";
    const QString formatClose = (level == MESSAGE_LEVEL_LOW) ? "</i></font>"              : (level == MESSAGE_LEVEL_HIGH) ? "</i></b>" : "</i>";
    mChatBox->append( QString("%1%2%3")
            .arg( formatOpen )
            .arg( message )
            .arg( formatClose ) );
    mChatBox->setAlignment( Qt::AlignLeft );
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





ChatTextBrowser::ChatTextBrowser( ImageLoaderFactory*    imageLoaderFactory,
                            const Logging::Config& loggingConfig,
                            QWidget*               parent )
  : QTextBrowser( parent ),
    mLogger( loggingConfig.createLogger() )
{
    setOpenExternalLinks( true );
    setMouseTracking(true);

    // This keeps the messages from growing unbounded; it removes old
    // messages when limit is reached.
    document()->setMaximumBlockCount( 1000 );

    // Monitor document changes so that card name cursor ranges can be adjusted
    // or deleted if/when content is deleted.
    connect( document(), &QTextDocument::contentsChange, this, &ChatTextBrowser::handleDocumentContentChange );

    // Create the image loader and connect it to our handler.
    mCardImageLoader = imageLoaderFactory->createImageLoader(
            loggingConfig.createChildConfig( "imageloader" ), this );
    connect( mCardImageLoader, &CardImageLoader::imageLoaded, this, &ChatTextBrowser::handleImageLoaded );
}


void
ChatTextBrowser::addRoomJoinMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig, bool rejoin )
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

    append( QString("<i>%1 </i><b>%2</b><i>:</i>")
            .arg( pre )
            .arg( title ) );
    append( QString("<i>&nbsp;&nbsp;&nbsp;%1 (%2)</i>")
            .arg( desc )
            .arg( roomSetCodes.join( "/" ) ) );
}


void
ChatTextBrowser::addRoomLeaveMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig )
{
    const QString title = QString::fromStdString( roomConfig->getName() );
    append( QString("<i>%1 </i><b>%2</b>")
            .arg( tr("Left Room") )
            .arg( title ) );
    append( QString() );
}


void
ChatTextBrowser::addCardSelectMessage( const CardDataSharedPtr& cardData, bool autoSelected )
{
    const QString selectedStr = autoSelected ? tr("*Auto-selected*") : tr("Selected");
    const QString name = QString::fromStdString( cardData->getName() );
    append( QString("<i>%1</i> <b>%2</b>")
            .arg( selectedStr )
            .arg( name ) );

    // Store card name position for tooltip referencing
    CardNameCursorRange cardNamePos;
    cardNamePos.cardData = cardData;
    cardNamePos.endPos = textCursor().position();
    cardNamePos.beginPos = cardNamePos.endPos - name.length();
    mCardNameCursorRanges.append( cardNamePos );
}


void
ChatTextBrowser::addChatMessage( const QString& user, const QString& message )
{
    QRegExp urlRegExp( "((?:https?|ftp)://\\S+)" );
    QString messageWithLinks( message );
    messageWithLinks.replace( urlRegExp, "<a href=\"\\1\">\\1</a>" );
    append( "<b><font color=\"Green\">[" + user + "]</font></b> " + messageWithLinks );
}


void
ChatTextBrowser::addGameMessage( const QString& message, DraftSidebar::MessageLevel level )
{
    const QString formatOpen  = (level == DraftSidebar::MESSAGE_LEVEL_LOW) ? "<i><font color=\"Gray\">" : (level == DraftSidebar::MESSAGE_LEVEL_HIGH) ? "<i><b>"   : "<i>";
    const QString formatClose = (level == DraftSidebar::MESSAGE_LEVEL_LOW) ? "</i></font>"              : (level == DraftSidebar::MESSAGE_LEVEL_HIGH) ? "</i></b>" : "</i>";
    append( QString("%1%2%3")
            .arg( formatOpen )
            .arg( message )
            .arg( formatClose ) );
}


bool
ChatTextBrowser::event( QEvent *event )
{
    // Look for card names in our list to bring up a tooltip.
    if( event->type() == QEvent::ToolTip )
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>( event );
        QTextCursor cursor = cursorForPosition( helpEvent->pos() );

        // Search in reverse order since items of interest are more likely at the end.
        CardDataSharedPtr cardData;
        for( auto iter = mCardNameCursorRanges.crbegin(); iter != mCardNameCursorRanges.crend(); ++iter )
        {
            if( (cursor.position() > iter->beginPos) && (cursor.position() < iter->endPos) )
            {
                cardData = iter->cardData;
            }
            // If our range is before the cursor we can stop searching.
            if( iter->endPos < cursor.position() ) break;
        }

        // If there's a card under the cursor, load the card image for the tooltip.
        if( cardData )
        {
            mToolTipPos = helpEvent->globalPos();
            mToolTipCardData = cardData;
            const int toolTipMuid = cardData->getMultiverseId();
            if( toolTipMuid > 0 )
            {
                mCardImageLoader->loadImage( toolTipMuid );
            }
            else
            {
                mLogger->debug( "skipping image load of -1 muid" );
            }
        }
        else
        {
            QToolTip::hideText();
        }
        return true;
    }
    return QTextEdit::event(event);
}


void
ChatTextBrowser::handleDocumentContentChange( int pos, int charsRemoved, int charsAdded )
{
        auto iter = mCardNameCursorRanges.begin();
        while( iter != mCardNameCursorRanges.end() )
        {
            // If the position is before our range, range must be offset.
            if( pos < iter->beginPos )
            {
                iter->beginPos -= charsRemoved;
                iter->beginPos += charsAdded;
                iter->endPos -= charsRemoved;
                iter->endPos += charsAdded;
            }
            if( iter->beginPos < 0 )
            {
                iter = mCardNameCursorRanges.erase( iter );
            }
            else
            {
                ++iter;
            }
        }
}


void
ChatTextBrowser::handleImageLoaded( int multiverseId, const QImage &image )
{
    mLogger->debug( "image {} loaded", multiverseId );

    const int toolTipMuid = mToolTipCardData->getMultiverseId();
    if( toolTipMuid == multiverseId )
    {
        QString toolTipStr;
        QPixmap pixmap = QPixmap::fromImage( image );
        if( !pixmap.isNull() )
        {
            // If this is a split card, always show a rotated tooltip.  Otherwise
            // show a normal tooltip if the view is zoomed out.
            if( mToolTipCardData->isSplit() )
            {
                QTransform transform;
                transform.rotate( 90 );
                QPixmap rotatedPixmap = pixmap.transformed( transform );
                toolTipStr = qtutils::getPixmapAsHtmlText( rotatedPixmap );
            }
            else
            {
                toolTipStr = qtutils::getPixmapAsHtmlText( pixmap );
            }
        }
        QToolTip::showText( mToolTipPos, toolTipStr );
    }
    else
    {
        mLogger->error( "ignoring image load for muid {}, waiting for {}", multiverseId, toolTipMuid );
    }
}
