#ifndef CAPSULEINDICATOR_H
#define CAPSULEINDICATOR_H

#include <QSvgRenderer>
#include <QWidget>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class CapsuleIndicator : public QWidget
{
    Q_OBJECT
public:

    enum StyleType
    {
        STYLE_NORMAL,
        STYLE_COMPACT,
        STYLE_MICRO
    };

    CapsuleIndicator( StyleType style = STYLE_NORMAL, int height = 36, QWidget* parent = 0 );
    void setLabelText( const QString& str );
    void setValueText( const QString& str );

    void setSvgIconPath( const QString& svgIconPath );
    void setBackgroundColor( const QColor& backgroundColor );
    void setBorderColor( const QColor& borderColor );
    void setBorderBold( bool bold );
    void setTextColor( const QColor& textColor );

    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;

protected:
    virtual void resizeEvent( QResizeEvent *event );
    virtual void paintEvent( QPaintEvent *event );

private:

    void updateHeightBasedFactors( int height );
    void updateSizeBasedFactors( int width, int height );
    void updateValueTextFont();

    StyleType mStyle;
    int       mPreferredHeight;

    QString mLabelText;
    QString mValueText;
    QColor  mBackgroundColor;
    QColor  mBorderColor;
    bool    mBorderBold;
    QColor  mTextColor;

    QSvgRenderer mSvgRenderer;

    qreal   mMarginF;
    qreal   mSpacingF;
    QSizeF  mIconSizeF;
    qreal   mValueTextWidthF;
    qreal   mLabelTextWidthF;

    QFont   mLabelTextFont;
    QFont   mValueTextFont;

    QRectF  mIconRectF;
    QRectF  mLabelTextRectF;
    QRectF  mValueTextRectF;

    qreal        mBorderWidthF;
    QPainterPath mBorderPainterPath;
};

#endif  // CAPSULEINDICATOR_H

