#include "MtgJsonAllSetsFileCache.h"

#include <QFile>
#include "qtutils_core.h"

static const QString ALLSETS_FILENAME( "AllSets.json" );

QString
MtgJsonAllSetsFileCache::getCachedFilePath()
{
    return mCacheDir.filePath( ALLSETS_FILENAME );
}


bool
MtgJsonAllSetsFileCache::commit( const QString& filePath )
{
    const QString cacheFilePath( mCacheDir.filePath( ALLSETS_FILENAME ) );
    const QString oldCacheFilePath( mCacheDir.filePath( ALLSETS_FILENAME + ".old" ) );

    // Make sure no old file exists.
    QFile::remove( oldCacheFilePath );

    // Stash away the original file (if it exists) in case of failure.
    if( QFile::exists( cacheFilePath ) )
    {
        bool stashed = QFile::rename( cacheFilePath, oldCacheFilePath );
        if( !stashed )
        {
            mLogger->warn( "Error renaming {} to {}", cacheFilePath, oldCacheFilePath );
            return false;
        }
    }

    // Copy the file to the cache location.
    bool copied = QFile::copy( filePath, cacheFilePath );
    if( !copied )
    {
        mLogger->warn( "Error copying {} to {}", filePath, cacheFilePath );

        // Remove anything copied and restore old file.
        QFile::remove( cacheFilePath );
        QFile::rename( oldCacheFilePath, cacheFilePath );
        return false;
    }

    // Remove the old file.
    QFile::remove( oldCacheFilePath );

    return true;
}
