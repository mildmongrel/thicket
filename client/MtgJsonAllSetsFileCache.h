#ifndef MTGJSONALLSETSFILECACHE_H
#define MTGJSONALLSETSFILECACHE_H

#include <QDir>
#include "Logging.h"
#include "AllSetsUpdateChannel.h"

class MtgJsonAllSetsFileCache
{

public:

    MtgJsonAllSetsFileCache( const QDir&     cacheDir,
                             Logging::Config loggingConfig = Logging::Config() )
      : mCacheDir( cacheDir ),
        mLogger( loggingConfig.createLogger() )
    {}

    QString getCachedFilePath( const AllSetsUpdateChannel::ChannelType& channel ) const;
    QString getCachedFileVersion( const AllSetsUpdateChannel::ChannelType& channel ) const;

    // Commit file and version info to the application's storage area.
    bool commit( const AllSetsUpdateChannel::ChannelType& channel, const QString& filePath, const QString& version );

private:

    QDir getChannelDir( const AllSetsUpdateChannel::ChannelType& channel ) const;

    QDir                            mCacheDir;
    std::shared_ptr<spdlog::logger> mLogger;

};

#endif
