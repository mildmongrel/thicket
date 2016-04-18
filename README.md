# Thicket
Thicket is a tabletop draft simulator.

---

# Building (Linux)
These build notes apply to Ubuntu 14.04.3 or similar.

## Packages
Install the following packages to build client and/or server:

    sudo apt-get install build-essential
    sudo apt-get install cmake
    sudo apt-get install qt5base-dev
### Protobuf    
The protobuf package in Ubuntu 14.04.3 is too old (2.5.0).  Use a PPA to get protobuf 2.6.1:

    sudo add-apt-repository ppa:5-james-t/protobuf-ppa
    sudo apt-get update
    sudo apt-get install protobuf-compiler
On a more recent Ubuntu version it should be much simpler:

    sudo apt-get install protobuf-compiler

## Building
To build the the 'Release' version of the server:

    cd server
    mkdir build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    
To build the debug version, substitute 'Debug' for 'Release'.

To build the client, cd to the 'client' folder instead.

---

# Credits
Thicket stands on the shoulders of many folks' wonderful efforts.

## Software

- [Qt](http://www.qt.io)
  - Application framework used for both server and client
- [Google Protocol Buffers](https://developers.google.com/protocol-buffers/)
  - Messaging library
- [RapidJSON](http://rapidjson.org/)
  - Fast JSON parsing/generation for C++
- [spdlog](https://github.com/gabime/spdlog)
  - Super fast C++ logging library
- [catch](https://github.com/philsquared/Catch)
  - C++-native, header-only framework for unit-tests

## Other

- Card information provided by [MTG JSON](http://mtgjson.com/).
- Icon courtesy of [game-icons.net](http://game-icons.net/faq.html). License: [CC-BY-3.0](http://creativecommons.org/licenses/by/3.0/)

