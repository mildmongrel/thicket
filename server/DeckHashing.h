#ifndef DECKHASHING_H
#define DECKHASHING_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QCryptographicHash>
#include "PlayerInventory.h"

inline QString
computeCockatriceHash( const PlayerInventory& inv )
{
    QStringList deck;

    for( auto zone : { PlayerInventory::ZONE_MAIN, PlayerInventory::ZONE_SIDEBOARD } )
    {
        for( auto card : inv.getCards( zone ) )
        {
            QString name = QString::fromStdString( card->getName() );

            // Replace UTF-8 chars as Cockatrice does (see Cockatrice/common/decklist.cpp).
            name.replace( "Æ", "AE" );
            name.replace( "’", "'" );

            // Fix slashes for split cards as Cockatrice does.
            name.replace( QRegularExpression( "\\s*/+\\s*" ), " // " );

            name = name.toLower();
            if( zone == PlayerInventory::ZONE_SIDEBOARD ) name.prepend( "SB:" );
            deck.push_back( name );
        }

        BasicLandQuantities basics = inv.getBasicLandQuantities( zone );
        for( auto basic : gBasicLandTypeArray )
        {
            for( int i = 0; i < basics.getQuantity( basic ); ++i )
            {
                QString name = QString::fromStdString( stringify( basic ) ).toLower();
                if( zone == PlayerInventory::ZONE_SIDEBOARD ) name.prepend( "SB:" );
                deck.push_back( name );
            }
        }
    }

    deck.sort();
    QString joinedDeck = deck.join( ';' );

    QByteArray rawHash = QCryptographicHash::hash( joinedDeck.toUtf8(),
            QCryptographicHash::Sha1 );
    quint64 number =  (((quint64) (unsigned char) rawHash[0]) << 32)
                    + (((quint64) (unsigned char) rawHash[1]) << 24)
                    + (((quint64) (unsigned char) rawHash[2]) << 16)
                    + (((quint64) (unsigned char) rawHash[3]) << 8)
                    +   (quint64) (unsigned char) rawHash[4];
    QString hash = QString::number( number, 32 ).rightJustified( 8, '0' );

    return hash;
}

#endif
