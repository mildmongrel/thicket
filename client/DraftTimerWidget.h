#ifndef DRAFTTIMERWIDGET_H
#define DRAFTTIMERWIDGET_H

#include <QLabel>

class DraftTimerWidget : public QLabel
{
    Q_OBJECT

public:

    enum SizeType
    {
        SIZE_NORMAL,
        SIZE_LARGE
    };

    explicit DraftTimerWidget( SizeType size = SIZE_NORMAL, int alertThreshold = -1, QWidget* parent = 0 );

public slots:

    // Set count; -1 makes the counter empty.
    void setCount( int count );

    // Set threshold where appearance becomes alerting; -1 to disable.
    void setAlertThreshold( int threshold ) { mAlertThreshold = threshold; }

private:

    int mAlertThreshold;
    bool mAlerted;

};

#endif  // DRAFTTIMERWIDGET_H
