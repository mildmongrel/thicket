#include "catch.hpp"
#include "NetConnectionServer.h"
#include "NetConnection.h"

#include <QCoreApplication>
#include <QTimer>

#define TEST_PORT 53330

#include "testnetconnection.h"

void
NetTestHarness::start()
{
    Logging::Config thisLoggingConfig;
    thisLoggingConfig.setName( "nettester" );
    thisLoggingConfig.setStdoutLogging( true );
    thisLoggingConfig.setLevel( spdlog::level::debug );
    mLogger = thisLoggingConfig.createLogger();

    mServer = new NetConnectionServer( this );

    Logging::Config clientConnLoggingConfig = thisLoggingConfig.createChildConfig( "clientconn" );
    clientConnLoggingConfig.setLevel( spdlog::level::trace );
    mClientConn = new NetConnection( clientConnLoggingConfig, this );

    // Establish connections.
    doConnect();

    run( mServerConn, mClientConn );

    // Disconnect everything.
    if( !mSkipDisconnect ) doDisconnect();

    /* Close everything and exit. */

    mClientConn->close();
    delete mClientConn;
    mServerConn->close();
    delete mServerConn;

    mServer->close();
    delete mServer;

    QCoreApplication::exit( 0 );
}


void
NetTestHarness::doConnect()
{
    /* Synchronous method of waiting for connections to establish */

    QTimer watchdogTimer;
    watchdogTimer.setSingleShot( true );
    QEventLoop loop;

    bool serverConnected = false;
    bool clientConnected = false;
    connect( mServer, &NetConnectionServer::incomingConnectionSocket, this,
        [&]( qintptr socketDescriptor ) {
            mLogger->debug( "server socket connected" );

            Logging::Config loggingConfig;
            loggingConfig.setName( "serverconn" );
            loggingConfig.setStdoutLogging( true );
            loggingConfig.setLevel( spdlog::level::trace );

            mServerConn = new NetConnection( loggingConfig, this );
            mServerConn->setSocketDescriptor( socketDescriptor );

            serverConnected = true;
            if( serverConnected && clientConnected )
                loop.quit();

        }, Qt::QueuedConnection ); // QueuedConnection avoids any direct call before event loop
    connect( mClientConn, &NetConnection::connected, this,
        [&]() {
            mLogger->debug( "client socket connected" );
            clientConnected = true;
            if( serverConnected && clientConnected )
                loop.quit();
        }, Qt::QueuedConnection ); // QueuedConnection avoids any direct call before event loop
    connect( &watchdogTimer, &QTimer::timeout, &loop, &QEventLoop::quit );

    mLogger->debug( "starting server..." );
    bool listening = mServer->listen( QHostAddress::Any, TEST_PORT );
    {
        mLogger->warn( "server error: {}", mServer->errorString().toStdString() );
        CATCH_REQUIRE( listening );
    }
    mLogger->debug( "server listening..." );

    mLogger->debug( "client connecting..." );
    mClientConn->connectToHost( "localhost", TEST_PORT );

    watchdogTimer.start( 100 );
    loop.exec();
    if( serverConnected && clientConnected )
        mLogger->debug( "both server and client connected" );
    else
        mLogger->error( "timeout waiting for both server and client to connect" );

    disconnect( &watchdogTimer, 0, 0, 0 );
    disconnect( mServer, &NetConnectionServer::incomingConnectionSocket, 0, 0 );
    disconnect( mClientConn, &NetConnection::connected, 0, 0 );

    // At this point handlers have run and connections should be valid and connected.
    CATCH_REQUIRE( mServerConn );
    CATCH_REQUIRE( mServerConn->state() == QAbstractSocket::ConnectedState );
    CATCH_REQUIRE( mClientConn->state() == QAbstractSocket::ConnectedState );

}


void
NetTestHarness::doDisconnect()
{
    /* Synchronous client disconnection. */

    QTimer watchdogTimer;
    watchdogTimer.setSingleShot( true );
    QEventLoop loop;

    mLogger->debug( "client disconnecting..." );
    connect( mClientConn, &NetConnection::disconnected, this,
        [&]() {
            mLogger->debug( "client disconnected" );
            loop.quit();
        }, Qt::QueuedConnection ); // QueuedConnection avoids any direct call before event loop
    connect( &watchdogTimer, &QTimer::timeout, &loop, &QEventLoop::quit );

    mClientConn->disconnectFromHost();

    watchdogTimer.start( 100 );
    loop.exec();
    CATCH_REQUIRE( watchdogTimer.isActive() );  // ensure no timeout

    disconnect( &watchdogTimer, 0, 0, 0 );
    disconnect( mClientConn, &NetConnection::msgReceived, 0, 0 );
}


void
TxRxNetTester::run( NetConnection* server, NetConnection* client )
{
    QByteArray zeroPayload;
    testTxRx( server, client, zeroPayload );
    testTxRx( client, server, zeroPayload );

    QByteArray smallPayload( "HELLO" );
    testTxRx( server, client, smallPayload );
    testTxRx( client, server, smallPayload );

    /* Medium payload tests */
    QByteArray mediumPayload;
    mediumPayload.fill( 'X', 15000 );

    testTxRx( server, client, mediumPayload );
    testTxRx( client, server, mediumPayload );

    client->setCompressionMode( NetConnection::COMPRESSION_MODE_UNCOMPRESSED );
    testTxRx( client, server, mediumPayload );

    client->setCompressionMode( NetConnection::COMPRESSION_MODE_COMPRESSED );
    testTxRx( client, server, mediumPayload );

    /* Large payload tests */

    QByteArray largePayload;
    largePayload.fill( 'X', 100000 );

    // Too large should fail to send.
    client->setCompressionMode( NetConnection::COMPRESSION_MODE_UNCOMPRESSED );
    CATCH_REQUIRE_FALSE( client->sendMsg( largePayload ) );

    client->setCompressionMode( NetConnection::COMPRESSION_MODE_COMPRESSED );
    testTxRx( client, server, largePayload );

    client->setCompressionMode( NetConnection::COMPRESSION_MODE_AUTO );
    testTxRx( client, server, largePayload );
}


void
TxRxNetTester::testTxRx( NetConnection* txConn, NetConnection* rxConn, const QByteArray& data )
{
    QTimer watchdogTimer;
    watchdogTimer.setSingleShot( true );
    QEventLoop loop;

    mLogger->debug( "sending data {} to {}...", (std::size_t)txConn, (std::size_t)rxConn );
    connect( rxConn, &NetConnection::msgReceived, this,
        [&]( const QByteArray& byteArray ) {
            CATCH_REQUIRE( (byteArray == data) );
            loop.quit();
        }, Qt::QueuedConnection ); // QueuedConnection avoids any direct call before event loop
    connect( &watchdogTimer, &QTimer::timeout, &loop, &QEventLoop::quit );

    bool sendOk = txConn->sendMsg( data );
    CATCH_REQUIRE( sendOk );

    watchdogTimer.start( 100 );
    loop.exec();
    CATCH_REQUIRE( watchdogTimer.isActive() );  // ensure no timeout

    disconnect( &watchdogTimer, 0, 0, 0 );
    disconnect( rxConn, &NetConnection::msgReceived, 0, 0 );
}


void
RxInactivityNetTester::run( NetConnection* server, NetConnection* client )
{
    QTimer watchdogTimer;
    watchdogTimer.setSingleShot( true );
    QEventLoop loop;

    connect( client, &NetConnection::disconnected, this, [&]() {
            loop.quit();
        }, Qt::QueuedConnection ); // QueuedConnection avoids any direct call before event loop
    connect( &watchdogTimer, &QTimer::timeout, &loop, &QEventLoop::quit );

    client->setRxInactivityAbortTime( 100 );
    mLogger->debug( "waiting for rx inactivity timeout..." );

    watchdogTimer.start( 200 );
    loop.exec();
    CATCH_REQUIRE( watchdogTimer.isActive() );  // ensure no timeout

    disconnect( &watchdogTimer, 0, 0, 0 );
    disconnect( client, &NetConnection::disconnected, 0, 0 );

    mSkipDisconnect = true;
}


CATCH_TEST_CASE( "NetConnection", "[netconn]" )
{
    int argc = 1;
    char argv[2][10];
    strcpy( argv[0], "dummy" );
    argv[1][0] = 0;
    CatchQCoreApplication app( argc, (char**)argv );

    CATCH_SECTION( "Transmit/Receive" )
    {
        // This will start the test from the application event loop.
        TxRxNetTester tester( &app );
        QTimer::singleShot( 0, &tester, SLOT(start()) );
        app.exec();
    }

    CATCH_SECTION( "Rx Inactivity Abort" )
    {
        // This will start the test from the application event loop.
        RxInactivityNetTester tester( &app );
        QTimer::singleShot( 0, &tester, SLOT(start()) );
        app.exec();
    }
}
