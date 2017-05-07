#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int    gArgc;
char** gArgv;

int main( int argc, char* argv[] )
{
    // Save argc and argv globally, needed by QCoreApplication.
    gArgc = argc;
    gArgv = argv;

    int result = Catch::Session().run( argc, argv );
    return ( result < 0xff ? result : 0xff );
}


