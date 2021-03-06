#ifndef CLIENTSETTINGS_H
#define CLIENTSETTINGS_H

#include <QObject>
#include "BasicLand.h"
#include "BasicLandMuidMap.h"
#include "AllSetsUpdateChannel.h"

#include "Logging.h"

QT_BEGIN_NAMESPACE
class QSettings;
class QDir;
QT_END_NAMESPACE

class ClientSettings : public QObject
{
    Q_OBJECT

public:

    ClientSettings( const QDir&            settingsDir,
                    const Logging::Config& loggingConfig,
                    QObject*               parent = 0 );

    void reset();

    QString getWebServiceBaseUrl() const;
    void    overrideWebServiceBaseUrl( const QString& override ) { mWebServiceBaseUrlOverride = override; }

    AllSetsUpdateChannel::ChannelType getAllSetsUpdateChannel() const;

    QStringList getConnectBuiltinServers() const;
    QStringList getConnectUserServers() const;
    void        setConnectUserServers( const QStringList& servers );
    bool        addConnectUserServer( const QString& server );
    QString     getConnectLastGoodServer() const;
    void        setConnectLastGoodServer( const QString& server );
    QString     getConnectLastGoodUsername() const;
    void        setConnectLastGoodUsername( const QString& username );

    QString getCardImageUrlTemplate() const;

    BasicLandMuidMap getBasicLandMultiverseIds() const;
    void setBasicLandMultiverseIds( const BasicLandMuidMap& basic );

    quint64 getImageCacheMaxSize() const;
    void    setImageCacheMaxSize( quint64 size );

    bool getBeepOnNewPack() const;
    void setBeepOnNewPack( bool beep );

    QByteArray getMainWindowGeometry() const;
    void       setMainWindowGeometry( const QByteArray& byteArray );

    QByteArray getDraftTabSplitterState() const;
    void       setDraftTabSplitterState( const QByteArray& byteArray );

    //
    // CommanderPane settings are done by index of pane.
    //

    QString getCommanderPaneZoom( int index ) const;
    void    setCommanderPaneZoom( int index, const QString& zoomStr );

    QString getCommanderPaneCategorization( int index ) const;
    void    setCommanderPaneCategorization( int index, const QString& catStr );

    QString getCommanderPaneSort( int index ) const;
    void    setCommanderPaneSort( int index, const QString& sortStr );

signals:

    void imageCacheMaxSizeChanged( unsigned int size );
    void basicLandMultiverseIdsChanged( const BasicLandMuidMap& muidMap );

private:

    QSettings* settings;
    QString    mWebServiceBaseUrlOverride;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif
