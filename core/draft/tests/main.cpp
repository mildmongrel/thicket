
//#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
//#include "catch.hpp"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

// This needs to be here for logging to work in the other files.
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

int main( int argc, char* const argv[] )
{
    // global setup...
    el::Loggers::getLogger("core");  // creates the "core" logger with default params

    int result = Catch::Session().run( argc, argv );

    // global clean-up...

    return result;
}
