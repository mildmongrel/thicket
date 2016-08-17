#ifndef ALLSETSUPDATER_H
#define ALLSETSUPDATER_H

#include <QObject>
#include "clienttypes.h"

class AllSetsUpdater : public QObject
{
    Q_OBJECT

public:
    AllSetsUpdater( QObject* parent = 0 ) : QObject( parent ) {}
    virtual ~AllSetsUpdater() {}

    virtual void start( bool background = false ) = 0;

signals:
    void allSetsUpdated( const AllSetsDataSharedPtr& allSetsDataSptr );
    void finished();
};

#endif  // ALLSETSUPDATER_H
