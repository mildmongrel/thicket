#ifndef MTGJSONALLSETSFILECACHE_H
#define MTGJSONALLSETSFILECACHE_H

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

    QString getCachedFilePath();

    // Commit file to the application's storage area.
    bool commit( const QString& filePath );

private:

    QDir                            mCacheDir;
    std::shared_ptr<spdlog::logger> mLogger;

};

#endif
