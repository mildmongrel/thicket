#include "SizedImageCache.h"

#include <QImageReader>
#include <QFile>
#include <QSet>
#include "qtutils_core.h"

static const QString CACHE_INDEX_FILENAME  = ".cacheindex";
static const quint32 CACHE_INDEX_MAGIC_NUM = 0x000CAC3E;
static const quint16 CACHE_INDEX_VERSION   = 0x0100;      // 16-bit version: 8 bits major, 8 bits minor


QDataStream& operator<<( QDataStream &stream, const SizedImageCache::IndexEntry &entry )
{
    stream << entry.fileName;
    stream << entry.fileAccessDateTime;
    return stream;
}


QDataStream& operator>>( QDataStream &stream, SizedImageCache::IndexEntry &entry )
{
    stream >> entry.fileName;
    stream >> entry.fileAccessDateTime;
    return stream;
}


SizedImageCache::SizedImageCache( const QDir&     imageCacheDir,
                                  quint64         maxBytes,
                                  Logging::Config loggingConfig )
  : mCacheDir( imageCacheDir ),
    mCacheMaxBytes( maxBytes ),
    mCacheCurrentBytes( 0 ),
    mLogger( loggingConfig.createLogger() )
{
    deserializeCacheIndex();
    mLogger->debug( "deserialized image cache index: {} entries", mCacheIndex.size() );

    // Figure out which files are present or not preset by starting with
    // the index and removing all files that match in the cache.
    QSet<QString> rollCallFileNamesSet;
    for( auto entry : mCacheIndex ) rollCallFileNamesSet.insert( entry.fileName );

    // Get the list of all actual files in the cache directory.  If there
    // are files present that aren't in the index, append them to the index.
    // Appending will also maintain the ordering of newest to oldest.
    QFileInfoList fileInfoList( mCacheDir.entryInfoList( QStringList(), QDir::Files ) );
    mLogger->debug( "image cache contains {} files", fileInfoList.size() );
    for( QFileInfo info : fileInfoList )
    {
        // Skip the cache file itself.
        if( info.fileName() == CACHE_INDEX_FILENAME ) continue;

        mCacheCurrentBytes += info.size();

        if( rollCallFileNamesSet.contains( info.fileName() ) )
        {
            // File is accounted for.
            rollCallFileNamesSet.remove( info.fileName() );
        }
        else
        {
            // File appeared in the cache without being indexed.  Add to
            // the cache with the oldest possible timestamp.
            IndexEntry entry;
            entry.fileName = info.fileName();
            entry.fileAccessDateTime = QDateTime::fromTime_t( 0 );
            mCacheIndex.append( entry );
        }
    }

    // Remove entries in the index that weren't accounted for.
    for( auto fileName : rollCallFileNamesSet )
    {
        auto iter = mCacheIndex.begin();
        while( iter != mCacheIndex.end() )
        {
            if( fileName == iter->fileName )
            {
                mCacheIndex.erase( iter );
                break;
            }
            ++iter;
        }
    }

    mLogger->debug( "image cache contains {} bytes", mCacheCurrentBytes );
    mLogger->debug( "image cache index has {} entries after resolving cache", mCacheIndex.size() );

    // Ensure the cache is right-sized.
    resizeCache( mCacheMaxBytes );
}


SizedImageCache::~SizedImageCache()
{
    mLogger->debug( "serializing image cache index: {} entries", mCacheIndex.size() );
    serializeCacheIndex();
}


void
SizedImageCache::setMaxBytes( quint64 maxBytes )
{
    mCacheMaxBytes = maxBytes;
    resizeCache( mCacheMaxBytes );
}


bool
SizedImageCache::tryReadFromCache( const QString& nameWithoutExt, QImage& image )
{
    if( !mCacheDir.exists() )
        return false;

    // Construct the filename with no extension.  QImageReader will try all
    // extensions it knows to read the file.
    const QString imageReaderFilename = mCacheDir.filePath( nameWithoutExt );

    QImageReader reader( imageReaderFilename );
    image = reader.read();

    if( image.isNull() )
    {
        // There was a file error of some kind.
        if( reader.error() == QImageReader::FileNotFoundError )
        {
            mLogger->debug( "cache miss loading image {}", imageReaderFilename );
        }
        else
        {
            mLogger->warn( "error loading image {} from cache: {}", imageReaderFilename, reader.errorString() );
        }
        return false;
    }

    mLogger->debug( "loaded image file {} from cache", imageReaderFilename );

    // Update the cache access time.
    QString imageReaderActualFileName = QFileInfo( reader.fileName() ).fileName();
    for( auto iter = mCacheIndex.begin(); iter != mCacheIndex.end(); ++iter )
    {
        if( iter->fileName == imageReaderActualFileName )
        {
            // Update time stamp and move to front of index.
            IndexEntry entry = *iter;
            entry.fileAccessDateTime = QDateTime::currentDateTime();
            mCacheIndex.erase( iter );
            mCacheIndex.prepend( entry );
            break;
        }
    }

    return true;
}


bool
SizedImageCache::tryWriteToCache( const QString& nameWithoutExt, const QString& extension, const QByteArray& imageData )
{
    if( !mCacheDir.exists() )
        return false;

    const QString cacheFileName = nameWithoutExt + extension;
    const QString cacheFilePath = mCacheDir.filePath( cacheFileName );
    QFile cacheFile( cacheFilePath );
    if( !cacheFile.open( QIODevice::WriteOnly ) )
    {
        mLogger->warn( "Could not open cachefile ({}) for writing - not caching image!", cacheFile.fileName() );
        return false;
    }

    // Resize the cache (if necessary) to fit the new file.
    resizeCache( mCacheMaxBytes - imageData.size() );

    // Write the file.
    cacheFile.write( imageData );
    cacheFile.close();
    mLogger->debug( "wrote image file {} to cache", cacheFile.fileName() );

    // Create a new cache index entry and place at front of index.
    IndexEntry entry;
    entry.fileName = cacheFileName;
    entry.fileAccessDateTime = QDateTime::currentDateTime();
    mCacheIndex.prepend( entry );
    mCacheCurrentBytes += imageData.size();

    return true;
}


bool
SizedImageCache::deserializeCacheIndex()
{
    QFile file( mCacheDir.filePath( CACHE_INDEX_FILENAME ) );
    mLogger->debug( "deserializing image cache index file: {}", file.fileName() );

    if( !file.exists() )
    {
        mLogger->info( "image cache index file not found" );
        return false;
    }

    if( !file.open( QIODevice::ReadOnly ) )
    {
        mLogger->warn( "error opening image cache index file" );
        return false;
    }

    QDataStream in( &file );

    // Read and check the magic number.
    quint32 magic;
    in >> magic;
    if( magic != CACHE_INDEX_MAGIC_NUM )
    {
        mLogger->warn( "bad magic number in image cache index file" );
        return false;
    }

    // Read and check the version.
    quint16 version;
    in >> version;
    if( version != CACHE_INDEX_VERSION )
    {
        mLogger->warn( "unrecognized version in image cache index file" );
        return false;
    }

    in.setVersion(QDataStream::Qt_5_2);

    // Read the list.
    in >> mCacheIndex;

    return true;
}


bool
SizedImageCache::serializeCacheIndex()
{
    QFile file( mCacheDir.filePath( CACHE_INDEX_FILENAME ) );
    mLogger->debug( "serializing image cache index file: {}", file.fileName() );
    file.open( QIODevice::WriteOnly );
    QDataStream out( &file );

    // Write a header with a "magic number" and a version
    out << CACHE_INDEX_MAGIC_NUM;
    out << CACHE_INDEX_VERSION;

    out.setVersion(QDataStream::Qt_5_2);

    // Write the list.
    out << mCacheIndex;

    return true;
}


bool
SizedImageCache::resizeCache( quint64 maxSize )
{
    const quint32 startTotalBytes = mCacheCurrentBytes;

    //
    // Iterate backwards through the index, deleting files and index
    // entries until the cache size is under the limit.
    //

    while( !mCacheIndex.isEmpty() && (mCacheCurrentBytes > maxSize) )
    {
        // Remove the oldest index item.
        IndexEntry oldestIndexEntry = mCacheIndex.takeLast();

        mLogger->debug( "purging cached image file {}", oldestIndexEntry.fileName );

        // Get QFileInfo for the item.
        QFileInfo oldestFileInfo( mCacheDir.filePath( oldestIndexEntry.fileName ) );

        if( !oldestFileInfo.exists() )
        {
            mLogger->notice( "resizeCache: indexed file {} no longer exists", oldestIndexEntry.fileName );
            continue;
        }

        quint64 oldestFileSize = oldestFileInfo.size();

        // Try to delete the file.
        if( !mCacheDir.remove( oldestIndexEntry.fileName ) )
        {
            mLogger->warn( "resizeCache: failed to delete file {}", oldestIndexEntry.fileName );
            return false;
        }

        // Reduce the overall size by the file just deleted.
        mCacheCurrentBytes -= oldestFileSize;
    }

    if( mCacheCurrentBytes < startTotalBytes )
    {
        mLogger->debug( "image cache reduced from {} to {} total bytes", startTotalBytes, mCacheCurrentBytes );
    }

    return true;
}
