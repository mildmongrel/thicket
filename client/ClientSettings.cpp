#include "ClientSettings.h"

#include <QDir>
#include <QSettings>
#include <QTemporaryFile>

#include "qtutils_core.h"

static const int CURRENT_VERSION = 1;

ClientSettings::ClientSettings( const QDir&            settingsDir,
                                const Logging::Config& loggingConfig,
                                QObject*               parent )
  : QObject( parent ),
    mLogger( loggingConfig.createLogger() )
{
    settings = new QSettings( settingsDir.filePath( "clientsettings.ini" ), QSettings::IniFormat, this );

    // Get version.  Returns 0 if no version value found in settings.
    int version = settings->value( "version" ).toInt();

    if( version != CURRENT_VERSION )
    {
        if( version < 0 )
        {
            mLogger->warn( "Invalid settings version ({}), clearing!", version );
            settings->clear();
        }
        else if( version == 0 )
        {
            mLogger->info( "Creating new settings" );
            settings->clear();
        }
        else if( version > CURRENT_VERSION )
        {
            QTemporaryFile tmpFile;
            mLogger->warn( "Settings file is newer than application - using temporary file {}", tmpFile.fileName() );
            settings->deleteLater();
            settings = new QSettings( tmpFile.fileName(), QSettings::IniFormat, this );
        }
        else
        {
            mLogger->info( "Updating settings version {} to version {}", version, CURRENT_VERSION );

            // EXAMPLE FOR FUTURE SETTINGS UPDATES:
            // if( version < 2 )
            // {
            //   < convert any v1 settings to v2 >
            // }
            // if( version < 3 )
            // {
            //   < convert any v2 settings to v3 >
            // }
        }

        // Once we've gotten this far our settings version is current.
        settings->setValue( "version", CURRENT_VERSION );
    }
}


void
ClientSettings::reset()
{
    mLogger->notice( "Settings reset!" );
    settings->clear();
    settings->setValue( "version", CURRENT_VERSION );
}


QString
ClientSettings::getWebServiceBaseUrl() const
{
    return !mWebServiceBaseUrlOverride.isEmpty() ?  mWebServiceBaseUrlOverride :
            settings->value( "webservice/baseurl", "http://thicketdraft.net:53332" ).toString();
}


AllSetsUpdateChannel::ChannelType
ClientSettings::getAllSetsUpdateChannel() const
{
    QString channelName = settings->value( "update/allsetsupdatechannel" ).toString();
    AllSetsUpdateChannel::ChannelType channel = AllSetsUpdateChannel::stringToChannel( channelName );
    return (channel != AllSetsUpdateChannel::CHANNEL_UNKNOWN) ? channel : AllSetsUpdateChannel::getDefaultChannel();
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


BasicLandMuidMap
ClientSettings::getBasicLandMultiverseIds() const
{
    BasicLandMuidMap muidMap;
    muidMap.setMuid( BASIC_LAND_PLAINS,   settings->value( "basiclandmuid/plains",   "401985" ).toInt() );
    muidMap.setMuid( BASIC_LAND_ISLAND,   settings->value( "basiclandmuid/island",   "401918" ).toInt() );
    muidMap.setMuid( BASIC_LAND_SWAMP,    settings->value( "basiclandmuid/swamp",    "402053" ).toInt() );
    muidMap.setMuid( BASIC_LAND_MOUNTAIN, settings->value( "basiclandmuid/mountain", "401953" ).toInt() );
    muidMap.setMuid( BASIC_LAND_FOREST,   settings->value( "basiclandmuid/forest",   "401882" ).toInt() );
    return muidMap;
}


void
ClientSettings::setBasicLandMultiverseIds( const BasicLandMuidMap& muidMap )
{
    settings->setValue( "basiclandmuid/plains",   muidMap.getMuid( BASIC_LAND_PLAINS   ) );
    settings->setValue( "basiclandmuid/island",   muidMap.getMuid( BASIC_LAND_ISLAND   ) );
    settings->setValue( "basiclandmuid/swamp",    muidMap.getMuid( BASIC_LAND_SWAMP    ) );
    settings->setValue( "basiclandmuid/mountain", muidMap.getMuid( BASIC_LAND_MOUNTAIN ) );
    settings->setValue( "basiclandmuid/forest",   muidMap.getMuid( BASIC_LAND_FOREST   ) );
    emit basicLandMultiverseIdsChanged( muidMap );
}

quint64
ClientSettings::getImageCacheMaxSize() const
{
    const unsigned int defaultSize = 50 * 1024 * 1024; // 50 MB
    return settings->value( "imagecache/maxsize", QString::number( defaultSize ) ).toULongLong();
}


void
ClientSettings::setImageCacheMaxSize( quint64 size )
{
    settings->setValue( "imagecache/maxsize", size );
    emit imageCacheMaxSizeChanged( size );
}


bool
ClientSettings::getBeepOnNewPack() const
{
    return settings->value( "sound/beeponnewpack", false ).toBool();
}


void
ClientSettings::setBeepOnNewPack( bool beep )
{
    settings->setValue( "sound/beeponnewpack", beep );
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


QByteArray
ClientSettings::getDraftTabSplitterState() const
{
    return settings->value( "mainwindow/drafttabsplitterstate" ).toByteArray();
}


void
ClientSettings::setDraftTabSplitterState( const QByteArray& byteArray )
{
    settings->setValue( "mainwindow/drafttabsplitterstate", byteArray );
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

