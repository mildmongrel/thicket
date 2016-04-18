#ifndef SIZEDSVGWIDGET_H
#define SIZEDSVGWIDGET_H

#include <QSvgWidget>

class SizedSvgWidget : public QSvgWidget
{
    Q_OBJECT

public:

    SizedSvgWidget( const QSize& size, Qt::AspectRatioMode mode = Qt::IgnoreAspectRatio, QWidget* parent = 0 );

    void setSize( const QSize& size );
    void setScaling( const QSize& size, Qt::AspectRatioMode mode = Qt::IgnoreAspectRatio );

    void load( const QString & file );
    void load( const QByteArray & contents );

    virtual QSize sizeHint() const override { return getAdjustedSize(); }
    virtual QSize minimumSizeHint() const override { return getAdjustedSize(); }

private:

    QSize getAdjustedSize() const;

    QSize mScalingSize;
    Qt::AspectRatioMode mAspectRatioMode;
};

#endif  // SIZEDSVGWIDGET_H

