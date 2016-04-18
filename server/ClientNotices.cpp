#include "ClientNotices.h"

#include <QFile>
#include <QTextStream>

const QString ANNOUNCEMENTS_FILE = "thicketserver-announce.txt";

ClientNotices::ClientNotices( QObject *parent )
  : QObject( parent ) {}


bool
ClientNotices::readAnnouncementsFromDisk()
{
    QFile file( ANNOUNCEMENTS_FILE );
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        return false;
    }
    QTextStream in( &file );
    mAnnouncements = in.readAll();

    emit announcementsUpdate( mAnnouncements );
    return true;
}


void
ClientNotices::setAlert( const QString& alert )
{
    mAlert = alert;
    emit alertUpdate( mAlert );
}
