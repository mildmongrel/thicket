#ifndef MTGJSONALLSETSFILECACHE_H
#define MTGJSONALLSETSFILECACHE_H

#include <QFile>
#include <QDir>
#include "Logging.h"

class MtgJsonAllSetsFileCache
{

public:

    MtgJsonAllSetsFileCache( const QDir&     cacheDir,
                             Logging::Config loggingConfig = Logging::Config() )
      : mCacheDir( cacheDir ),
        mLogger( loggingConfig.createLogger() )
    {}

    QString getCachedFilePath()
    {
        return mCacheDir.filePath( "AllSets.json" );
    }

    // This will:
    //   - check that the parameter file exists
    //   - move the current AllSets.json file in the cache (if it exists) to AllSets.json.bak
    //   - move the parameter file to AllSets.json
    bool moveToCache( QString& filePath );

private:

    QDir                            mCacheDir;
    std::shared_ptr<spdlog::logger> mLogger;

};

#endif
