#include "catch.hpp"
#include "spdlog/spdlog.h"
#include "Decklist.h"

CATCH_TEST_CASE( "Decklist tests", "[decklist]" )
{
    CATCH_SECTION( "Empty/Clear" )
    {
        Decklist a;
        CATCH_REQUIRE( a.isEmpty() );
        a.addCard( "Test Card" );
        CATCH_REQUIRE_FALSE( a.isEmpty() );
        a.clear();
        CATCH_REQUIRE( a.isEmpty() );
        a.addCard( "Test Card", Decklist::ZONE_SIDEBOARD, 10 );
        CATCH_REQUIRE_FALSE( a.isEmpty() );
        a.clear();
        CATCH_REQUIRE( a.isEmpty() );
    }

    CATCH_SECTION( "Equality/Inequality" )
    {
        Decklist a;
        Decklist b;

        CATCH_SECTION( "Empty" )
        {
            CATCH_REQUIRE( a == b );
            CATCH_REQUIRE_FALSE( a != b );
        }
        CATCH_SECTION( "Single items" )
        {
            Decklist empty;

            a.addCard( "Test Card" );
            CATCH_REQUIRE_FALSE( a.isEmpty() );
            CATCH_REQUIRE( a != empty );
            CATCH_REQUIRE_FALSE( a == empty );

            b.addCard( "Test Card" );
            CATCH_REQUIRE( b != empty );
            CATCH_REQUIRE_FALSE( b == empty );

            CATCH_REQUIRE( a == b );
            CATCH_REQUIRE_FALSE( a != b );
        }
        CATCH_SECTION( "Single items qty" )
        {
            Decklist empty;

            a.addCard( "Test Card", Decklist::ZONE_MAIN, 10 );
            CATCH_REQUIRE_FALSE( a.isEmpty() );
            CATCH_REQUIRE( a != empty );
            CATCH_REQUIRE_FALSE( a == empty );

            b.addCard( "Test Card", Decklist::ZONE_MAIN, 10 );
            CATCH_REQUIRE( b != empty );
            CATCH_REQUIRE_FALSE( b == empty );

            CATCH_REQUIRE( a == b );
            CATCH_REQUIRE_FALSE( a != b );
        }
        CATCH_SECTION( "Different zones" )
        {
            Decklist empty;

            a.addCard( "Test Card", Decklist::ZONE_MAIN, 10 );
            CATCH_REQUIRE_FALSE( a.isEmpty() );
            CATCH_REQUIRE( a != empty );
            CATCH_REQUIRE_FALSE( a == empty );

            b.addCard( "Test Card", Decklist::ZONE_SIDEBOARD, 10 );
            CATCH_REQUIRE_FALSE( a.isEmpty() );
            CATCH_REQUIRE( b != empty );
            CATCH_REQUIRE_FALSE( b == empty );

            CATCH_REQUIRE( a != b );
            CATCH_REQUIRE_FALSE( a == b );

            // Now add to match
            a.addCard( "Test Card", Decklist::ZONE_SIDEBOARD, 10 );
            b.addCard( "Test Card", Decklist::ZONE_MAIN, 10 );
            CATCH_REQUIRE( a == b );
            CATCH_REQUIRE_FALSE( a != b );
        }
        CATCH_SECTION( "Multiples same" )
        {
            a.addCard( "Test Card Main A", Decklist::ZONE_MAIN, 10 );
            a.addCard( "Test Card Main B", Decklist::ZONE_MAIN, 5 );
            a.addCard( "Test Card Main C" );
            a.addCard( "Test Card SB A", Decklist::ZONE_SIDEBOARD, 10 );
            a.addCard( "Test Card SB B", Decklist::ZONE_SIDEBOARD, 5 );
            a.addCard( "Test Card SB C", Decklist::ZONE_SIDEBOARD );

            b.addCard( "Test Card Main A", Decklist::ZONE_MAIN, 10 );
            b.addCard( "Test Card Main B", Decklist::ZONE_MAIN, 5 );
            b.addCard( "Test Card Main C" );
            b.addCard( "Test Card SB A", Decklist::ZONE_SIDEBOARD, 10 );
            b.addCard( "Test Card SB B", Decklist::ZONE_SIDEBOARD, 5 );
            b.addCard( "Test Card SB C", Decklist::ZONE_SIDEBOARD );

            CATCH_REQUIRE_FALSE( a.isEmpty() );
            CATCH_REQUIRE( a == b );
            CATCH_REQUIRE_FALSE( a != b );
        }
        CATCH_SECTION( "Multiples diff" )
        {
            a.addCard( "Test Card Main A", Decklist::ZONE_MAIN, 10 );
            a.addCard( "Test Card Main B", Decklist::ZONE_MAIN, 5 );
            a.addCard( "Test Card Main C" );
            a.addCard( "Test Card SB A", Decklist::ZONE_SIDEBOARD, 10 );
            a.addCard( "Test Card SB B", Decklist::ZONE_SIDEBOARD, 5 );
            a.addCard( "Test Card SB C", Decklist::ZONE_SIDEBOARD );

            b.addCard( "Test Card Main A", Decklist::ZONE_MAIN, 10 );
            b.addCard( "Test Card Main B", Decklist::ZONE_MAIN, 4 ); // note diff here
            b.addCard( "Test Card Main C" );
            b.addCard( "Test Card SB A", Decklist::ZONE_SIDEBOARD, 10 );
            b.addCard( "Test Card SB B", Decklist::ZONE_SIDEBOARD, 5 );
            b.addCard( "Test Card SB C", Decklist::ZONE_SIDEBOARD );

            CATCH_REQUIRE( a != b );
            CATCH_REQUIRE_FALSE( a == b );

            Decklist c;
            c.addCard( "Test Card Main A", Decklist::ZONE_MAIN, 10 );
            c.addCard( "Test Card Main B", Decklist::ZONE_MAIN, 5 );
            c.addCard( "Test Card Main C" );
            c.addCard( "Test Card SB A", Decklist::ZONE_SIDEBOARD, 10 );
            c.addCard( "Test Card SB B", Decklist::ZONE_SIDEBOARD, 4 ); // note diff here
            c.addCard( "Test Card SB C", Decklist::ZONE_SIDEBOARD );

            CATCH_REQUIRE( a != c );
            CATCH_REQUIRE_FALSE( a == c );
        }
    }
    CATCH_SECTION( "Getters" )
    {
        Decklist d;
        std::vector<SimpleCardData> cards;
        
        cards = d.getCards( Decklist::ZONE_MAIN );
        CATCH_REQUIRE( cards.empty() );
        cards = d.getCards( Decklist::ZONE_SIDEBOARD );
        CATCH_REQUIRE( cards.empty() );

        d.addCard( "Test Card Main A", Decklist::ZONE_MAIN, 10 );
        d.addCard( "Test Card Main B", Decklist::ZONE_MAIN, 5 );
        d.addCard( "Test Card Main C" );
        d.addCard( "Test Card SB A", Decklist::ZONE_SIDEBOARD, 9 );
        d.addCard( "Test Card SB B", Decklist::ZONE_SIDEBOARD );

        cards = d.getCards( Decklist::ZONE_MAIN );
        CATCH_REQUIRE( cards.size() == 3 );
        CATCH_REQUIRE( d.getCardQuantity( SimpleCardData( "Test Card Main A", "" ),
                                          Decklist::ZONE_MAIN ) == 10 );
        CATCH_REQUIRE( d.getCardQuantity( SimpleCardData( "Test Card Main B", "" ),
                                          Decklist::ZONE_MAIN ) == 5 );
        CATCH_REQUIRE( d.getCardQuantity( SimpleCardData( "Test Card Main C", "" ),
                                          Decklist::ZONE_MAIN ) == 1 );
        CATCH_REQUIRE( d.getCardQuantity( SimpleCardData( "Test Card SB A", "" ),
                                          Decklist::ZONE_MAIN ) == 0 );

        cards = d.getCards( Decklist::ZONE_SIDEBOARD );
        CATCH_REQUIRE( cards.size() == 2 );
        CATCH_REQUIRE( d.getCardQuantity( SimpleCardData( "Test Card SB A", "" ),
                                          Decklist::ZONE_SIDEBOARD ) == 9 );
        CATCH_REQUIRE( d.getCardQuantity( SimpleCardData( "Test Card SB B", "" ),
                                          Decklist::ZONE_SIDEBOARD ) == 1 );
        CATCH_REQUIRE( d.getCardQuantity( SimpleCardData( "Test Card Main A", "" ),
                                          Decklist::ZONE_SIDEBOARD ) == 0 );
    }
    CATCH_SECTION( "Parsing" )
    {
        Decklist d;
        Decklist::ParseResult r;

        CATCH_SECTION( "Empty/Comments" )
        {
            r = d.parse( "" );
            CATCH_REQUIRE( d.isEmpty() );
            CATCH_REQUIRE_FALSE( r.hasErrors() );

            r = d.parse( "  " );
            CATCH_REQUIRE( d.isEmpty() );
            CATCH_REQUIRE_FALSE( r.hasErrors() );

            r = d.parse( "\t" );
            CATCH_REQUIRE( d.isEmpty() );
            CATCH_REQUIRE_FALSE( r.hasErrors() );

            r = d.parse( "// This is a comment" );
            CATCH_REQUIRE( d.isEmpty() );
            CATCH_REQUIRE_FALSE( r.hasErrors() );

            r = d.parse( "   // This is a comment" );
            CATCH_REQUIRE( d.isEmpty() );
            CATCH_REQUIRE_FALSE( r.hasErrors() );
        }

        CATCH_SECTION( "Errors" )
        {
            // Explicit quantity is required; some cards begin with a number which
            // would make it indistinguishable from a quantity.
            CATCH_SECTION( "No Quantity" )
            {
                r = d.parse( "Test Card" );
                CATCH_REQUIRE( r.hasErrors() );
                CATCH_REQUIRE( d.isEmpty() );

                r = d.parse( "SB Test Card" );
                CATCH_REQUIRE( r.hasErrors() );
                CATCH_REQUIRE( d.isEmpty() );

                r = d.parse( "[TST] Test Card" );
                CATCH_REQUIRE( r.hasErrors() );
                CATCH_REQUIRE( d.isEmpty() );
            }

            r = d.parse( "1Z Test Card" );
            CATCH_REQUIRE( d.isEmpty() );
            CATCH_REQUIRE( r.hasErrors() );

            r = d.parse( "1XZ Test Card" );
            CATCH_REQUIRE( d.isEmpty() );
            CATCH_REQUIRE( r.hasErrors() );
        }

        CATCH_SECTION( "DEC format" )
        {
            CATCH_SECTION( "Main" )
            {
                Decklist ref;
                ref.addCard( "Test Card", Decklist::ZONE_MAIN, 1 );

                r = d.parse( "1 Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( " 1 Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "1x Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( " 1x Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "1X Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "1 Test Card    " );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();
            }
            CATCH_SECTION( "Sideboard" )
            {
                Decklist ref;
                ref.addCard( "Test Card", Decklist::ZONE_SIDEBOARD, 1 );

                r = d.parse( "SB 1 Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "SB: 1 Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "sb: 1 Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "   SB:   1 Test Card   " );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();
            }
        }

        CATCH_SECTION( "MWDECK format" )
        {
            CATCH_SECTION( "Main" )
            {
                Decklist ref;
                ref.addCard( SimpleCardData( "Test Card", "TST" ), Decklist::ZONE_MAIN, 1 );

                r = d.parse( "1 [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( " 1 [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "1x [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( " 1x [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "1X [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "1 [TST] Test Card   " );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();
            }
            CATCH_SECTION( "Sideboard" )
            {
                Decklist ref;
                ref.addCard( SimpleCardData( "Test Card", "TST" ), Decklist::ZONE_SIDEBOARD, 1 );

                r = d.parse( "SB 1 [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "SB: 1 [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "sb: 1 [TST] Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();

                r = d.parse( "   SB:   1 [TST]  Test Card" );
                CATCH_REQUIRE( d == ref );
                CATCH_REQUIRE_FALSE( r.hasErrors() );
                d.clear();
            }
        }

        CATCH_SECTION( "Mixed formats and qtys" )
        {
            Decklist ref;
            ref.addCard( SimpleCardData( "Test Card A", "" ), Decklist::ZONE_MAIN, 11 );
            ref.addCard( SimpleCardData( "Test Card A", "" ), Decklist::ZONE_SIDEBOARD, 22 );
            ref.addCard( SimpleCardData( "Test Card B", "TST" ), Decklist::ZONE_MAIN, 33 );
            ref.addCard( SimpleCardData( "Test Card B", "TS2" ), Decklist::ZONE_SIDEBOARD, 44 );

            std::ostringstream os;
            os << "11 Test Card A" << std::endl;
            os << "SB: 22 Test Card A" << std::endl;
            os << "33 [TST] Test Card B" << std::endl;
            os << "SB: 44 [TS2] Test Card B" << std::endl;

            r = d.parse( os.str() );

            CATCH_REQUIRE( d == ref );
            CATCH_REQUIRE_FALSE( r.hasErrors() );
        }

        CATCH_SECTION( "DEC Loopback" )
        {
            Decklist ref;
            ref.addCard( SimpleCardData( "Test Card A", "" ), Decklist::ZONE_MAIN, 11 );
            ref.addCard( SimpleCardData( "Test Card A", "" ), Decklist::ZONE_SIDEBOARD, 22 );

            std::string s = ref.getFormattedString();
            r = d.parse( s );

            CATCH_REQUIRE( d == ref );
            CATCH_REQUIRE_FALSE( r.hasErrors() );
        }

        CATCH_SECTION( "MWDECK Loopback" )
        {
            Decklist ref;
            ref.addCard( SimpleCardData( "Test Card A", "" ), Decklist::ZONE_MAIN, 11 );
            ref.addCard( SimpleCardData( "Test Card A", "" ), Decklist::ZONE_SIDEBOARD, 22 );
            ref.addCard( SimpleCardData( "Test Card B", "TST" ), Decklist::ZONE_MAIN, 33 );
            ref.addCard( SimpleCardData( "Test Card B", "TS2" ), Decklist::ZONE_SIDEBOARD, 44 );

            std::string s = ref.getFormattedString( Decklist::FORMAT_MWDECK );
            r = d.parse( s );

            CATCH_REQUIRE( d == ref );
            CATCH_REQUIRE_FALSE( r.hasErrors() );
        }
    }
}

