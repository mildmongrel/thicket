#ifndef CLIENTSETTINGS_H
#define CLIENTSETTINGS_H

#include <QObject>
#include "BasicLand.h"

QT_BEGIN_NAMESPACE
class QSettings;
class QDir;
QT_END_NAMESPACE

class ClientSettings : public QObject
{
    Q_OBJECT

public:

    ClientSettings( const QDir& settingsDir, QObject* parent = 0 );

    QString getConnectHost() const;
    void    setConnectHost( const QString& host );

    int     getConnectPort() const;
    void    setConnectPort( int port );

    QString getConnectName() const;
    void    setConnectName( const QString& name );

    QString getCardImageUrlTemplate() const;

    int     getBasicLandMultiverseId( BasicLandType basic ) const;

private:

    QSettings* settings;

};

#endif
