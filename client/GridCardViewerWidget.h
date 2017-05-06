#ifndef GRIDCARDVIEWERWIDGET_H
#define GRIDCARDVIEWERWIDGET_H

#include "CardViewerWidget.h"

QT_BEGIN_NAMESPACE
class QGridLayout;
QT_END_NAMESPACE

class GridToolButton;

// Grid Card Viewer widget - hold a layout of card widgets and manages sorting,
// zooming, forwarding card signals, etc.
class GridCardViewerWidget : public CardViewerWidget
{
    Q_OBJECT

public:
    explicit GridCardViewerWidget( ImageLoaderFactory*    imageLoaderFactory,
                                   const Logging::Config& loggingConfig = Logging::Config(),
                                   QWidget*               parent = 0 );
    virtual ~GridCardViewerWidget();

    virtual void setCards( const QList<CardDataSharedPtr>& cards ) override;

    virtual void setWaitingForTurn( bool waiting ) override;

private:

    virtual void selectedCardsUpdateHandler() override;

    bool isIndexSelectable( int i );

    QSet<int> getRowSelectableIndices( int row );
    QSet<int> getColSelectableIndices( int col );

    void updateCardWidgetSelectedAppearance( CardWidget* cardWidget );
    void updateButtonVisibility();

    QGridLayout* mGridCardsLayout;
    GridToolButton* mTopColButtons[3];
    GridToolButton* mBottomColButtons[3];
    GridToolButton* mLeftRowButtons[3];
    GridToolButton* mRightRowButtons[3];

    bool mWaitingForTurn;
};



#include <QToolButton>
class GridToolButton : public QToolButton
{
    Q_OBJECT
public:
    GridToolButton( QWidget* parent = 0 );

signals:
    void mouseEntered();
    void mouseExited();

protected:
    virtual void enterEvent( QEvent* e ) override;
    virtual void leaveEvent( QEvent* e ) override;
};

#endif  // CARDVIEWERWIDGET_H
