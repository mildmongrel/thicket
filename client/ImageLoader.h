#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QMap>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
QT_END_NAMESPACE

class ImageCache;

#include "Logging.h"

class ImageLoader : public QObject
{
    Q_OBJECT

public:
    ImageLoader( ImageCache*     imageCache,
                 const QString&  cardImageUrlTemplateStr,
                 Logging::Config loggingConfig = Logging::Config(),
                 QObject*        parent = 0 );

    void loadImage( int multiverseId );

signals:
    void imageLoaded( int multiverseId, const QImage& image );

private slots:
    void networkAccessFinished( QNetworkReply *reply );

private:

    ImageCache* const        mImageCache;
    const QString            mCardImageUrlTemplateStr;
    QNetworkAccessManager*   mNetworkAccessManager;
    QMap<QNetworkReply*,int> mReplyToMuidMap;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // IMAGELOADER_H
