#ifndef DRAFTSIDEBAR_H
#define DRAFTSIDEBAR_H

#include <QStackedWidget>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

#include "clienttypes.h"
#include "Logging.h"

class DraftTimerWidget;
class RoomConfigAdapter;
class ChatTextEdit;
class ImageLoaderFactory;
class ImageLoader;

class DraftSidebar : public QStackedWidget
{
    Q_OBJECT

public:

    enum MessageLevel
    {
        MESSAGE_LEVEL_LOW,
        MESSAGE_LEVEL_NORMAL,
        MESSAGE_LEVEL_HIGH
    };

    DraftSidebar( ImageLoaderFactory*    imageLoaderFactory,
                  const Logging::Config& loggingConfig = Logging::Config(),
                  QWidget*               parent = 0 );

    void addRoomJoinMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig, bool rejoin );
    void addRoomLeaveMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig );
    void addCardSelectMessage( const CardDataSharedPtr& cardData, bool autoSelected );
    void addChatMessage( const QString& user, const QString& message );
    void addGameMessage( const QString& message, MessageLevel level = MESSAGE_LEVEL_NORMAL );

    bool isCompacted();

    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;

signals:
    void chatMessageComposed( const QString& str );
    void compacted();
    void expanded();

protected:
    virtual void resizeEvent( QResizeEvent *event ) override;

private:

    void updateUnreadChatIndicator();

    QWidget* mExpandedWidget;
    QWidget* mCompactWidget;

    ChatTextEdit* mChatView;

    int     mUnreadChatMessages;
    QLabel* mCompactChatLabel;

    std::shared_ptr<spdlog::logger> mLogger;
};


class ChatTextEdit : public QTextEdit
{
    Q_OBJECT

public:

    ChatTextEdit( ImageLoaderFactory*    imageLoaderFactory,
                  const Logging::Config& loggingConfig = Logging::Config(),
                  QWidget*               parent = 0 );

    void addRoomJoinMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig, bool rejoin );
    void addRoomLeaveMessage( const std::shared_ptr<RoomConfigAdapter>& roomConfig );
    void addCardSelectMessage( const CardDataSharedPtr& cardData, bool autoSelected );
    void addChatMessage( const QString& user, const QString& message );
    void addGameMessage( const QString& message, DraftSidebar::MessageLevel level = DraftSidebar::MESSAGE_LEVEL_NORMAL );

    virtual bool event( QEvent *event ) override;

private:

    struct CardNameCursorRange
    {
        int               beginPos;
        int               endPos;
        CardDataSharedPtr cardData;
    };

    void handleDocumentContentChange( int pos, int charsRemoved, int charsAdded );
    void handleImageLoaded( int multiverseId, const QImage &image );

    QList<CardNameCursorRange> mCardNameCursorRanges;

    ImageLoader*      mImageLoader;
    CardDataSharedPtr mToolTipCardData;
    QPoint            mToolTipPos;

    std::shared_ptr<spdlog::logger> mLogger;

};

#endif  // DRAFTSIDEBAR_H

