#include "AdminShell.h"

#include <QLocalServer>
#include <QLocalSocket>

#include "qtutils.h"
#include "ClientNotices.h"

AdminShell::AdminShell( const std::shared_ptr<ClientNotices>& clientNotices,
                        const Logging::Config&                loggingConfig,
                        QObject*                              parent )
:   QObject( parent ),
    mClientNotices( clientNotices ),
    mServer( 0 ),
    mConnection( 0 ),
    mLoggingConfig( loggingConfig ),
    mLogger( mLoggingConfig.createLogger() )
{
}


void
AdminShell::start()
{
    mServer = new QLocalServer( this );
    mLogger->debug( "starting admin server" );

    const QString SERVER_NAME = "admin";

    if( QLocalServer::removeServer( SERVER_NAME ) )
    {
        mLogger->debug( "removed existing admin server" );
    }

    if( !mServer->listen( SERVER_NAME ) )
    {
        mLogger->error( "Unable to start the admin server: {}", mServer->errorString() );
        emit finished();
        return;
    }

    connect( mServer, &QLocalServer::newConnection, this, &AdminShell::handleNewConnection );

    mLogger->notice( "The admin shell is running at: {}", mServer->fullServerName() );
}


void
AdminShell::handleNewConnection()
{
    QLocalSocket* conn = mServer->nextPendingConnection();
    if( mConnection == 0 )
    {
        mConnection = conn;
        mLogger->notice( "admin connection established" );

        connect( mConnection, &QLocalSocket::readyRead,
                 this, &AdminShell::handleSocketReadyRead );
        connect( mConnection, &QLocalSocket::disconnected,
                 this, &AdminShell::handleSocketDisconnected );
        writeToSocket( "\nThicket Admin Shell\nType 'h' or 'help' for command list.\n\n" );
        writePrompt();
    }
    else
    {
        mLogger->warn( "admin connection refused - already in use" );
        conn->abort();
    }
}


bool
AdminShell::writeToSocket( QString str )
{
    return (mConnection->write( str.toUtf8()) && mConnection->flush() );
}


void
AdminShell::handleSocketReadyRead()
{
    while( mConnection && mConnection->canReadLine() )
    {
        QString line = QString::fromUtf8( mConnection->readLine() ).trimmed();
        mLogger->info( "admin command rx: '{}'", line );

        processCommand( line );

        // Checking connection here because a quit command can remove it.
        if( mConnection )
        {
            writePrompt();
        }
    }
}


void
AdminShell::handleSocketDisconnected()
{
    mLogger->notice( "admin connection disconnected" );
    mConnection = 0;
}


bool
AdminShell::writePrompt()
{
    return writeToSocket( "admin> " );
}


void
AdminShell::processCommand( const QString& line )
{
    QString command = line.section( " ", 0, 0 );
    if( command.isEmpty() ) return;

    if( (command.compare( "h" ) == 0) || (command.compare( "help" ) == 0) )
    {
        writeToSocket( "Available commands:\n\n" );
        writeToSocket( " a,  announce         Re-read the announcements file from disk and update\n" );
        writeToSocket( "                      clients\n" );
        writeToSocket( " as, alertset [msg]   Set alert message to clients\n" );
        writeToSocket( " ac, alertclear       Clear alert to clients\n" );
        writeToSocket( " h,  help             Print this help\n" );
        writeToSocket( " q,  quit             Quit admin shell\n" );
        writeToSocket( "\n" );
    }
    else if( (command.compare( "a" ) == 0) || (command.compare( "announce" ) == 0) )
    {
        // Re-read the announcements; this will signal the server to send
        // the announcements out to clients.
        if( mClientNotices->readAnnouncementsFromDisk() )
        {
            writeToSocket( "Announcements updated\n\n" );
        }
        else
        {
            writeToSocket( "Error reading announcements - check announcements file\n\n" );
        }
    }
    else if( (command.compare( "ac" ) == 0) || (command.compare( "alertclear" ) == 0) )
    {
        // Empty string = no alert
        mClientNotices->setAlert( "" );
        writeToSocket( "Alert cleared\n\n" );
    }
    else if( (command.compare( "as" ) == 0) || (command.compare( "alertset" ) == 0) )
    {
        QString alert = line.section( " ", 1, -1 );
        writeToSocket( "Alert set: " + alert + "\n\n" );
        mClientNotices->setAlert( alert );

    }
    else if( (command.compare( "q" ) == 0) || (command.compare( "quit" ) == 0) )
    {
        mConnection->abort();
        mConnection = 0;
    }
    else
    {
        writeToSocket( "Unrecognized command '" + command + "'\n\n" );
    }
}

