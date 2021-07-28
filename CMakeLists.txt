cmake_minimum_required(VERSION 3.12)

project(hydownloader-systray LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(USE_QT5)
    find_package(Qt5 COMPONENTS Widgets Gui Network REQUIRED)
else()
    find_package(Qt6 COMPONENTS Widgets Gui Network REQUIRED)
endif()

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

add_library(hydownloader
    src/hydownloader-cpp/hydownloaderconnection.h
    src/hydownloader-cpp/hydownloaderconnection.cpp
    src/hydownloader-cpp/hydownloaderlogmodel.h
    src/hydownloader-cpp/hydownloaderlogmodel.cpp
    src/hydownloader-cpp/hydownloadersingleurlqueuemodel.h
    src/hydownloader-cpp/hydownloadersingleurlqueuemodel.cpp
    src/hydownloader-cpp/hydownloadersubscriptionmodel.h
    src/hydownloader-cpp/hydownloadersubscriptionmodel.cpp
    src/hydownloader-cpp/hydownloaderjsonobjectlistmodel.h
    src/hydownloader-cpp/hydownloaderjsonobjectlistmodel.cpp
    src/hydownloader-cpp/hydownloadersubscriptionchecksmodel.cpp
    src/hydownloader-cpp/hydownloadersubscriptionchecksmodel.h)

target_include_directories(hydownloader-systray PRIVATE src/hydownloader-cpp)

if(USE_QT5)
    target_link_libraries(hydownloader PRIVATE Qt5::Core Qt5::Network Qt5::Gui)
    target_link_libraries(hydownloader-systray PRIVATE Qt5::Widgets Qt5::Network hydownloader)
else()
    target_link_libraries(hydownloader PRIVATE Qt6::Core Qt6::Network Qt6::Gui)
    target_link_libraries(hydownloader-systray PRIVATE Qt6::Widgets Qt6::Network hydownloader)
endif()

install(TARGETS hydownloader LIBRARY DESTINATION lib)
install(TARGETS hydownloader-systray RUNTIME DESTINATION bin)
