#include "CapsuleIndicator.h"

#include <QColor>
#include <QPainter>
#include <QResizeEvent>

static qreal BORDER_WIDTH_NORMAL = 1.5f;
static qreal BORDER_WIDTH_BOLD   = 2.0f;

CapsuleIndicator::CapsuleIndicator( bool compact, int height, QWidget *parent )
  : QWidget( parent ),
    mCompact( compact ),
    mPreferredHeight( height ),
    mBackgroundColor( Qt::transparent ),
    mBorderColor( Qt::lightGray ),
    mTextColor( Qt::lightGray ),
    mLabelTextWidthF( 0 ),
    mBorderWidthF( BORDER_WIDTH_NORMAL )
{
    updateHeightBasedFactors( mPreferredHeight );

    if( compact )
    {
        setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
    }
    else
    {
        setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    }
}

void
CapsuleIndicator::setLabelText( const QString& str )
{
    mLabelText = str;

    if( !mCompact )
    {
        QFontMetrics fm( str );
        mLabelTextWidthF = fm.boundingRect( str ).width();
        adjustSize();
    }

    update();
}


void
CapsuleIndicator::setValueText( const QString& str )
{
    const bool lengthChanged = (str.length() != mValueText.length());
    mValueText = str;

    if( lengthChanged )
    {
        // Only do the update if the string length has changed.
        updateValueTextFont();
    }

    update();
}


void
CapsuleIndicator::setSvgIconPath( const QString& svgIconPath )
{
    mSvgRenderer.load( svgIconPath );
    update();
}


void
CapsuleIndicator::setBackgroundColor( const QColor& backgroundColor )
{
    mBackgroundColor = backgroundColor;
    update();
}


void
CapsuleIndicator::setBorderColor( const QColor& borderColor )
{
    mBorderColor = borderColor;
    update();
}


void
CapsuleIndicator::setBorderBold( bool bold )
{
    mBorderWidthF = bold ? BORDER_WIDTH_BOLD : BORDER_WIDTH_NORMAL;
    update();
}


void
CapsuleIndicator::setTextColor( const QColor& textColor )
{
    mTextColor = textColor;
    update();
}


QSize
CapsuleIndicator::minimumSizeHint() const
{
    QSize sz( mMarginF + mIconSizeF.width() + mSpacingF + mValueTextWidthF + mMarginF, mPreferredHeight );
    if( !mCompact )
    {
        // Add size to display label text.
        sz.setWidth( sz.width() + mLabelTextWidthF + mSpacingF );
    }
    return sz;
}


QSize
CapsuleIndicator::sizeHint() const
{
    return minimumSizeHint();
}


void
CapsuleIndicator::resizeEvent( QResizeEvent *event )
{
    const bool heightChanged = height() != event->oldSize().height();

    if( heightChanged )
    {
        // Only update if height changed.
        updateHeightBasedFactors( height() );
        updateValueTextFont();
    }

    const qreal heightF = height();
    const qreal widthF = width();
    const QRectF rectF = rect();

    QRectF borderRectF = rect();
    // inset rectangle by half-width of border
    const qreal halfBorderWidthF = mBorderWidthF / 2.0f;
    borderRectF.adjust( halfBorderWidthF, halfBorderWidthF, -halfBorderWidthF, -halfBorderWidthF );
    mBorderPainterPath = QPainterPath();
    mBorderPainterPath.addRoundedRect( borderRectF, heightF / 2.0f, heightF / 2.0f );

    mIconRectF = QRectF( QPointF(), mIconSizeF );
    mIconRectF.moveCenter( rectF.center() ); // center on widget's rect for height
    mIconRectF.moveRight ( widthF - mMarginF - mValueTextWidthF - mSpacingF ); // move right edge to left of valueText

    mLabelTextRectF = rect();
    mLabelTextRectF.setLeft( mMarginF );
    mLabelTextRectF.setRight( mIconRectF.left() - mSpacingF );

    mValueTextRectF = rect();
    mValueTextRectF.setWidth( mValueTextWidthF );
    mValueTextRectF.moveRight( widthF - mMarginF );
}


void
CapsuleIndicator::paintEvent( QPaintEvent *event )
{
    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    QPen pen( mBorderColor );
    pen.setWidth( mBorderWidthF );
    painter.setPen( pen );
    painter.fillPath( mBorderPainterPath, mBackgroundColor );
    painter.drawPath( mBorderPainterPath );

    QPen valueTextPen( mTextColor );
    painter.setPen( valueTextPen );
    painter.setFont( mValueTextFont );
    painter.drawText( mValueTextRectF, Qt::AlignCenter, mValueText );

    if( mSvgRenderer.isValid() )
    {
        mSvgRenderer.render( &painter, mIconRectF );
    }

    // Add label text if not the compact version.
    if( !mCompact )
    {
        painter.setFont( mLabelTextFont );
        painter.drawText( mLabelTextRectF, Qt::AlignLeft | Qt::AlignVCenter, mLabelText );
    }
}


// Update internal values that contribute to the size hints of the widget
// or that can be lazy-initialized based on height only.
void
CapsuleIndicator::updateHeightBasedFactors( int height )
{
    const qreal heightF = height;

    // Margin from right and left edges.  Wider looks better on full version.
    mMarginF = mCompact ? heightF / 4.0f : // height 36 -> margin 8
                          heightF / 3.0f;  // height 36 -> margin 12

    qreal iconLengthF = (heightF * 2.0f) / 3.0f;      // height 36 -> iconLength 24
    mIconSizeF = QSizeF( iconLengthF, iconLengthF );

    // Space between items.
    mSpacingF = mCompact ? heightF * 0.05f :
                           heightF * 0.1f;

    mValueTextWidthF = heightF * 0.75f;

    mLabelTextFont = font();
    mLabelTextFont.setPointSize( height / 4 );
}


// Update font size for value text to fit within bounds.
// Depends on height; mHeightF must be set prior to calling this method.
void
CapsuleIndicator::updateValueTextFont()
{
    if( (height() <= 0) || mValueText.isEmpty() ) return;

    mValueTextFont = font();

    // Create a temporary sizing string which is the same number of characters
    // as the value string but with wide glyphs.  This way the font will
    // be adjusted the same for all values with the same number of chars.
    QString sizingStr( mValueText );
    sizingStr.fill( '#' );

    // Start with the largest font size and ratchet down until the sizing string
    // fits within the allotted width.
    int pointSize = height() / 2;           // height 36 -> font size 18
    const int minPointSize = height() / 4;  // height 36 -> font size 8
    do
    {
        mValueTextFont.setPointSize( pointSize );
        QFontMetrics fm( mValueTextFont );
        if( fm.boundingRect( sizingStr ).width() <= mValueTextWidthF ) break;
        pointSize--;
    }
    while( pointSize > minPointSize );
}
