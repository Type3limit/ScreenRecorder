cmake_minimum_required(VERSION 3.22...3.25)
project(screenRecorder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(BUILD_TYPE "Release")
if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(BUILD_TYPE "Debug")
endif()

find_package(Qt5 COMPONENTS
        Core
        Gui
        Svg
        Widgets
        Network
        REQUIRED)

link_directories(${CMAKE_CURRENT_LIST_DIR}/obs/lib/${BUILD_TYPE})

include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/obs/include
        ${CMAKE_CURRENT_LIST_DIR}/obs-plugins/include
        ${CMAKE_CURRENT_LIST_DIR}/dialogs
        ${CMAKE_CURRENT_LIST_DIR}/widgets
        ${CMAKE_CURRENT_LIST_DIR}/helper)

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
        dialogs/countdowndialog.cpp
        dialogs/countdowndialog.h
        dialogs/countdowndialog.ui
        dialogs/testwindow.cpp
        dialogs/testwindow.h
        dialogs/testwindow.ui
        helper/qoperationhelper.h
        helper/qoperationhelper.h
)
if(WIN32)
    list(APPEND RESOURCES
            helper/qhotkey_win.cpp
    )
elseif(APPLE)

    list(APPEND RESOURCES
            helper/qhotkey_mac.cpp
    )
elseif(UNIX)

    list(APPEND RESOURCES
            helper/qhotkey_x11.cpp
    )
endif()
add_executable(screenRecorder ${RESOURCES} main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE obs)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Svg
        Qt5::Widgets
        Qt5::Network
)
