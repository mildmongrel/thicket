#ifndef COMMANDERPANESETTINGS_H
#define COMMANDERPANESETTINGS_H

#include "ClientSettings.h"

// This class provides a facade to ClientSettings by setting the
// CommanderPane index.  Passing this to a CommanderPane class allows it
// to remain ignorant of its index.
class CommanderPaneSettings
{
public:

    CommanderPaneSettings( ClientSettings& clientSettings, int commanderPaneIndex )
      : mClientSettings( clientSettings ),
        mCommanderPaneIndex( commanderPaneIndex ) {}

    QString getZoom() const
    {
        return mClientSettings.getCommanderPaneZoom( mCommanderPaneIndex );
    }

    void setZoom( const QString& zoomStr )
    {
        mClientSettings.setCommanderPaneZoom( mCommanderPaneIndex, zoomStr );
    }

    QString getCategorization() const
    {
        return mClientSettings.getCommanderPaneCategorization( mCommanderPaneIndex );
    }

    void setCategorization( const QString& catStr )
    {
        mClientSettings.setCommanderPaneCategorization( mCommanderPaneIndex, catStr );
    }

    QString getSort() const
    {
        return mClientSettings.getCommanderPaneSort( mCommanderPaneIndex );
    }

    void setSort( const QString& sortStr )
    {
        mClientSettings.setCommanderPaneSort( mCommanderPaneIndex, sortStr );
    }

private:

    ClientSettings& mClientSettings;
    int             mCommanderPaneIndex;

};

#endif
