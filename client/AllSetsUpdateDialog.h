#ifndef ALLSETSUPDATEDIALOG_H
#define ALLSETSUPDATEDIALOG_H

#include <QDialog>
#include "clienttypes.h"

class AllSetsUpdateDialog : public QDialog
{
    Q_OBJECT

public:
    virtual ~AllSetsUpdateDialog() {}
    virtual AllSetsDataSharedPtr getAllSetsData() const = 0;
};

#endif  // ALLSETSUPDATEDIALOG_H
