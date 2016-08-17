#ifndef ALLSETSUPDATECHANNEL_H
#define ALLSETSUPDATECHANNEL_H

class AllSetsUpdateChannel
{
public:
    enum ChannelType
    {
        CHANNEL_STABLE,
        CHANNEL_MTGJSON,
        CHANNEL_UNKNOWN
    };

    static inline ChannelType getDefaultChannel()
    {
        // Currently the default is the stable channel because the main
        // server is on the stable channel anyway and there's no reason
        // for clients to get ahead of it.
        return CHANNEL_STABLE;
    }

    static inline ChannelType stringToChannel( const QString& str )
    {
        if( str == "stable" ) return CHANNEL_STABLE;
        if( str == "mtgjson" ) return CHANNEL_MTGJSON;
        return CHANNEL_UNKNOWN;
    }

    static inline QString channelToString( const ChannelType& channel )
    {
        switch( channel )
        {
            case CHANNEL_STABLE:  return "stable";
            case CHANNEL_MTGJSON: return "mtgjson";
            default:              return QString();
        }
    }
};

#endif
