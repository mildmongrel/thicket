#include "catch.hpp"

#include <QObject>
#include <QByteArray>

#include <QCoreApplication>

// This is a QCoreApplication that can handle exceptions thrown by the
// Catch test framework and exit gracefully.
class CatchQCoreApplication : public QCoreApplication
{
public:
    CatchQCoreApplication( int& argc, char** argv )
      : QCoreApplication( argc, argv )
    {}

    virtual bool notify( QObject* receiver, QEvent* event ) override
    {
        bool done = true;
        try
        {
            done = QCoreApplication::notify( receiver, event );
        }
        catch( const Catch::TestFailureException& ex )
        {
            exit( 0 );
        }
        catch( ... )
        {
            std::cerr << "TestApplication: caught unexpected exception!!" << std::endl;
            exit( -1 );
        }
        return done;
    }
};


// Test harness that handles NetConnection setup/teardown around invoking
// a test function.
class NetTestHarness : public QObject
{
    Q_OBJECT

public:
    NetTestHarness( QObject* parent = nullptr )
      : QObject( parent ),
        mServer( nullptr ),
        mServerConn( nullptr ),
        mClientConn( nullptr ),
        mSkipDisconnect( false )
    {}
    virtual ~NetTestHarness() {}

public slots:

    void start();

protected:

    virtual void run( NetConnection* server, NetConnection* client ) = 0;

    std::shared_ptr<spdlog::logger> mLogger;

    void doConnect();
    void doDisconnect();

    NetConnectionServer* mServer;
    NetConnection*       mServerConn;
    NetConnection*       mClientConn;

    // run() can set this to true to skip the disconnect process.
    bool mSkipDisconnect;
};


class TxRxNetTester : public NetTestHarness
{
public:
    TxRxNetTester( QObject* parent = nullptr ) : NetTestHarness( parent ) {}
    virtual ~TxRxNetTester() {}
protected:
    virtual void run( NetConnection* server, NetConnection* client );
private:
    virtual void testTxRx( NetConnection* txConn, NetConnection* rxConn, const QByteArray& data, bool sendFailExpected = false );
};


class RxInactivityNetTester : public NetTestHarness
{
public:
    RxInactivityNetTester( QObject* parent = nullptr ) : NetTestHarness( parent ) {}
    virtual ~RxInactivityNetTester() {}
protected:
    virtual void run( NetConnection* server, NetConnection* client );
};

