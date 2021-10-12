cmake_minimum_required(VERSION 3.12)

project(hydownloader-systray LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_QT5 "Build against Qt5" OFF)

if(USE_QT5)
    find_package(Qt5 COMPONENTS Widgets Gui Network REQUIRED)
else()
    find_package(Qt6 COMPONENTS Widgets Gui Network REQUIRED)
endif()

add_subdirectory(src/hydownloader-cpp)

add_executable(hydownloader-systray
    src/hydownloader-systray/main.cpp
    src/hydownloader-systray/mainwindow.cpp
    src/hydownloader-systray/mainwindow.h
    src/hydownloader-systray/jsonobjectdelegate.cpp
    src/hydownloader-systray/jsonobjectdelegate.h
    src/hydownloader-systray/mainwindow.ui
    src/hydownloader-systray/addurlsdialog.cpp
    src/hydownloader-systray/addurlsdialog.h
    src/hydownloader-systray/addurlsdialog.ui)

target_link_libraries(hydownloader-systray PRIVATE hydownloader-cpp::hydownloader-cpp)

if(USE_QT5)
    target_link_libraries(hydownloader-systray PRIVATE Qt5::Widgets Qt5::Network)
else()
    target_link_libraries(hydownloader-systray PRIVATE Qt6::Widgets Qt6::Network)
endif()

install(TARGETS hydownloader-systray RUNTIME DESTINATION bin)
