#ifndef EXPSYMIMAGELOADER_H
#define EXPSYMIMAGELOADER_H

#include "CachedImageLoader.h"

#include "clienttypes.h"
#include "Logging.h"

class ExpSymImageLoader : public CachedImageLoader
{
    Q_OBJECT

public:
    ExpSymImageLoader( ImageCache*          imageCache,
                       const QString&       urlTemplateStr,
                       AllSetsDataSharedPtr allSetsData,
                       Logging::Config      loggingConfig = Logging::Config(),
                       QObject*             parent = 0 );

    void loadImage( const QString& setCode );

signals:
    void imageLoaded( const QString& setCode, const QImage& image );

protected:

    // Get cache image name for token.  Derived classes must implement this
    // to complete support for cache read/wrote.
    virtual QString getCacheImageName( const QVariant& token ) override;

private:

    const QString        mUrlTemplateStr;
    AllSetsDataSharedPtr mAllSetsData;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // EXPSYMIMAGELOADER_H
