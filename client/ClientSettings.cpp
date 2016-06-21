#include "ClientSettings.h"

#include <QDir>
#include <QSettings>


ClientSettings::ClientSettings( const QDir& settingsDir, QObject* parent )
  : QObject( parent )
{
    settings = new QSettings( settingsDir.filePath( "clientsettings.ini" ), QSettings::IniFormat, this );
}


QStringList
ClientSettings::getMtgJsonAllSetsBuiltinUrls() const
{
    QStringList strList;
    strList << "http://mtgjson.com/json/AllSets.json";
    return strList;
}


QStringList
ClientSettings::getMtgJsonAllSetsUserUrls() const
{
    return settings->value( "mtgjson_allsets_update/userurls" ).toStringList();
}


void
ClientSettings::setMtgJsonAllSetsUserUrls( const QStringList& urls )
{
    settings->setValue( "mtgjson_allsets_update/userurls", urls );
}


QString
ClientSettings::getMtgJsonAllSetsLastGoodUrl() const
{
    return settings->value( "mtgjson_allsets_update/lastgoodurl" ).toString();
}


void
ClientSettings::setMtgJsonAllSetsLastGoodUrl( const QString& url )
{
    settings->setValue( "mtgjson_allsets_update/lastgoodurl", url );
}


QStringList
ClientSettings::getConnectBuiltinServers() const
{
    QStringList strList;
    strList << "thicketdraft.net";
    return strList;
}


QStringList 
ClientSettings::getConnectUserServers() const
{
    return settings->value( "connect/userservers" ).toStringList();
}


void        
ClientSettings::setConnectUserServers( const QStringList& servers )
{
    settings->setValue( "connect/userservers", servers );
}


bool
ClientSettings::addConnectUserServer( const QString& server )
{
    bool result = false;
    // if not in builtin and not in user, add
    if( !getConnectBuiltinServers().contains( server, Qt::CaseInsensitive ) )
    {
        QStringList userServers = getConnectUserServers();
        if( !userServers.contains( server, Qt::CaseInsensitive ) )
        {
            userServers << server;
            setConnectUserServers( userServers );
            result = true;
        }
    }

    return result;
}


QString     
ClientSettings::getConnectLastGoodServer() const
{
    return settings->value( "connect/lastgoodserver" ).toString();
}


void        
ClientSettings::setConnectLastGoodServer( const QString& server )
{
    settings->setValue( "connect/lastgoodserver", server );
}


QString     
ClientSettings::getConnectLastGoodUsername() const
{
    return settings->value( "connect/lastgoodusername" ).toString();
}


void        
ClientSettings::setConnectLastGoodUsername( const QString& username )
{
    settings->setValue( "connect/lastgoodusername", username );
}


QString
ClientSettings::getCardImageUrlTemplate() const
{
    // example: http://gatherer.wizards.com/Handlers/Image.ashx?multiverseid=2479&type=card
    const QString defaultUrlTemplate(
            "http://gatherer.wizards.com/Handlers/Image.ashx?multiverseid=%muid%&type=card" );

    return settings->value( "external/cardimageurl", defaultUrlTemplate ).toString();
}


int
ClientSettings::getBasicLandMultiverseId( BasicLandType basic ) const
{
    switch( basic )
    {
        case BASIC_LAND_PLAINS:
            return settings->value( "basiclandmuid/plains", "401985" ).toInt();
        case BASIC_LAND_ISLAND:
            return settings->value( "basiclandmuid/island", "401918" ).toInt();
        case BASIC_LAND_SWAMP:
            return settings->value( "basiclandmuid/swamp", "402053" ).toInt();
        case BASIC_LAND_MOUNTAIN:
            return settings->value( "basiclandmuid/mountain", "401953" ).toInt();
        case BASIC_LAND_FOREST:
            return settings->value( "basiclandmuid/forest", "401882" ).toInt();
    }
    return -1;
}


unsigned int
ClientSettings::getImageCacheMaxSize() const
{
    const unsigned int defaultSize = 50 * 1024 * 1024; // 50 MB
    return settings->value( "imagecache/maxsize", QString::number( defaultSize ) ).toUInt();
}


QByteArray
ClientSettings::getMainWindowGeometry() const
{
    return settings->value( "mainwindow/geometry" ).toByteArray();
}


void
ClientSettings::setMainWindowGeometry( const QByteArray& byteArray )
{
    settings->setValue( "mainwindow/geometry", byteArray );
}


QString
ClientSettings::getCommanderPaneZoom( int index ) const
{
    return settings->value( QString("commanderpane%1/zoom").arg(QString::number(index)) ).toString();
}


void
ClientSettings::setCommanderPaneZoom( int index, const QString& zoomStr )
{
    settings->setValue( QString("commanderpane%1/zoom").arg(QString::number(index)), zoomStr );
}


QString
ClientSettings::getCommanderPaneCategorization( int index ) const
{
    return settings->value( QString("commanderpane%1/categorization").arg(QString::number(index)) ).toString();
}


void
ClientSettings::setCommanderPaneCategorization( int index, const QString& catStr )
{
    settings->setValue( QString("commanderpane%1/categorization").arg(QString::number(index)), catStr );
}


QString
ClientSettings::getCommanderPaneSort( int index ) const
{
    return settings->value( QString("commanderpane%1/sort").arg(QString::number(index)) ).toString();
}


void
ClientSettings::setCommanderPaneSort( int index, const QString& sortStr )
{
    settings->setValue( QString("commanderpane%1/sort").arg(QString::number(index)), sortStr );
}

