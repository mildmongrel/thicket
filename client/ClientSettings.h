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

    QStringList getMtgJsonAllSetsBuiltinUrls() const;
    QStringList getMtgJsonAllSetsUserUrls() const;
    void        setMtgJsonAllSetsUserUrls( const QStringList& urls );
    QString     getMtgJsonAllSetsLastGoodUrl() const;
    void        setMtgJsonAllSetsLastGoodUrl( const QString& url );

    QStringList getConnectBuiltinServers() const;
    QStringList getConnectUserServers() const;
    void        setConnectUserServers( const QStringList& servers );
    bool        addConnectUserServer( const QString& server );
    QString     getConnectLastGoodServer() const;
    void        setConnectLastGoodServer( const QString& server );
    QString     getConnectLastGoodUsername() const;
    void        setConnectLastGoodUsername( const QString& username );

    QString getCardImageUrlTemplate() const;

    int     getBasicLandMultiverseId( BasicLandType basic ) const;

    QByteArray getMainWindowGeometry() const;
    void       setMainWindowGeometry( const QByteArray& byteArray );

    //
    // CommanderPane settings are done by index of pane.
    //

    QString getCommanderPaneZoom( int index ) const;
    void    setCommanderPaneZoom( int index, const QString& zoomStr );

    QString getCommanderPaneCategorization( int index ) const;
    void    setCommanderPaneCategorization( int index, const QString& catStr );

    QString getCommanderPaneSort( int index ) const;
    void    setCommanderPaneSort( int index, const QString& sortStr );

private:

    QSettings* settings;

};

#endif
