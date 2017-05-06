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

    void setPreselectable( bool enabled );
    bool isPreselectable() const { return mPreselectable; }
    void setPreselected( bool enabled );
    bool isPreselected() const { return mPreselected; }
    void setDimmed( bool enabled );
    bool isDimmed() const { return mDimmed; }
    void setHighlighted( bool enabled );
    bool isHighlighted() const { return mHighlighted; }
    void setSelectedByOpponent( bool enabled );
    bool isSelectedByOpponent() const { return mSelectedByOpponent; }
    void setSelectedByPlayer( bool enabled );
    bool isSelectedByPlayer() const { return mSelectedByPlayer; }

    // Resets all above traits to false.
    void resetTraits();

signals:
    void preselectRequested();
    void selectRequested();
    void moveRequested();

protected:
    virtual void mousePressEvent( QMouseEvent* event ) override;
    virtual void mouseDoubleClickEvent( QMouseEvent* event ) override;
    virtual void mouseMoveEvent( QMouseEvent* event ) override;
    virtual void enterEvent( QEvent* event ) override;
    virtual void leaveEvent( QEvent* event ) override;
    virtual bool event( QEvent* event ) override;

private slots:
    void handleImageLoaded( int multiverseId, const QImage &image );

private:

    void updateScaling();
    void updateOverlay();

    CardDataSharedPtr         mCardDataSharedPtr;
    ImageLoaderFactory* const mImageLoaderFactory;

    ImageLoader*      mImageLoader;

    QSize             mDefaultSize;
    float             mZoomFactor;

    // A copy of the original-sized pixmap obtained from ImageLoader.
    QPixmap           mPixmap;

    // The string for the default tooltip.
    QString           mToolTipStr;

    bool mPreselectable;
    bool mPreselected;
    bool mDimmed;
    bool mHighlighted;
    bool mSelectedByOpponent;
    bool mSelectedByPlayer;

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
    CardWidget_Overlay( CardWidget* parent );

    bool inPreselectRegion( const QPoint& pos ) const;

protected:
    virtual void resizeEvent( QResizeEvent* resizeEvent ) override;
    virtual void paintEvent( QPaintEvent* ) override;

private:

    // Pointer to parent CardWidget where properties can be queried.
    const CardWidget * const mParentCardWidget;

    QRect  mPreselectRect;

    // Cached painting calculations.
    QRectF          mPreselectRectF;
    QRadialGradient mBackgroundRadialGradient;
    QRectF          mIconRectF;
    QRectF          mPinTopRectF;
    QRectF          mPinHandleRectF;
    QPainterPath    mPinBasePath;
    QPainterPath    mPinPath;
    QRectF          mBannerRectF;
};

#endif  // CARDWIDGET_H
