cmake_minimum_required (VERSION 2.8.12)

project (Thicket)

include_directories(${CMAKE_SOURCE_DIR}/../draft)
include_directories(${CMAKE_SOURCE_DIR}/../cards)
include_directories(${CMAKE_SOURCE_DIR}/../logging)
include_directories(${CMAKE_SOURCE_DIR}/../util)

# rapidjson
include_directories(${CMAKE_SOURCE_DIR}/../../ext/rapidjson/include)
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

# spdlog
include_directories(${CMAKE_SOURCE_DIR}/../../ext/spdlog/include)
if(MINGW)
  # for windows compiling spdlog using mingw, fails to build without this
  add_definitions(-D_WIN32_WINNT=0x600)
endif()

add_executable(tester
    main.cpp
    ../draft/tests/testround.cpp
    ../draft/tests/teststate.cpp
    ../draft/tests/testtick.cpp
    ../draft/tests/testevents.cpp
    ../cards/CardPoolSelector.cpp
    ../cards/MtgJsonAllSetsData.cpp
    ../cards/MtgJsonCardData.cpp
    ../cards/PlayerInventory.cpp
    ../cards/tests/testmtgjson.cpp
    ../cards/tests/testcardpool.cpp
    ../cards/tests/testplayerinventory.cpp
    ../util/SimpleRandGen.cpp
    ../util/tests/testicompare.cpp
    ../util/tests/testrandgen.cpp
)

# catch.hpp unit testing includes
target_include_directories(tester PUBLIC ${CMAKE_SOURCE_DIR}/../../ext/catch/include)
set_target_properties(tester PROPERTIES COMPILE_DEFINITIONS CATCH_CONFIG_PREFIX_ALL)

#these require cmake 3.2
#set_property(TARGET thicket PROPERTY CXX_STANDARD 11)
#set_property(TARGET thicket PROPERTY CXX_STANDARD_REQUIRED ON)

# This works for GNU compilers (linux g++, mingw), but may not be OK for
# other platforms.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
