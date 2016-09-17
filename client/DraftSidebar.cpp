#include "DraftSidebar.h"

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSvgRenderer>
#include <QPainter>
#include <QResizeEvent>

#include "CapsuleIndicator.h"
#include "ChatEditWidget.h"

static const int CAPSULE_HEIGHT = 36;

DraftSidebar::DraftSidebar( const Logging::Config& loggingConfig,
                            QWidget*               parent )
  : QStackedWidget( parent ),
    mTimeRemainingCapsuleMode( CAPSULE_MODE_INACTIVE ),
    mTimeRemaining( -1 ),
    mTimeRemainingAlertThreshold( -1 ),
    mQueuedPacksCapsuleMode( CAPSULE_MODE_INACTIVE ),
    mQueuedPacks( -1 ),
    mUnreadChatMessages( 0 ),
    mLogger( loggingConfig.createLogger() )
{
    mExpandedTimeRemainingCapsule = new CapsuleIndicator( false, CAPSULE_HEIGHT );
    mExpandedTimeRemainingCapsule->setLabelText( "seconds" );
    mExpandedTimeRemainingCapsule->setToolTip( tr("Time remaining to select a card") );

    mExpandedQueuedPacksCapsule = new CapsuleIndicator( false, CAPSULE_HEIGHT );
    mExpandedQueuedPacksCapsule->setLabelText( "packs" );
    mExpandedQueuedPacksCapsule->setToolTip( tr("Packs awaiting selection") );

    mCompactTimeRemainingCapsule = new CapsuleIndicator( true, CAPSULE_HEIGHT );
    mCompactTimeRemainingCapsule->setToolTip( tr("Time remaining to select a card") );

    mCompactQueuedPacksCapsule = new CapsuleIndicator( true, CAPSULE_HEIGHT );
    mCompactQueuedPacksCapsule->setToolTip( tr("Packs awaiting selection") );

    setTimeRemainingCapsulesLookAndFeel( mTimeRemainingCapsuleMode );
    setQueuedPacksCapsulesLookAndFeel( mQueuedPacksCapsuleMode );

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
    expandedWidgetLayout->addWidget( mExpandedQueuedPacksCapsule );
    expandedWidgetLayout->addWidget( mExpandedTimeRemainingCapsule );
    expandedWidgetLayout->addWidget( mChatView );
    expandedWidgetLayout->addWidget( chatEdit );

    expandedWidgetLayout->setContentsMargins( 0, 0, 0, 0 );

    mCompactWidget = new QWidget();
    QVBoxLayout* compactWidgetLayout = new QVBoxLayout( mCompactWidget );
    compactWidgetLayout->setAlignment( Qt::AlignCenter );
    compactWidgetLayout->addWidget( mCompactQueuedPacksCapsule );
    compactWidgetLayout->addWidget( mCompactTimeRemainingCapsule );
    compactWidgetLayout->addStretch();
    compactWidgetLayout->addWidget( mCompactChatLabel );
    compactWidgetLayout->addSpacing( 100 );
    compactWidgetLayout->setContentsMargins( 0, 0, 0, 0 );

    addWidget( mExpandedWidget );
    addWidget( mCompactWidget );

    // Forward chat message composition signal.
    connect( chatEdit, &ChatEditWidget::messageComposed, this, &DraftSidebar::chatMessageComposed );
}


void
DraftSidebar::setDraftTimeRemaining( int time )
{
    mTimeRemaining = time;
    updateTimeRemainingCapsules();
}


void
DraftSidebar::setDraftTimeRemainingAlertThreshold( int time )
{
    mTimeRemainingAlertThreshold = time;
    updateTimeRemainingCapsules();
}


void
DraftSidebar::setDraftQueuedPacks( int packs )
{
    mQueuedPacks = packs;

    if( mQueuedPacks <= 0 )
    {
        // Only update look and feel if mode has actually changed.
        if( mQueuedPacksCapsuleMode != CAPSULE_MODE_INACTIVE )
        {
            mQueuedPacksCapsuleMode = CAPSULE_MODE_INACTIVE;
            setQueuedPacksCapsulesLookAndFeel( mQueuedPacksCapsuleMode );
        }
    }
    else
    {
        // Only update look and feel if mode has actually changed.
        if( mQueuedPacksCapsuleMode != CAPSULE_MODE_NORMAL )
        {
            mQueuedPacksCapsuleMode = CAPSULE_MODE_NORMAL;
            setQueuedPacksCapsulesLookAndFeel( mQueuedPacksCapsuleMode );
        }
    }

    mExpandedQueuedPacksCapsule->setValueText( (mQueuedPacks > 0) ? QString::number( mQueuedPacks )
                                                                  : QString() );
    mCompactQueuedPacksCapsule->setValueText( (mQueuedPacks > 0) ? QString::number( mQueuedPacks )
                                                                 : QString() );
}


void
DraftSidebar::addChatMessage( const QString& user, const QString& message )
{
    mChatView->append( "<b><font color=\"Blue\">" + user + ":</font></b> " + message );

    // If the compact widget is showing, add to the unread messages.
    mUnreadChatMessages = (currentWidget() == mCompactWidget) ? mUnreadChatMessages + 1 : 0;
    updateUnreadChatIndicator();
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
    }
    else if( (width > MAX_COMPACT_WIDTH) && (currentWidget() == mCompactWidget) )
    {
        // Switch to expanded view.
        mCompactWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        mExpandedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setCurrentWidget( mExpandedWidget );

        mUnreadChatMessages = 0;
        updateUnreadChatIndicator();
    }
}


void
DraftSidebar::updateTimeRemainingCapsules()
{
    if( mTimeRemaining < 0 )
    {
        // Only update look and feel if mode has actually changed.
        if( mTimeRemainingCapsuleMode != CAPSULE_MODE_INACTIVE )
        {
            mTimeRemainingCapsuleMode = CAPSULE_MODE_INACTIVE;
            setTimeRemainingCapsulesLookAndFeel( mTimeRemainingCapsuleMode );
        }
    }
    else if( mTimeRemaining > mTimeRemainingAlertThreshold )
    {
        // Only update look and feel if mode has actually changed.
        if( mTimeRemainingCapsuleMode != CAPSULE_MODE_NORMAL )
        {
            mTimeRemainingCapsuleMode = CAPSULE_MODE_NORMAL;
            setTimeRemainingCapsulesLookAndFeel( mTimeRemainingCapsuleMode );
        }
    }
    else if( mTimeRemaining <= mTimeRemainingAlertThreshold )
    {
        // Only update look and feel if mode has actually changed.
        if( mTimeRemainingCapsuleMode != CAPSULE_MODE_ALERTED )
        {
            mTimeRemainingCapsuleMode = CAPSULE_MODE_ALERTED;
            setTimeRemainingCapsulesLookAndFeel( mTimeRemainingCapsuleMode );
        }
    }

    mExpandedTimeRemainingCapsule->setValueText( (mTimeRemaining >= 0) ? QString::number( mTimeRemaining )
                                                                       : QString() );
    mCompactTimeRemainingCapsule->setValueText( (mTimeRemaining >= 0) ? QString::number( mTimeRemaining )
                                                                      : QString() );
}


void
DraftSidebar::setTimeRemainingCapsulesLookAndFeel( CapsuleModeType mode )
{
    CapsuleIndicator* const capsules[] = { mExpandedTimeRemainingCapsule, mCompactTimeRemainingCapsule };

    switch( mode )
    {
        case CAPSULE_MODE_INACTIVE:
            for( CapsuleIndicator* capsule : capsules )
            {
                capsule->setSvgIconPath( ":/alarm-clock-lightgray.svg" );
                capsule->setBorderColor( Qt::lightGray );
                capsule->setBorderBold( false );
                capsule->setBackgroundColor( Qt::transparent );
                capsule->setTextColor( Qt::lightGray );
            }
            break;
        case CAPSULE_MODE_NORMAL:
            for( CapsuleIndicator* capsule : capsules )
            {
                capsule->setSvgIconPath( ":/alarm-clock-darkgray.svg" );
                capsule->setBorderColor( Qt::darkGray );
                capsule->setBorderBold( true );
                capsule->setBackgroundColor( Qt::transparent );
                capsule->setTextColor( Qt::darkGray );
            }
            break;
        case CAPSULE_MODE_ALERTED:
            for( CapsuleIndicator* capsule : capsules )
            {
                capsule->setSvgIconPath( ":/alarm-clock-white.svg" );
                capsule->setBorderColor( Qt::red );
                capsule->setBorderBold( false );
                capsule->setBackgroundColor( Qt::red );
                capsule->setTextColor( Qt::white );
            }
            break;
        default:
            mLogger->warn( "unhandled lookandfeel {}", mode );
            break;
    }
}


void
DraftSidebar::setQueuedPacksCapsulesLookAndFeel( CapsuleModeType mode )
{
    CapsuleIndicator* const capsules[] = { mExpandedQueuedPacksCapsule, mCompactQueuedPacksCapsule };

    switch( mode )
    {
        case CAPSULE_MODE_INACTIVE:
            for( CapsuleIndicator* capsule : capsules )
            {
                capsule->setSvgIconPath( ":/stack-lightgray.svg" );
                capsule->setBorderColor( Qt::lightGray );
                capsule->setBorderBold( false );
                capsule->setBackgroundColor( Qt::transparent );
                capsule->setTextColor( Qt::lightGray );
            }
            break;
        case CAPSULE_MODE_NORMAL:
            for( CapsuleIndicator* capsule : capsules )
            {
                capsule->setSvgIconPath( ":/stack-darkgray.svg" );
                capsule->setBorderColor( Qt::darkGray );
                capsule->setBorderBold( true );
                capsule->setBackgroundColor( Qt::transparent );
                capsule->setTextColor( Qt::darkGray );
            }
            break;
        case CAPSULE_MODE_ALERTED:
        default:
            mLogger->warn( "unhandled lookandfeel {}", mode );
            break;
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
