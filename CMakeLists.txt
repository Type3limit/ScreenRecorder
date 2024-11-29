cmake_minimum_required(VERSION 3.22...3.25)
project(screenRecorder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(QT_DEFAULT_MAJOR_VERSION 5)
set(BUILD_TYPE "Release")
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(BUILD_TYPE "Debug")
endif()

if(WIN32)
    set(PLATFORM "windows")
elseif(APPLE)
    set(PLATFORM "macos")
elseif(UNIX)
    set(PLATFORM "linux")
endif()



set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib/${PLATFORM}/${BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib/${PLATFORM}/${BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin/${PLATFORM}/${BUILD_TYPE})
set(QT_PLUGIN_PATH ${CMAKE_CURRENT_LIST_DIR}/bin/${PLATFORM}/${BUILD_TYPE})

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS
        Core
        Gui
        Svg
        Widgets
        Network
        MultiMedia
        MultiMediaWidgets
        REQUIRED)

link_directories(${CMAKE_CURRENT_LIST_DIR}/obs/lib/${BUILD_TYPE})

include_directories(
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/obs/include
        ${CMAKE_CURRENT_LIST_DIR}/obs-plugins/include
        ${CMAKE_CURRENT_LIST_DIR}/dialogs
        ${CMAKE_CURRENT_LIST_DIR}/dialogs/upload
        ${CMAKE_CURRENT_LIST_DIR}/widgets
        ${CMAKE_CURRENT_LIST_DIR}/helper
        ${CMAKE_CURRENT_LIST_DIR}/../thirdpart/src/jsrt-api
        ${CMAKE_CURRENT_LIST_DIR}/../thirdpart/src/jsrt-api/base
        ${CMAKE_CURRENT_LIST_DIR}/../thirdpart/src/jsrt-api/helper)

set(RESOURCES resource.qrc
        dialogs/recordingwindow.cpp
        dialogs/recordingwindow.h
        dialogs/recordingwindow.ui
        widgets/volumecontrol.h
        widgets/volumecontrol.cpp
        helper/obswrapper.h
        helper/obswrapper.cpp
        helper/config.h
        helper/optionnalchain.h
        helper/extensionmethods.h
        dialogs/preview/videorenderer.h
        dialogs/preview/uploadnoticewindow.ui
        dialogs/preview/uploadnoticewindow.h
        dialogs/preview/uploadnoticewindow.cpp
        dialogs/preview/videopreviewdialog.ui
        dialogs/preview/videopreviewdialog.h
        dialogs/preview/videopreviewdialog.cpp
        dialogs/minimizedrecordingwindow.cpp
        dialogs/minimizedrecordingwindow.h
        dialogs/minimizedrecordingwindow.ui
        dialogs/backgroundwindow.cpp
        dialogs/backgroundwindow.h
        dialogs/backgroundwindow.h
        helper/qhotkey.h
        helper/qhotkey.cpp
        helper/qhotkey_p.h
        helper/debug.cpp
        helper/debug.h
        dialogs/dragmovedialog.h
        dialogs/dragmovedialog.cpp
        dialogs/updatenoticedialog.h
        dialogs/updatenoticedialog.cpp
        dialogs/updatenoticedialog.ui
        dialogs/upload/disableview.h
        dialogs/upload/disableview.cpp
        dialogs/upload/disableview.ui
        dialogs/upload/uploadpathwidget.h
        dialogs/upload/uploadpathwidget.cpp
        dialogs/upload/uploadoperator.h
        dialogs/upload/uploadoperator.cpp
        dialogs/upload/networkcolumnandfoklerwidget.h
        dialogs/upload/networkcolumnandfoklerwidget.cpp
        dialogs/upload/networkcolumnandfoklerwidget.ui
        dialogs/upload/foldermodel.h
        dialogs/upload/foldermodel.cpp
        dialogs/upload/cloumnmodel.h
        dialogs/upload/cloumnmodel.cpp
        dialogs/upload/cloumnchoosenwidget.h
        dialogs/upload/cloumnchoosenwidget.cpp
        dialogs/upload/cloumnchoosenwidget.ui
        dialogs/countdowndialog.cpp
        dialogs/countdowndialog.h
        dialogs/countdowndialog.ui
        dialogs/testwindow.cpp
        dialogs/testwindow.h
        dialogs/testwindow.ui
        dialogs/usermessagebox.h
        dialogs/usermessagebox.cpp
        dialogs/usermessagebox.ui
        dialogs/framelessdialog.h
        dialogs/framelessdialog.cpp
        dialogs/logindialog.h
        dialogs/logindialog.cpp
        dialogs/logindialog.ui
        helper/qoperationhelper.h
        helper/qoperationhelper.h
        commandlinedefinations.h
)

set(LinkLib
        Qt${QT_DEFAULT_MAJOR_VERSION}::Core
        Qt${QT_DEFAULT_MAJOR_VERSION}::Gui
        Qt${QT_DEFAULT_MAJOR_VERSION}::Svg
        Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets
        Qt${QT_DEFAULT_MAJOR_VERSION}::Network
        Qt${QT_DEFAULT_MAJOR_VERSION}::Multimedia
        Qt${QT_DEFAULT_MAJOR_VERSION}::MultimediaWidgets)

if(WIN32)
    list(APPEND RESOURCES
            helper/qhotkey_win.cpp
    )
    list(APPEND  LinkLib obs)

elseif(APPLE)

    list(APPEND RESOURCES
            helper/qhotkey_mac.cpp
    )
elseif(UNIX)

    find_package(X11 REQUIRED)

    find_package(Qt${QT_DEFAULT_MAJOR_VERSION} COMPONENTS X11Extras REQUIRED)

    include_directories(${X11_INCLUDE_DIR})
    include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

    list(APPEND LinkLib
            ${X11_LIBRARIES}
            Qt${QT_DEFAULT_MAJOR_VERSION}::X11Extras)


    list(APPEND RESOURCES
            helper/qhotkey_x11.cpp
    )

endif()
include(FetchContent)
include(ExternalProject)

set (JSRT_API_URL "http://192.168.6.120/QiuXiangyue/jsrt-api.git")
set (QT_AVPLAYER_URL "https://github.com/valbok/QtAVPlayer")

set(JSRT_API_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdPart/src/jsrt-api")
file(REAL_PATH ${JSRT_API_DIR} JSRT_API_DIR)

set(QT_AV_PLAYER_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdPart/src/QtAVPlayer")
file(REAL_PATH ${QT_AV_PLAYER_DIR} QT_AV_PLAYER_DIR)

#FetchContent_Declare(
#        jsrt-api
#        GIT_REPOSITORY ${JSRT_API_URL}
#        GIT_TAG "master"
#        SOURCE_DIR ${JSRT_API_DIR}
#  )
#
#FetchContent_Declare(
#        QtAVPlayer
#        GIT_REPOSITORY ${QT_AVPLAYER_URL}
#        GIT_TAG "master"
#        SOURCE_DIR ${QT_AV_PLAYER_DIR}
#)
#
#FetchContent_MakeAvailable(jsrt-api QtAVPlayer)

set(QT_AVPLAYER_MULTIMEDIA ON)

set(CMAKE_LIBRARY_PATH ../thirdPart/lib)

add_subdirectory(../thirdPart/src/jsrt-api jsrt-api)

include_directories(../thirdPart/src/QtAVPlayer/src)

set(QT_AVPLAYER_DIR ../thirdPart/src/QtAVPlayer/src/QtAVPlayer)

include(../thirdPart/src/QtAVPlayer/src/QtAVPlayer/QtAVPlayer.cmake)

if(WIN32)
    add_executable(${CMAKE_PROJECT_NAME} WIN32 ${RESOURCES} main.cpp ${QtAVPlayer_SOURCES})

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE appicon.rc)

    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE DbgHelp)
else()
    add_executable(${CMAKE_PROJECT_NAME}  ${RESOURCES} main.cpp appicon.rc ${QtAVPlayer_SOURCES})
endif()

#add_dependencies(${CMAKE_PROJECT_NAME} jsrt_api)
#
#add_dependencies(${CMAKE_PROJECT_NAME} QtAVPlayer)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${LinkLib}
        jsrt_api
        ${QtAVPlayer_LIBS})

if(UNIX)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libobs.so)
endif ()


