#include "catch.hpp"
#include "ImageCache.h"
#include <QTemporaryDir>
#include <QMap>
#include <QImage>
#include <QImageWriter>
#include <QColor>

static const int DEFAULT_MAX_BYTES = 1000000;

class TestHelper
{
public:

    enum ImageType { IMAGE_SMALL, IMAGE_MEDIUM, IMAGE_LARGE };

    static TestHelper* getInstance()
    {
        if( sInstance == nullptr ) sInstance = new TestHelper();
        return sInstance;
    }

    Logging::Config getLoggingConfig() { return mLoggingConfig; }

    QImage getImage( ImageType i )
    {
        if( !mImages.contains( i ) )
        {
            unsigned int dim = 100;
            if( i == IMAGE_MEDIUM ) dim = 200;
            if( i == IMAGE_LARGE ) dim = 300;
            QImage image( dim, dim, QImage::Format_ARGB32 );
            image.fill( Qt::red );
            mImages.insert( i, image );
        }
        return mImages.value( i );
    }

    QByteArray getImagePNGByteArray( ImageType i )
    {
        if( !mImagePNGByteArrays.contains( i ) )
        {
            // Create temporary dir to write temporary images to get file size.
            QTemporaryDir tempDir;
            CATCH_REQUIRE( tempDir.isValid() );
            QDir dir( tempDir.path() );

            // Write the image and check the size.
            QImage image = getImage( i );
            QString imageFilePath( dir.filePath( QString::number(i) + ".png" ) );
            QImageWriter imageWriter( imageFilePath );
            imageWriter.write( image );

            QFile imageFile( imageFilePath );
            imageFile.open( QIODevice::ReadOnly );
            mImagePNGByteArrays[i] = imageFile.readAll();
        }
        return mImagePNGByteArrays.value( i );
    }

    unsigned int getImagePNGSize( ImageType i ) { return getImagePNGByteArray( i ).size(); }

private:

    static TestHelper* sInstance;

    TestHelper()
    {
        mLoggingConfig.setName( "imagecache" );
        mLoggingConfig.setStdoutLogging( true );
        mLoggingConfig.setLevel( spdlog::level::debug );
    }

    Logging::Config mLoggingConfig;
    QMap<ImageType,QImage> mImages;
    QMap<ImageType,QByteArray> mImagePNGByteArrays;
};

TestHelper* TestHelper::sInstance = nullptr;



CATCH_TEST_CASE( "ImageCache - no existing files, no index", "[imagecache]" )
{
    // Create temporary directory.  This will self-delete at end of scope.
    QTemporaryDir dir;
    CATCH_REQUIRE( dir.isValid() );

    // create a cache
    ImageCache imageCache( dir.path(), DEFAULT_MAX_BYTES, TestHelper::getInstance()->getLoggingConfig() );

    // check no files managed, total size == 0
    CATCH_REQUIRE( imageCache.getCount() == 0 );
    CATCH_REQUIRE( imageCache.getCurrentBytes() == 0 );
}


CATCH_TEST_CASE( "ImageCache - existing files, no index", "[imagecache]" )
{
    // Create temporary directory.  This will self-delete at end of scope.
    QTemporaryDir tempDir;
    CATCH_REQUIRE( tempDir.isValid() );
    QDir dir( tempDir.path() );

    // Add some fake image files.
    QImage testImage = TestHelper::getInstance()->getImage( TestHelper::IMAGE_SMALL );
    const unsigned int imageSize = TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_SMALL );
    for( int i = 0; i < 10; ++i )
    {
        QString imageFilePath( dir.filePath( QString::number(i) + ".png" ) );
        QImageWriter imageWriter( imageFilePath );
        imageWriter.write( testImage );
    }

    CATCH_SECTION( "Right-sized cache" )
    {
        // Create a cache right-sized for the files.
        ImageCache imageCacheB( dir.path(), imageSize * 10, TestHelper::getInstance()->getLoggingConfig() );

        // Check that all files are managed.
        CATCH_REQUIRE( imageCacheB.getCount() == 10 );
        CATCH_REQUIRE( imageCacheB.getCurrentBytes() == imageSize * 10 );
    }

    CATCH_SECTION( "Oversized cache" )
    {
        // Create a cache oversized for the files.
        ImageCache imageCacheA( dir.path(), (imageSize * 10) + 1, TestHelper::getInstance()->getLoggingConfig() );

        // Check that all files are managed.
        CATCH_REQUIRE( imageCacheA.getCount() == 10 );
        CATCH_REQUIRE( imageCacheA.getCurrentBytes() == imageSize * 10 );
    }

    CATCH_SECTION( "Undersized cache" )
    {
        // Create a cache just undersized for the files.
        ImageCache imageCacheC( dir.path(), (imageSize * 10) - 1, TestHelper::getInstance()->getLoggingConfig() );

        // Check that one file was discarded.
        CATCH_REQUIRE( imageCacheC.getCount() == 9 );
        CATCH_REQUIRE( imageCacheC.getCurrentBytes() == imageSize * 9 );
    }
}

CATCH_TEST_CASE( "ImageCache - no existing files, existing index", "[imagecache]" )
{
    // Create temporary directory.  This will self-delete at end of scope.
    QTemporaryDir tempDir;
    CATCH_REQUIRE( tempDir.isValid() );
    QDir dir( tempDir.path() );

    // Create a cache, add some files, close it.
    {
        ImageCache imageCache( dir.path(), DEFAULT_MAX_BYTES, TestHelper::getInstance()->getLoggingConfig() );
        CATCH_REQUIRE( imageCache.tryWriteToCache( 0, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_SMALL ) ) );
        CATCH_REQUIRE( imageCache.tryWriteToCache( 1, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_MEDIUM ) ) );
        CATCH_REQUIRE( imageCache.tryWriteToCache( 2, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_LARGE ) ) );
        CATCH_REQUIRE( imageCache.getCount() == 3 );
        CATCH_REQUIRE( imageCache.getCurrentBytes() ==
                TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_SMALL ) +
                TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_MEDIUM ) +
                TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_LARGE ) );
    }

    // Remove all the non-index files from the directory.
    CATCH_REQUIRE( dir.remove( "0.png" ) );
    CATCH_REQUIRE( dir.remove( "1.png" ) );
    CATCH_REQUIRE( dir.remove( "2.png" ) );

    ImageCache imageCache( dir.path(), DEFAULT_MAX_BYTES, TestHelper::getInstance()->getLoggingConfig() );
    CATCH_REQUIRE( imageCache.getCount() == 0 );
    CATCH_REQUIRE( imageCache.getCurrentBytes() == 0 );

    CATCH_REQUIRE( imageCache.tryWriteToCache( 0, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_SMALL ) ) );
    CATCH_REQUIRE( imageCache.getCount() == 1 );
    CATCH_REQUIRE( imageCache.getCurrentBytes() ==
            TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_SMALL ) );

    CATCH_REQUIRE( imageCache.tryWriteToCache( 4, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_SMALL ) ) );
    CATCH_REQUIRE( imageCache.getCount() == 2 );
    CATCH_REQUIRE( imageCache.getCurrentBytes() ==
            TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_SMALL ) * 2 );
}

CATCH_TEST_CASE( "ImageCache - Cache ordering and purging", "[imagecache]" )
{
    // Create temporary directory.  This will self-delete at end of scope.
    QTemporaryDir tempDir;
    CATCH_REQUIRE( tempDir.isValid() );
    QDir dir( tempDir.path() );

    // Create a cache sized to fit one of each file.
    const unsigned int maxCacheSize = TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_SMALL ) +
                                      TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_MEDIUM ) +
                                      TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_LARGE );

    ImageCache imageCache( dir.path(), maxCacheSize, TestHelper::getInstance()->getLoggingConfig() );

    // Add one of each image file, smallest to largest.
    CATCH_REQUIRE( imageCache.tryWriteToCache( 0, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_SMALL ) ) );
    CATCH_REQUIRE( imageCache.tryWriteToCache( 1, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_MEDIUM ) ) );
    CATCH_REQUIRE( imageCache.tryWriteToCache( 2, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_LARGE ) ) );

    CATCH_REQUIRE( imageCache.getCount() == 3 );
    CATCH_REQUIRE( imageCache.getCurrentBytes() == maxCacheSize );

    CATCH_SECTION( "Purge oldest on write" )
    {
        QImage tmpImage;
        CATCH_REQUIRE( imageCache.tryWriteToCache( 3, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_SMALL ) ) );
        CATCH_REQUIRE( imageCache.getCount() == 3 );
        CATCH_REQUIRE( imageCache.getCurrentBytes() == maxCacheSize );

        CATCH_REQUIRE_FALSE( imageCache.tryReadFromCache( 0, tmpImage ) );
        CATCH_REQUIRE( imageCache.tryReadFromCache( 1, tmpImage ) );
        CATCH_REQUIRE( imageCache.tryReadFromCache( 2, tmpImage ) );
        CATCH_REQUIRE( imageCache.tryReadFromCache( 3, tmpImage ) );
    }

    CATCH_SECTION( "Purge oldest on write after reordering" )
    {
        QImage tmpImage;
        CATCH_REQUIRE( imageCache.tryReadFromCache( 0, tmpImage ) );
        CATCH_REQUIRE( imageCache.tryWriteToCache( 3, ".png", TestHelper::getInstance()->getImagePNGByteArray( TestHelper::IMAGE_SMALL ) ) );
        CATCH_REQUIRE( imageCache.getCount() == 3 );
        unsigned int expectedSize = maxCacheSize -
                                    TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_MEDIUM ) +
                                    TestHelper::getInstance()->getImagePNGSize( TestHelper::IMAGE_SMALL );
        CATCH_REQUIRE( imageCache.getCurrentBytes() == expectedSize );

        CATCH_REQUIRE_FALSE( imageCache.tryReadFromCache( 1, tmpImage ) );
        CATCH_REQUIRE( imageCache.tryReadFromCache( 0, tmpImage ) );
        CATCH_REQUIRE( imageCache.tryReadFromCache( 2, tmpImage ) );
        CATCH_REQUIRE( imageCache.tryReadFromCache( 3, tmpImage ) );
    }
}
