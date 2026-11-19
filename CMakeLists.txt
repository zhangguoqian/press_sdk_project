cmake_minimum_required(VERSION 3.27)

project(press_sdk_project
    VERSION 1.0.0
    DESCRIPTION "Press SDK - Machine control library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Installation directory" FORCE)

add_subdirectory(press)

option(BUILD_QT_EXAMPLE "Build the Qt GUI example application" ON)
if (BUILD_QT_EXAMPLE)
    # add_subdirectory(example/Qt)
    add_subdirectory(example/press_desktop_qt)
endif ()

option(BUILD_CPP_TEST "Build the C++ interface test example" ON)
if (BUILD_CPP_TEST)
    add_subdirectory(test/cpp)
endif ()