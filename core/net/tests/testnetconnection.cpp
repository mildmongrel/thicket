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
    /* Run all combinations of packet sizes with header and commpression modes. */

    QByteArray zeroPayload;

    QByteArray smallPayload( "HELLO" );

    QByteArray mediumPayload;
    mediumPayload.fill( 'X', 15000 );

    // This payload is expected to compress within a brief message size.
    QByteArray largePayload;
    largePayload.fill( 'X', 100000 );

    const std::vector<NetConnection::CompressionMode> compressionModes = {
            NetConnection::COMPRESSION_MODE_AUTO,
            NetConnection::COMPRESSION_MODE_COMPRESSED,
            NetConnection::COMPRESSION_MODE_UNCOMPRESSED };

    const std::vector<NetConnection::HeaderMode> headerModes = {
            NetConnection::HEADER_MODE_AUTO,
            NetConnection::HEADER_MODE_BRIEF,
            NetConnection::HEADER_MODE_EXTENDED };

    for( auto compressionMode : compressionModes )
    {
        CATCH_INFO( "compression mode " << compressionMode );

        for( auto headerMode : headerModes )
        {
            CATCH_INFO( "header mode " << headerMode );

            client->setCompressionMode( compressionMode );
            client->setHeaderMode( headerMode );
            server->setCompressionMode( compressionMode );
            server->setHeaderMode( headerMode );

            testTxRx( server, client, zeroPayload );
            testTxRx( client, server, zeroPayload );

            testTxRx( server, client, smallPayload );
            testTxRx( client, server, smallPayload );

            testTxRx( server, client, mediumPayload );
            testTxRx( client, server, mediumPayload );

            bool sendFailExpected = false;
            if( (compressionMode == NetConnection::COMPRESSION_MODE_UNCOMPRESSED) && 
                (headerMode == NetConnection::HEADER_MODE_BRIEF) )
            {
                sendFailExpected = true;
            }

            testTxRx( client, server, largePayload, sendFailExpected );
            testTxRx( server, client, largePayload, sendFailExpected );
        }
    }
}


void
TxRxNetTester::testTxRx( NetConnection* txConn, NetConnection* rxConn, const QByteArray& data, bool sendFailExpected )
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
    if( !sendFailExpected )
    {
        CATCH_REQUIRE( sendOk );

        watchdogTimer.start( 100 );
        loop.exec();
        CATCH_REQUIRE( watchdogTimer.isActive() );  // ensure no timeout
    }
    else
    {
        CATCH_REQUIRE_FALSE( sendOk );
    }

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
#ifdef Q_OS_WIN
    // Using the original arguments (MS-extension __argc and __argv)
    // prevents a crash deep inside QCoreApplicationPrivate on Windows.
    // The arguments are meaningless to this test app anyway.
    CatchQCoreApplication app( __argc, __argv );
#else
    int argc = 1;
    char argv[2][10];
    strcpy( argv[0], "dummy" );
    argv[1][0] = 0;
    CatchQCoreApplication app( argc, (char**)argv );
#endif

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
