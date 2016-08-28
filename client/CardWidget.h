#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QLabel>
#include <QPixmap>

#include "clienttypes.h"
#include "OverlayWidget.h"
#include "Logging.h"

QT_BEGIN_NAMESPACE
class QResizeEvent;
QT_END_NAMESPACE

class ImageLoaderFactory;
class ImageLoader;

class CardWidget_Overlay;

class CardWidget : public QLabel
{
    Q_OBJECT
public:
    explicit CardWidget( const CardDataSharedPtr& cardDataSharedPtr,
                         ImageLoaderFactory*      imageLoaderFactory,
                         const QSize&             defaultSize,
                         const Logging::Config&   loggingConfig = Logging::Config(),
                         QWidget*                 parent = 0 );

    CardDataSharedPtr getCardData() const { return mCardDataSharedPtr; }

    void setDefaultSize( const QSize& size );
    void setZoomFactor( float zoomFactor );
    void loadImage();

    void setPreselectable( bool preselectable ) { mPreselectable = preselectable; }
    void setPreselected( bool enabled );

signals:
    void preselectRequested();
    void selectRequested();
    void moveRequested();

protected:
    QSize sizeHint() const override;
    virtual void mousePressEvent( QMouseEvent* event ) override;
    virtual void mouseDoubleClickEvent( QMouseEvent* event ) override;
    virtual void enterEvent( QEvent* event ) override;
    virtual void leaveEvent( QEvent* event ) override;

private slots:
    void handleImageLoaded( int multiverseId, const QImage &image );

private:
    void updatePixmap();
    void initOverlay();

    CardDataSharedPtr         mCardDataSharedPtr;
    ImageLoaderFactory* const mImageLoaderFactory;

    ImageLoader*      mImageLoader;

    QSize             mDefaultSize;
    float             mZoomFactor;

    // A copy of the original-sized pixmap obtained from ImageLoader.
    QPixmap           mPixmap;

    bool mPreselectable;
    bool mPreselected;

    bool                mMouseWithin;
    CardWidget_Overlay* mOverlay;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};


// This would be a nested class within CardWidget but Qt doesn't allow
// nested QObject classes.
class CardWidget_Overlay : public OverlayWidget
{
    Q_OBJECT
public:
    CardWidget_Overlay( QWidget * parent = 0 );

    void setPreselected( bool enabled );

signals:
    void preselectRequested();

protected:
    virtual bool event( QEvent* event ) override;
    virtual void mouseMoveEvent( QMouseEvent* event ) override;
    virtual void mousePressEvent( QMouseEvent* event ) override;
    virtual void resizeEvent( QResizeEvent* resizeEvent ) override;
    virtual void paintEvent( QPaintEvent* ) override;

private:
    bool   mPreselected;
    QPoint mMousePos;

    QRect  mPreselectRect;

    // Cached painting calculations.
    QRectF          mPreselectRectF;
    QRadialGradient mBackgroundRadialGradient;
    QRectF          mIconRectF;
    QRectF          mPinTopRectF;
    QRectF          mPinHandleRectF;
    QPainterPath    mPinBasePath;
    QPainterPath    mPinPath;
};

#endif  // CARDWIDGET_H
