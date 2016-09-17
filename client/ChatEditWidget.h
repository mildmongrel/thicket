#ifndef CHATEDITWIDGET_H
#define CHATEDITWIDGET_H

#include <QTextEdit>

class ChatEditWidget : public QTextEdit
{
    Q_OBJECT
public:
    ChatEditWidget( QWidget* parent = nullptr );
signals:
    void messageComposed( const QString& str );
protected:
    virtual void keyPressEvent ( QKeyEvent* event ) override;
};

#endif  // CHATEDITWIDGET_H

