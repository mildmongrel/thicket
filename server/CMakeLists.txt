cmake_minimum_required (VERSION 2.8.12)

project (ThicketServer)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Add local modules to CMAKE_MODULE_PATH.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../ext/version_git/cmake/modules ${CMAKE_MODULE_PATH})

# Find necessary Qt packages
find_package(Qt5Network)

# Find the protobuf library
find_package(Protobuf REQUIRED)

# spdlog includes/definitions
include_directories(${CMAKE_SOURCE_DIR}/../ext/spdlog/include)
if(MINGW)
  # for windows compiling spdlog using mingw, fails to build without this
  add_definitions(-D_WIN32_WINNT=0x600)
endif()

# rapidjson includes/definitions
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)
include_directories(${CMAKE_SOURCE_DIR}/../ext/rapidjson/include)

# cpp-lru-cache includes
include_directories(${CMAKE_SOURCE_DIR}/../ext/cpp-lru-cache/include)

# Load VERSION with current tag/hash from git.
include(GetGitRevisionDescription)
git_describe(VERSION --tags --dirty=-dirty)

# Generate the version file.
message(STATUS "Configuring version file with VERSION=${VERSION}")
configure_file(version.cpp.in version.cpp)

set(PROTOBUF_IMPORT_DIRS ${PROTOBUF_IMPORT_DIRS} ../core/draft)
set(PROTO_MSG_FILES ../core/proto/messages.proto ../core/draft/DraftConfig.proto)
PROTOBUF_GENERATE_CPP( PROTO_SRC_FILES PROTO_HDR_FILES ${PROTO_MSG_FILES} )

include_directories(${CMAKE_SOURCE_DIR})
include_directories(../core/cards)
include_directories(../core/draft)
include_directories(../core/logging)
include_directories(../core/net)
include_directories(../core/proto)
include_directories(../core/qt)
include_directories(../core/util)
include_directories(${PROTOBUF_INCLUDE_DIRS})

set(CARDS_SRC_DIR ../core/cards)
set(CARDS_SRC_FILES
    ${CARDS_SRC_DIR}/CardPoolSelector.cpp
    ${CARDS_SRC_DIR}/MtgJsonAllSetsData.cpp
    ${CARDS_SRC_DIR}/MtgJsonCardData.cpp
    ${CARDS_SRC_DIR}/PlayerInventory.cpp
)

set(DRAFT_SRC_DIR ../core/draft)
set(DRAFT_SRC_FILES
    ${DRAFT_SRC_DIR}/DraftConfigAdapter.cpp
)

set(UTILS_SRC_DIR ../core/util)
set(UTILS_SRC_FILES
    ${UTILS_SRC_DIR}/SimpleRandGen.cpp
)

add_executable(thicketserver
    main.cpp
    AdminShell.cpp
    Server.cpp
    ServerSettings.cpp
    ClientNotices.cpp
    ServerRoom.cpp
    ClientConnection.cpp
    ConnectionServer.cpp
    HumanPlayer.cpp
    RoomConfigValidator.cpp
    CardDispenserFactory.cpp
    BoosterDispenser.cpp
    CustomCardListDispenser.cpp
    ${PROTO_SRC_FILES}
    ${CARDS_SRC_FILES}
    ${DRAFT_SRC_FILES}
    ${UTILS_SRC_FILES}
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

add_executable(tester
    tests/main.cpp
    tests/testdeckhashing.cpp
    tests/testroomconfigvalidator.cpp
    tests/testboosterdispenser.cpp
    tests/testcustomcardlistdispenser.cpp
    tests/testcarddispenserfactory.cpp
    ../core/net/tests/testnetconnection.cpp
    ../core/net/NetConnection.cpp
    ../core/net/NetConnectionServer.cpp
    RoomConfigValidator.cpp
    BoosterDispenser.cpp
    CustomCardListDispenser.cpp
    CardDispenserFactory.cpp
    ${CARDS_SRC_FILES}
    ${PROTO_SRC_FILES}
    ${UTILS_SRC_FILES}
)

# catch.hpp unit testing includes
target_include_directories(tester PUBLIC ${CMAKE_SOURCE_DIR}/../ext/catch/include)
set_target_properties(tester PROPERTIES COMPILE_DEFINITIONS CATCH_CONFIG_PREFIX_ALL)

# Use the Widgets and Network modules from Qt 5.
target_link_libraries(thicketserver Qt5::Core Qt5::Network)
target_link_libraries(tester Qt5::Core Qt5::Network)

# Use the protobuf libraries
target_link_libraries(thicketserver ${PROTOBUF_LIBRARY})
target_link_libraries(tester ${PROTOBUF_LIBRARY})

# This works for GNU compilers (linux g++, mingw), but may not be OK for
# other platforms.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


