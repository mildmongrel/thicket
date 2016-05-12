#include <QApplication>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>

#include "version.h"
#include "client.h"
#include "ClientSettings.h"
#include "ImageCache.h"
#include "MtgJsonAllSetsData.h"
#include "MtgJsonAllSetsFileCache.h"
#include "MtgJsonAllSetsUpdateDialog.h"
#include "qtutils_core.h"


int main(int argc, char *argv[])
{
    QString logFilename = "thicketclient-" + QString::number( QApplication::applicationPid() ) + ".log";
    Logging::Config loggingConfig;
    loggingConfig.setName( "client" );
    loggingConfig.setStdoutLogging( true );
    loggingConfig.setLevel( spdlog::level::info );

    std::shared_ptr<spdlog::logger> logger = loggingConfig.createLogger();

    //
    // Create Qt application.
    //

    QApplication app(argc, argv);
    QApplication::setApplicationName( "thicketclient" );
    QApplication::setApplicationVersion( QString::fromStdString( gClientVersion ) );

    //
    // Parse command-line arguments.
    //

    QCommandLineParser parser;
    parser.setApplicationDescription( "Thicket draft client.");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption verboseOption(
            QStringList() << "verbose", "Verbose logging output." );
    parser.addOption( verboseOption );
    const QCommandLineOption logfileOption(
            QStringList() << "logfile", "Write log output to <file>.", "file", "" );
    parser.addOption( logfileOption );

    parser.process( app );

    // Parse logging-related options and update the logger first so
    // properly-configured logging is in effect ASAP.
    {
        bool updated = false;

        if( parser.isSet( verboseOption ) )
        {
            logger->debug( "command-line args: verbose={}", parser.value( verboseOption ) );
            loggingConfig.setLevel( spdlog::level::trace );
            updated = true;
        }

        std::string logfile;
        if( parser.isSet( logfileOption ) )
        {
            QString logfile = parser.value( logfileOption );
            logger->debug( "command-line args: logfile={}", logfile );
            if( !logfile.isEmpty() )
            {
                QFile( logfile ).remove();
                loggingConfig.setSimpleFileLogging( logfile.toStdString() );
                updated = true;
            }
        }

        // Modify the logger with logging options.
        if( updated )
        {
            logger = loggingConfig.createLogger();
        }
    }

    logger->info( "Client version {}", gClientVersion );
    logger->info( "Client protocol version {}.{}", 
            thicket::PROTOCOL_VERSION_MAJOR, thicket::PROTOCOL_VERSION_MINOR );

    //
    // Set up application directories.
    //

    QDir settingsDir( QStandardPaths::writableLocation( QStandardPaths::DataLocation ) );
    logger->debug( "settingsDir: {}", settingsDir.path().toStdString() );
    if( !settingsDir.exists() )
    {
        logger->info( "creating settings directory: {}", settingsDir.path().toStdString() );
        if( !settingsDir.mkpath( "." ) )
        {
            // On error use current working dir
            logger->warn( "error creating settings directory!" );
            settingsDir = QDir();
        }
        else
        {
            logger->debug( "created settings directory" );
        }
    }

    QDir cacheDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) );
    logger->debug( "cacheDir: {}", cacheDir.path().toStdString() );
    if( !cacheDir.exists() )
    {
        logger->info( "creating cache directory: {}", cacheDir.path().toStdString() );
        if( !cacheDir.mkpath( "." ) )
        {
            // On error use temp dir
            logger->warn( "error creating cache directory!" );
            cacheDir = QStandardPaths::writableLocation( QStandardPaths::TempLocation );
        }
        else
        {
            logger->debug( "created cache directory" );
        }
    }

    QString imageCachePath = cacheDir.path() + "/imagecache";
    QDir imageCacheDir( imageCachePath );
    if( !imageCacheDir.exists() )
    {
        logger->info( "creating image cache directory: {}", imageCacheDir.path().toStdString() );
        if( !imageCacheDir.mkpath( "." ) )
        {
            // On error use temp dir
            logger->warn( "error creating image cache directory!" );
            imageCacheDir = QStandardPaths::writableLocation( QStandardPaths::TempLocation );
        }
        else
        {
            logger->debug( "created image cache directory" );
        }
    }

    QString mtgJsonCachePath = cacheDir.path() + "/mtgjson";
    QDir mtgJsonCacheDir( mtgJsonCachePath );
    if( !mtgJsonCacheDir.exists() )
    {
        logger->info( "creating mtgjson cache directory: {}", mtgJsonCacheDir.path().toStdString() );
        if( !mtgJsonCacheDir.mkpath( "." ) )
        {
            // On error use temp dir
            logger->warn( "error creating mtgjson cache directory!" );
            mtgJsonCacheDir = QStandardPaths::writableLocation( QStandardPaths::TempLocation );
        }
        else
        {
            logger->debug( "created mtgjson cache directory" );
        }
    }

    //
    // Initialize set data.  This may fail, but the client can handle
    // having no set data so log and proceed.
    //

    MtgJsonAllSetsFileCache allSetsFileCache( mtgJsonCacheDir, loggingConfig.createChildConfig( "allsetsfilecache" ) );

    AllSetsDataSharedPtr allSetsDataSptr;

    const std::string allSetsFilePath = allSetsFileCache.getCachedFilePath().toStdString();
    FILE* allSetsDataFile = fopen( allSetsFilePath.c_str(), "r" );
    if( allSetsDataFile != NULL )
    {
        // Create the JSON set data instance.
        MtgJsonAllSetsData* mtgJsonAllSetsDataPtr = new MtgJsonAllSetsData( loggingConfig.createChildConfig( "mtgjson" ) );
        bool parseResult = mtgJsonAllSetsDataPtr->parse( allSetsDataFile );
        fclose( allSetsDataFile );
        if( parseResult )
        {
            allSetsDataSptr.reset( mtgJsonAllSetsDataPtr );
        }
        else
        {
            logger->warn( "failed to parse cached AllSets file at {}", allSetsFilePath );
            delete mtgJsonAllSetsDataPtr;
        }
    }
    else
    {
        // This may be normal, e.g. first session.
        logger->notice( "failed to open cached AllSets file at {}", allSetsFilePath );
    }

    //
    // Create other client helper objects.
    //

    ClientSettings settings( settingsDir );
    ImageCache imageCache( imageCacheDir, loggingConfig.createChildConfig( "imagecache" ) );
    MtgJsonAllSetsUpdateDialog* allSetsUpdateDialog = new MtgJsonAllSetsUpdateDialog(
            settings.getMtgJsonAllSetsUrl(),
            &allSetsFileCache,
            loggingConfig.createChildConfig( "allsetsupdate" ) );

    //
    // Create client main window and start.
    //

    Client client( &settings, allSetsDataSptr, allSetsUpdateDialog, &imageCache, loggingConfig );

    // Move the client window to the center of the screen.  Needs to be done after
    // show() because it looks like Qt doesn't calculate the geometry until show()
    // is called.
    client.show();
    client.move( QApplication::desktop()->screen()->rect().center() - client.rect().center() );

    return app.exec();
}
