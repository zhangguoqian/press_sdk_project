cmake_minimum_required(VERSION 3.16)

project(press_desktop_qt VERSION 3.1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "C:/Qt/5.15.2/mingw81_64")
endif()

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(press_desktop_qt WIN32 ${APP_SOURCES})

target_include_directories(press_desktop_qt PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(press_desktop_qt PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
