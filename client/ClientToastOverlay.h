#ifndef CLIENTTOASTOVERLAY_H
#define CLIENTTOASTOVERLAY_H

#include "OverlayWidget.h"

QT_BEGIN_NAMESPACE
class QTimer;
class QTextDocument;
QT_END_NAMESPACE

class ClientToastOverlay : public OverlayWidget
{
    Q_OBJECT
public:
    ClientToastOverlay( QWidget* parent );
    void setTimeToLive( int ttl ) { mTimeToLive = ttl; }
    void setBottomRightOffset( QPoint bottomRightOffset ) { mBottomRightOffset = bottomRightOffset; }
    void addToast( const QString& text );
    void clearToasts();

protected:
    virtual void mousePressEvent( QMouseEvent* event ) override;
    virtual void resizeEvent( QResizeEvent* resizeEvent ) override;
    virtual void paintEvent( QPaintEvent* ) override;

private slots:
    void handleTimerExpired();

private:

    void updateToastRects();
    static void formatTextDocument( QTextDocument& doc, const QString& text, int opacity );

    struct ToastData
    {
        QString        text;
        int            age;
        int            opacity;
        QTextDocument* doc;
        QRect          rect;
    };

    QPoint             mBottomRightOffset;
    int                mTimeToLive;
    QTimer*            mTimer;
    QVector<ToastData> mToasts;
};

#endif
