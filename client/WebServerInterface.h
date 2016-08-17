#ifndef WEBSERVERINTERFACE_H
#define WEBSERVERINTERFACE_H

class WebServerInterface
{
public:

    // Given server base, e.g. "http://thicketdraft.net:53332", get the downloads page forwarding link
    static QString getRedirectDownloadsUrl( QString serverBase )
    {
        return serverBase + "/redirect/downloads";
    }

    static QString getClientUpdateApiUrl( QString serverBase, QString clientVersion )
    {
        return serverBase + "/api/update/client/" + clientVersion;
    }

    static QString getMtgJsonAllSetsUpdateApiUrl( QString serverBase, QString clientVersion, QString allsetsVersion )
    {
        return serverBase + "/api/update/mtgjson/" + clientVersion + "?allsetsversion=" + allsetsVersion;
    }
};

#endif
