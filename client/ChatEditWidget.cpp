#include "ChatEditWidget.h"

#include <QKeyEvent>

ChatEditWidget::ChatEditWidget( QWidget* parent )
  : QTextEdit( parent )
{
    setPlaceholderText( tr("Type chat message here...") );
}


void
ChatEditWidget::keyPressEvent ( QKeyEvent * event )
{
    if( event->key() == Qt::Key_Return )
    {
        emit messageComposed( toPlainText() );
        clear();
  
        // Swallow up the event.
        event->accept();
    }
    else
    {
        QTextEdit::keyPressEvent( event );
    }
}

