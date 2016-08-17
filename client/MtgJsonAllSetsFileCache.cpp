#include "MtgJsonAllSetsFileCache.h"

#include <QFile>
#include <QTextStream>
#include "qtutils_core.h"

static const QString ALLSETS_FILENAME( "AllSets.json" );
static const QString ALLSETS_VERSION_FILENAME( ".allsets_version" );

QString
MtgJsonAllSetsFileCache::getCachedFilePath( const AllSetsUpdateChannel::ChannelType& channel ) const
{
    const QDir channelDir = getChannelDir( channel );
    return channelDir.filePath( ALLSETS_FILENAME );
}


QString
MtgJsonAllSetsFileCache::getCachedFileVersion( const AllSetsUpdateChannel::ChannelType& channel ) const
{
    const QDir channelDir = getChannelDir( channel );
    const QString versionFilePath( channelDir.filePath( ALLSETS_VERSION_FILENAME ) );

    // Return empty string if version file doesn't exist.
    if( !QFile::exists( versionFilePath ) ) return QString();

    // Read string from version file.
    QFile file( versionFilePath );
    if( !file.open( QFile::ReadOnly | QFile::Text ) )
    {
        mLogger->warn( "error reading from version file {}", versionFilePath );
        return QString();
    }

    QTextStream in( &file );
    QString version = in.readAll();
    file.close();

    // Sanity check: the version should be a short string.
    if( version.length() > 50 )
    {
        mLogger->warn( "version file {} content too long ({})", versionFilePath, version.length() );
        return QString();
    }

    mLogger->debug( "read cached file version: {}", version );
    return version;
}


bool
MtgJsonAllSetsFileCache::commit( const AllSetsUpdateChannel::ChannelType& channel, const QString& filePath, const QString& version )
{
    const QDir channelDir = getChannelDir( channel );
    const QString allSetsFilePath( channelDir.filePath( ALLSETS_FILENAME ) );
    const QString oldAllSetsFilePath( channelDir.filePath( ALLSETS_FILENAME + ".old" ) );
    const QString versionFilePath( channelDir.filePath( ALLSETS_VERSION_FILENAME ) );
    const QString oldVersionFilePath( channelDir.filePath( ALLSETS_VERSION_FILENAME + ".old" ) );
    bool copyOk;

    // Make sure channel folder exists; create if it doesn't
    if( !channelDir.exists() )
    {
        mLogger->info( "creating set data channel cache directory: {}", channelDir.path().toStdString() );
        if( !channelDir.mkpath( "." ) )
        {
            mLogger->warn( "error creating set data channel cache directory!" );
            return false;
        }
    }

    // Make sure no old files exist.
    QFile::remove( oldAllSetsFilePath );
    QFile::remove( oldVersionFilePath );

    // Stash away the original file (if it exists) in case of failure.
    if( QFile::exists( allSetsFilePath ) )
    {
        bool stashed = QFile::rename( allSetsFilePath, oldAllSetsFilePath );
        if( !stashed )
        {
            mLogger->warn( "error renaming {} to {}", allSetsFilePath, oldAllSetsFilePath );
            return false;
        }
    }

    // Stash away the original version file (if it exists) in case of failure.
    if( QFile::exists( versionFilePath ) )
    {
        bool renameOk = QFile::rename( versionFilePath, oldVersionFilePath );
        if( !renameOk )
        {
            // Recover and fail.
            mLogger->warn( "error renaming {} to {}", versionFilePath, oldVersionFilePath );
            goto recover_allsets_file;
        }
    }

    // Copy the file to the cache location.
    copyOk = QFile::copy( filePath, allSetsFilePath );
    if( !copyOk )
    {
        mLogger->warn( "error copying {} to {}", filePath, allSetsFilePath );

        // Remove anything copied, recover, and fail.
        QFile::remove( allSetsFilePath );
        goto recover_version_and_allsets_file;
    }

    // Write a new version file.  Enclosed in a scope to allow goto's to cross.
    {
        QFile file( versionFilePath );
        if( !file.open( QIODevice::ReadWrite ) )
        {
            mLogger->warn( "error writing to version file {}", versionFilePath );
            goto recover_version_and_allsets_file;
        }
        QTextStream stream( &file );
        stream << version;
        file.flush();
        file.close();
    }

    // Remove the old files.
    QFile::remove( oldAllSetsFilePath );
    QFile::remove( oldVersionFilePath );

    return true;

recover_version_and_allsets_file:
    QFile::rename( oldVersionFilePath, versionFilePath );
recover_allsets_file:
    QFile::rename( oldAllSetsFilePath, allSetsFilePath );

    return false;
}


QDir
MtgJsonAllSetsFileCache::getChannelDir( const AllSetsUpdateChannel::ChannelType& channel ) const
{
    const QString channelName = AllSetsUpdateChannel::channelToString( channel );
    QString channelPath = mCacheDir.path() + '/' + channelName;
    return QDir( channelPath );
}
