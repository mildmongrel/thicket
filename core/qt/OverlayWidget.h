#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QResizeEvent>

class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayWidget( QWidget* parent = 0 );

protected:

    // Catch resize and child events from the parent widget.
    virtual bool eventFilter( QObject* obj, QEvent* event ) override;

    // Track parent widget changes.
    virtual bool event( QEvent* event ) override;
};

#endif  // OVERLAYWIDGET_H
