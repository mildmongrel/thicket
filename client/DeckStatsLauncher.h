#ifndef DECKSTATSLAUNCHER_H
#define DECKSTATSLAUNCHER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
QT_END_NAMESPACE

class Decklist;

class DeckStatsLauncher : public QObject
{
    Q_OBJECT

public:
    DeckStatsLauncher( QObject *parent = 0 );

    // Launch the query which should ultimately open up a browser link to
    // the deckstats.net deck page.  After completion, whether successful
    // or not,  this object will delete itself.
    void launch( const Decklist& decklist );

private slots:
    void queryFinished( QNetworkReply* reply );

private:
    QNetworkAccessManager* mNetworkAccessManager;
};

#endif

