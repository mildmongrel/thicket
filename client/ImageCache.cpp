#include "ImageCache.h"

#include <QImageReader>
#include <QFile>
#include <QDir>
#include "qtutils_core.h"

ImageCache::ImageCache( const QDir&     imageCacheDir,
                        Logging::Config loggingConfig )
  : mImageCacheDir( imageCacheDir ),
    mLogger( loggingConfig.createLogger() )
{}


bool
ImageCache::tryReadFromCache( int multiverseId, QImage& image )
{
    if( !mImageCacheDir.exists() )
        return false;

    // Construct the filename with no extension.  QImageReader will try all
    // extensions it knows to read the file.
    const QString imageReaderFilename = mImageCacheDir.filePath( QString::number( multiverseId ) );

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
ImageCache::tryWriteToCache( int multiverseId, const QString& extension, const QByteArray& imageData )
{
    if( !mImageCacheDir.exists() )
        return false;

    const QString cacheFilename = mImageCacheDir.filePath( QString::number( multiverseId ) + extension );
    QFile cacheFile( cacheFilename );
    if( !cacheFile.open( QIODevice::WriteOnly ) )
    {
        mLogger->warn( "Could not open cachefile ({}) for writing - not caching image!", cacheFile.fileName() );
        return false;
    }

    cacheFile.write( imageData );
    cacheFile.close();
    mLogger->debug( "wrote image file {} to cache", cacheFile.fileName() );

    return true;
}

