#ifndef BASICLANDCONTROLWIDGET_H
#define BASICLANDCONTROLWIDGET_H

#include <map>
#include <QWidget>

#include "BasicLand.h"
#include "BasicLandQuantities.h"

QT_BEGIN_NAMESPACE
class QSpinBox;
QT_END_NAMESPACE

class BasicLandControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BasicLandControlWidget( QWidget *parent = 0 );

    // Mimic behavior of a user pressing the down-arrow for a land type.
    void decrementBasicLandQuantity( const BasicLandType& basic );

public slots:
    void setBasicLandQuantities( const BasicLandQuantities& basicLandQtys );

signals:
    void basicLandQuantitiesUpdate( const BasicLandQuantities& basicLandQtys );

private:
    std::map<BasicLandType,QSpinBox*> mBasicLandSpinBoxMap;
    BasicLandQuantities mBasicLandQtys;
};

#endif  // BASICLANDCONTROLWIDGET_H
