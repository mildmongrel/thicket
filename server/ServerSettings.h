#ifndef SERVERSETTINGS_H
#define SERVERSETTINGS_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

class ServerSettings : public QObject
{
    Q_OBJECT

public:

    ServerSettings( QObject* parent = 0 );

    QString getServerName();

private:

    QSettings* mSettings;

};

#endif
