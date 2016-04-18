#include "ServerSettings.h"

#include <QSettings>

const QString KEY_SERVER_NAME = "servername";


static void
setValueIfEmpty( QSettings* settings, const QString & key, const QVariant & value ) 
{
    if( !settings->contains( key ) )
    {
        settings->setValue( key, value );
    }
}


ServerSettings::ServerSettings( QObject* parent )
  : QObject( parent )
{
    mSettings = new QSettings( "thicketserver.ini", QSettings::IniFormat, this );

    setValueIfEmpty( mSettings, KEY_SERVER_NAME, "Thicket Server" );
}


QString
ServerSettings::getServerName()
{
    return mSettings->value( KEY_SERVER_NAME ).toString();
}
