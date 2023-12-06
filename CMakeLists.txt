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


find_package(Qt5 COMPONENTS
        Core
        Gui
        Svg
        Widgets
        Network
        REQUIRED)

link_directories(${CMAKE_CURRENT_LIST_DIR}/obs/lib/${BUILD_TYPE})

include_directories(
        ${CMAKE_CURRENT_LIST_DIR}
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
        dialogs/usermessagebox.h
        dialogs/usermessagebox.cpp
        dialogs/usermessagebox.ui
        dialogs/framelessdialog.h
        dialogs/framelessdialog.cpp
        helper/qoperationhelper.h
        helper/qoperationhelper.h
)
set(LinkLib
        Qt5::Core
        Qt5::Gui
        Qt5::Svg
        Qt5::Widgets
        Qt5::Network)
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

if(WIN32)
    add_executable(${CMAKE_PROJECT_NAME} WIN32 ${RESOURCES} main.cpp)
else()
    add_executable(${CMAKE_PROJECT_NAME}  ${RESOURCES} main.cpp)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${LinkLib}
)
if(UNIX)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libobs.so)
endif ()