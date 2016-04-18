#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QLabel>
#include <QPixmap>

#include "clienttypes.h"
#include "Logging.h"

class ImageLoaderFactory;
class ImageLoader;

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

signals:
    void clicked();
    void shiftClicked();
    void doubleClicked();
    void menuRequested();

protected:
    QSize sizeHint() const override;
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseDoubleClickEvent( QMouseEvent* event ) override;
    virtual void enterEvent( QEvent* event ) override;
    virtual void leaveEvent( QEvent* event ) override;

private slots:
    void handleImageLoaded( int multiverseId, const QImage &image );

private:
    void updatePixmap();

    CardDataSharedPtr         mCardDataSharedPtr;
    ImageLoaderFactory* const mImageLoaderFactory;

    ImageLoader*      mImageLoader;

    QSize             mDefaultSize;
    float             mZoomFactor;

    // A copy of the original-sized pixmap obtained from ImageLoader.
    QPixmap           mPixmap;

    Logging::Config                 mLoggingConfig;
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CARDWIDGET_H
