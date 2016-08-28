#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QResizeEvent>

class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayWidget(QWidget * parent = 0) : QWidget(parent) {
        if (parent) {
            resize(parent->size());
            parent->installEventFilter(this);
            raise();
        }
    }
protected:
    //! Catches resize and child events from the parent widget
    bool eventFilter(QObject * obj, QEvent * ev) {
 //       std::cout << "eventFilter\n";
        if (obj == parent()) {
            if (ev->type() == QEvent::Resize) {
                QResizeEvent * rev = static_cast<QResizeEvent*>(ev);
                resize(rev->size());
            }
            else if (ev->type() == QEvent::ChildAdded) {
                raise();
            }
        }
        return QWidget::eventFilter(obj, ev);
    }
    //! Tracks parent widget changes
    bool event(QEvent* ev) {
//        std::cout << "event\n";
        if (ev->type() == QEvent::ParentAboutToChange) {
            if (parent()) parent()->removeEventFilter(this);
        }
        else if (ev->type() == QEvent::ParentChange) {
            if (parent()) {
                parent()->installEventFilter(this);
                raise();
            }
        }
        return QWidget::event(ev);
    }
};

#endif  // OVERLAYWIDGET_H
