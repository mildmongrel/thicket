#include <QCommandLineParser>
#include <QtCore>
#include <QFile>

#include <stdlib.h>

#include "Server.h"
#include "ServerSettings.h"
#include "ClientNotices.h"
#include "AdminShell.h"
#include "version.h"
#include "qtutils.h"

#include "MtgJsonAllSetsData.h"
#include "CardPoolSelector.h"
#include "SimpleRandGen.h"

static std::shared_ptr<spdlog::logger> gLogger;

int main(int argc, char *argv[])
{
    qsrand( QTime(0,0,0).secsTo( QTime::currentTime() ) );

    Logging::Config loggingConfig;
    loggingConfig.setName( "server" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::info );

    gLogger = loggingConfig.createLogger();

    //
    // Create application and parse arguments.
    //

    QCoreApplication app( argc, argv );
    QCoreApplication::setApplicationName( "thicketserver" );
    QCoreApplication::setApplicationVersion( QString::fromStdString( gServerVersion ) );

    QCommandLineParser parser;
    parser.setApplicationDescription( "Creates a Thicket draft server instance.");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption portOption(
            QStringList() << "p" << "port", "Server port number.  (default: 53333)", "port", "53333" );
    parser.addOption( portOption );
    const QCommandLineOption verboseOption(
            QStringList() << "verbose", "Verbose logging output." );
    parser.addOption( verboseOption );
    const QCommandLineOption logfileOption(
            QStringList() << "logfile", "Write log output to <file>.", "file", "" );
    parser.addOption( logfileOption );
    const QCommandLineOption logRotateOption(
            QStringList() << "logrotate", "Rotate logs.  Requires '--logfile'." );
    parser.addOption( logRotateOption );
    const QCommandLineOption logRotateSizeLimitOption(
            QStringList() << "logrotate-size-limit", "Size limit for rotating log files (default 1000000).", "size-limit", "1000000" );
    parser.addOption( logRotateSizeLimitOption );
    const QCommandLineOption logRotateFileCountOption(
            QStringList() << "logrotate-file-count", "Number of files to rotate (default 2).", "file-count", "2" );
    parser.addOption( logRotateFileCountOption );

    parser.process( app );

    // Parse logging-related options and update the logger first so
    // properly-configured logging is in effect ASAP.
    {
        bool loggerUpdated = false;

        if( parser.isSet( verboseOption ) )
        {
            gLogger->debug( "command-line args: verbose={}", parser.value( verboseOption ) );
            loggingConfig.setLevel( spdlog::level::trace );
            gLogger = loggingConfig.createLogger();
        }

        std::string logfile;
        if( parser.isSet( logfileOption ) )
        {
            QString logfile = parser.value( logfileOption );
            gLogger->debug( "command-line args: logfile={}", logfile );
            if( !logfile.isEmpty() )
            {
                if( parser.isSet( logRotateOption ) )
                {
                    bool convertResult = true;

                    int index = logfile.lastIndexOf( '.' );
                    QString base = (index > 0) ? logfile.left( index ) : logfile;
                    QString ext = (index > 0) ? logfile.right( index - 1) : "";

                    gLogger->debug( "command-line args: logRotateSizeLimit={}",
                            parser.value( logRotateSizeLimitOption ) );
                    unsigned int logRotateSizeLimit =
                            parser.value( logRotateSizeLimitOption ).toUInt( &convertResult );
                    if( !convertResult )
                    {
                        gLogger->critical( "invalid argument" );
                        return 0;
                    }

                    gLogger->debug( "command-line args: logRotateFileCount={}",
                            parser.value( logRotateFileCountOption ) );
                    unsigned int logRotateFileCount =
                            parser.value( logRotateFileCountOption ).toUInt( &convertResult );
                    if( !convertResult )
                    {
                        gLogger->critical( "invalid argument" );
                        return 0;
                    }

                    loggingConfig.setRotatingFileLogging( base.toStdString(),
                            ext.toStdString(), logRotateSizeLimit, logRotateFileCount );
                }
                else
                {
                    QFile( logfile ).remove();
                    loggingConfig.setSimpleFileLogging( logfile.toStdString() );
                }
                loggerUpdated = true;
            }
        }

        // Modify the logger with logging options.
        if( loggerUpdated )
        {
            gLogger = loggingConfig.createLogger();
        }
    }

    // Parse port option.
    gLogger->debug( "command-line args: port={}", parser.value( portOption ) );
    bool convertResult = true;
    bool argsOk = true;
    unsigned int port = parser.value( portOption ).toUInt( &convertResult );
    argsOk &= convertResult;
    if( !argsOk )
    {
        gLogger->critical( "invalid argument" );
        return 0;
    }

    gLogger->info( "Server version {}", gServerVersion );
    gLogger->info( "Server protocol version {}.{}", 
            thicket::PROTOCOL_VERSION_MAJOR, thicket::PROTOCOL_VERSION_MINOR );

    //
    // Open and parse MtgJSON AllSets data.
    //

    // Open JSON set data file.
    const QDir allSetsDir = QDir::current();
    const std::string allSetsFilePath = allSetsDir.filePath( "AllSets.json" ).toStdString();
    FILE* allSetsDataFile = fopen( allSetsFilePath.c_str(), "r" );
    if( allSetsDataFile == NULL )
    {
        gLogger->critical( "failed to open {}!", allSetsFilePath );
        return 0;
    }

    //
    // Create the JSON set data instance.
    //

    // Raw non-const pointer is for initial parse, shared pointer to const
    // is used later but ensures cleanup on error.
    auto allSetsData = new MtgJsonAllSetsData();
    auto allSetsDataSharedPtr = std::shared_ptr<const AllSetsData>( allSetsData );
    bool parseResult = allSetsData->parse( allSetsDataFile );
    fclose( allSetsDataFile );
    if( !parseResult )
    {
        gLogger->critical( "Failed to parse AllSets.json!" );
        return 0;
    }

    //
    // Instantiate server settings.
    //
    std::shared_ptr<ServerSettings> serverSettings = std::make_shared<ServerSettings>( &app );

    //
    // Instantiate and initialize client notices.
    //
    std::shared_ptr<ClientNotices> clientNotices = std::make_shared<ClientNotices>( &app );
    if( !clientNotices->readAnnouncementsFromDisk() )
    {
        gLogger->warn( "Unable to read client announcements" );
    }

    //
    // Create and start Server.
    //

    Server* server = new Server( port, serverSettings, allSetsDataSharedPtr, clientNotices, loggingConfig, &app );

    // This will cause the application to exit when the server signals finished.    
    QObject::connect(server, SIGNAL(finished()), &app, SLOT(quit()));

    // This will start the server from the application event loop.
    QTimer::singleShot( 0, server, SLOT(start()) );

    //
    // Create and start the admin shell.
    //

    AdminShell* adminShell = new AdminShell( clientNotices, loggingConfig.createChildConfig( "adminshell" ), &app );

    // This will start the admin shell from the application event loop.
    QTimer::singleShot( 0, adminShell, SLOT(start()) );

    return app.exec();
}
