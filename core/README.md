Thicket core directory
======================

This directory contains code that is:

  - Closely coordinated between server and client (proto), or
  - Commonly used by both server and client (cards, logging, qt, util), or
  - Testable as a distinct entity (draft)


DIRECTORY  CLI  SVR  DESCRIPTION               DEPENDENCIES
---------  ---  ---  -----------               ------------

cards/      X    X   Card abstractions,        C++11 standard library,
                     MTG JSON database         rapidjson, catch (for tests)
                     handling, player
                     inventory

draft/           X   Abstracted management of  C++11 standard library,
                     draft mechanics           logging, catch (for tests)

logging/    X    X   General-purpose log       C++11 standard library,
                     configuration wrapped     spdlog
                     around spdlog

proto/      X    X   Thicket server/client     C++11 standard library,
                     protocol definition;      Google protobuf, cards
                     helper functions          (for helper functions)

qt/         X    X   Misc Qt-specific          C++11 standard library,
                     helper functions          Qt Core, Qt Widgets

test/                Harness for testing code  C++11 standard library,
                     in core directory         catch

util/       X    X   Misc helper functions,    C++11 standard library
                     random number generation

