#include "UnlimitedImageCache.h"

#include <QImageReader>
#include <QFile>
#include "qtutils_core.h"

UnlimitedImageCache::UnlimitedImageCache( const QDir&     imageCacheDir,
                                          Logging::Config loggingConfig )
  : mCacheDir( imageCacheDir ),
    mLogger( loggingConfig.createLogger() )
{
}


bool
UnlimitedImageCache::tryReadFromCache( const QString& nameWithoutExt, QImage& image )
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
    return true;
}


bool
UnlimitedImageCache::tryWriteToCache( const QString& nameWithoutExt, const QString& extension, const QByteArray& imageData )
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

    // Write the file.
    cacheFile.write( imageData );
    cacheFile.close();
    mLogger->debug( "wrote image file {} to cache", cacheFile.fileName() );

    return true;
}
