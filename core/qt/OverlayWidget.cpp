#include "OverlayWidget.h"

OverlayWidget::OverlayWidget( QWidget* parent )
  : QWidget( parent )
{
    if( parent)
    {
        resize( parent->size() );
        parent->installEventFilter( this );
        raise();
    }
}


bool
OverlayWidget::eventFilter( QObject* obj, QEvent* event )
{
    if( obj == parent() )
    {
        if( event->type() == QEvent::Resize )
        {
            // Resize this overlay to the size of the parent widget.
            QResizeEvent* resizeEvent = static_cast<QResizeEvent*>( event );
            resize( resizeEvent->size() );
        }
        else if( event->type() == QEvent::ChildAdded )
        {
            // Stay on top.
            raise();
        }
    }
    return QWidget::eventFilter( obj, event );
}


bool
OverlayWidget::event( QEvent* event )
{
    if( event->type() == QEvent::ParentAboutToChange )
    {
        if( parent() )
        {
            parent()->removeEventFilter( this );
        }
    }
    else if( event->type() == QEvent::ParentChange )
    {
        if( parent() )
        {
            parent()->installEventFilter( this );
            raise();
        }
    }
    return QWidget::event( event );
}
