#include "ClientSettings.h"

#include <QDir>
#include <QSettings>


ClientSettings::ClientSettings( const QDir& settingsDir, QObject* parent )
  : QObject( parent )
{
    settings = new QSettings( settingsDir.filePath( "clientsettings.ini" ), QSettings::IniFormat, this );
}


QString
ClientSettings::getConnectHost() const
{
    return settings->value( "connect/host", "thicketdraft.net" ).toString();
}


void
ClientSettings::setConnectHost( const QString& host )
{
    settings->setValue( "connect/host", host );
}


int
ClientSettings::getConnectPort() const
{
    return settings->value( "connect/port", "53333" ).toInt();
}


void
ClientSettings::setConnectPort( int port )
{
    settings->setValue( "connect/port", port );
}


QString
ClientSettings::getConnectName() const
{
    return settings->value( "connect/name", "player" ).toString();
}


void
ClientSettings::setConnectName( const QString& name )
{
    settings->setValue( "connect/name", name );
}


QString
ClientSettings::getMtgJsonAllSetsUrl() const
{
    const QString defaultUrl( "http://mtgjson.com/json/AllSets.json" );
    return settings->value( "external/mtgjsonallsetsurl", defaultUrl ).toString();
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

