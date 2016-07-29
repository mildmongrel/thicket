cmake_minimum_required (VERSION 2.8.12)

project (ThicketClient)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Add local modules to CMAKE_MODULE_PATH.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../ext/version_git/cmake/modules ${CMAKE_MODULE_PATH})

# Find necessary Qt packages
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5Svg)
find_package(Qt5Concurrent)

# Find the protobuf library
find_package(Protobuf REQUIRED)

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
include_directories(../core/proto)
include_directories(../core/qt)
include_directories(../core/util)
include_directories(${PROTOBUF_INCLUDE_DIRS})

qt5_add_resources(RESOURCES resources/client.qrc)

# This is here for windows MinGW to build the .rc file that sets the icon.
if(MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

if(WIN32)
    set(WIN_ICON_RESOURCES resources/client_winicon.rc)
endif(WIN32)

add_executable(thicketclient WIN32
    main.cpp
    client.cpp
    ClientSettings.cpp
    ImageCache.cpp
    ImageLoader.cpp
    ImageLoaderFactory.cpp
    FlowLayout.cpp
    CardWidget.cpp
    PlayerStatusWidget.cpp
    MtgJsonAllSetsFileCache.cpp
    AllSetsUpdateDialog.cpp
    MtgJsonAllSetsUpdateDialog.cpp
    ConnectDialog.cpp
    CreateRoomDialog.cpp
    CommanderPane.cpp
    CardViewerWidget.cpp
    BasicLandControlWidget.cpp
    DraftTimerWidget.cpp
    TickerWidget.cpp
    ServerViewWidget.cpp
    RoomViewWidget.cpp
    RoomConfigAdapter.cpp
    DeckStatsLauncher.cpp
    ../core/cards/MtgJsonAllSetsData.cpp
    ../core/cards/MtgJsonCardData.cpp
    ../core/cards/Decklist.cpp
    ../core/draft/DraftConfigAdapter.cpp
    ../core/qt/qtutils_widget.cpp
    ../core/qt/SizedSvgWidget.cpp
    ${PROTO_SRC_FILES}
    ${RESOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
    ${WIN_ICON_RESOURCES}
)

add_executable(tester
    tests/main.cpp
    tests/testroomconfigadapter.cpp
    tests/testimagecache.cpp
    RoomConfigAdapter.cpp
    ImageCache.cpp
    ../core/draft/DraftConfigAdapter.cpp
    ${PROTO_SRC_FILES}
    ${UTILS_SRC_FILES}
)

# catch.hpp unit testing includes
target_include_directories(tester PUBLIC ${CMAKE_SOURCE_DIR}/../ext/catch/include)
set_target_properties(tester PROPERTIES COMPILE_DEFINITIONS CATCH_CONFIG_PREFIX_ALL)

# rapidjson includes/definitions
include_directories(${CMAKE_SOURCE_DIR}/../ext/rapidjson/include)
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

# spdlog includes
include_directories(${CMAKE_SOURCE_DIR}/../ext/spdlog/include)
if(MINGW)
  # for windows compiling spdlog using mingw, fails to build without this
  add_definitions(-D_WIN32_WINNT=0x600)
endif()

# Use the necessary libraries from Qt 5.
target_link_libraries(thicketclient Qt5::Core Qt5::Widgets Qt5::Network Qt5::Svg Qt5::Concurrent)
target_link_libraries(tester Qt5::Core Qt5::Gui)

# Use the protobuf libraries
target_link_libraries(thicketclient ${PROTOBUF_LIBRARY})
target_link_libraries(tester ${PROTOBUF_LIBRARY})

# This works for GNU compilers (linux g++, mingw), but may not be OK for
# other platforms.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Installation
if(WIN32 AND NOT UNIX)
  INSTALL(TARGETS thicketclient DESTINATION .)
  INSTALL(FILES
      ${CMAKE_SOURCE_DIR}/../LICENSE
      ${CMAKE_SOURCE_DIR}/../RELNOTES.md
      DESTINATION .
  )
  IF(MINGW)
    # Compiler runtime DLLs
    get_filename_component(MINGW_DLL_DIR ${CMAKE_CXX_COMPILER} PATH)
    INSTALL(FILES
        ${MINGW_DLL_DIR}/libgcc_s_dw2-1.dll
        ${MINGW_DLL_DIR}/libstdc++-6.dll
        ${MINGW_DLL_DIR}/libwinpthread-1.dll
        DESTINATION .
    )
    # Qt DLLs
    get_target_property(QTCORE_LOCATION Qt5::Core LOCATION)
    get_filename_component(QT_DLL_DIR ${QTCORE_LOCATION} PATH)
    INSTALL(FILES
        ${QT_DLL_DIR}/Qt5Core.dll
        ${QT_DLL_DIR}/Qt5Gui.dll
	${QT_DLL_DIR}/Qt5Network.dll
	${QT_DLL_DIR}/Qt5Svg.dll
	${QT_DLL_DIR}/Qt5Widgets.dll
        DESTINATION .
    )
    # Qt Plugins
    INSTALL(FILES
        ${QT_DLL_DIR}/../plugins/platforms/qwindows.dll
        DESTINATION platforms
    )
    INSTALL(FILES
        ${QT_DLL_DIR}/../plugins/imageformats/qjpeg.dll
        DESTINATION imageformats
    )
    SET(CPACK_NSIS_DISPLAY_NAME "Thicket")
    SET(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    SET(CPACK_NSIS_MUI_FINISHPAGE_RUN "thicketclient.exe")
    SET(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
  ENDIF(MINGW)
endif(WIN32 AND NOT UNIX)

SET(CPACK_GENERATOR "ZIP;NSIS") # Create both ZIP and NSIS installer
SET(CPACK_PACKAGE_NAME "Thicket") # Otherwise uses proj name 'ThicketClient'
SET(CPACK_PACKAGE_VENDOR "Thicket") # Shows up in Windows 'Programs' area
SET(CPACK_PACKAGE_VERSION ${VERSION})
SET(CPACK_PACKAGE_EXECUTABLES "thicketclient;Thicket") # EXE and name of start menu link
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/../LICENSE)
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Thicket") # instead of default that includes version number
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0) # Don't insert extra directory in zip file
include(CPack)
