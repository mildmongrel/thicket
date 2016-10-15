#ifndef READYSPLASH_H
#define READYSPLASH_H

#include <QWidget>

class ReadySplash : public QWidget
{
    Q_OBJECT

public:

    ReadySplash( QWidget* parent = 0 );

signals:
    void ready( bool ready );

private:
    bool mBlinkState;
};

#endif  // READYSPLASH_H

