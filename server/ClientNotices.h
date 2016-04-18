#ifndef CLIENTNOTICES_H
#define CLIENTNOTICES_H

#include <QObject>

class ClientNotices : public QObject
{
    Q_OBJECT

public:

    ClientNotices( QObject *parent = 0 );

    //
    // Announcements to clients: An announcements string to be sent to clients.
    //

    // Read the announcements to client from disk.
    bool    readAnnouncementsFromDisk();
    QString getAnnouncements() const { return mAnnouncements; }

    //
    // Alert to clients: An alert string to be sent to clients.
    //

    void    setAlert( const QString& alert );
    QString getAlert() const { return mAlert; }

signals:

    void announcementsUpdate( const QString& text );
    void alertUpdate( const QString& text );

private:

    QString mAnnouncements;
    QString mAlert;

};

#endif
