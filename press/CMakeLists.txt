cmake_minimum_required(VERSION 3.16)

project(press
    VERSION 1.0.0
    DESCRIPTION "Cross-platform machine control library"
    LANGUAGES CXX
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/press/version.h"
    @ONLY
)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(press SHARED ${LIB_SOURCES})

add_library(press::press ALIAS press)

if (MSVC)
    set_target_properties(press PROPERTIES DEBUG_POSTFIX "d")
endif ()

target_include_directories(press
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/press>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/press>
        $<INSTALL_INTERFACE:include/press>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(press PRIVATE PRESS_BUILDING_LIBRARY)

if (WIN32)
    target_compile_definitions(press PRIVATE
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0601
        NOMINMAX
    )
    target_link_libraries(press PRIVATE ws2_32 setupapi)

    if (MSVC)
        target_compile_options(press PRIVATE /W4 /utf-8)
        target_compile_definitions(press PRIVATE _CRT_SECURE_NO_WARNINGS)
    else ()
        target_compile_options(press PRIVATE -Wall -Wextra)
    endif ()

elseif (ANDROID)
    target_link_libraries(press PRIVATE log)
    target_compile_options(press PRIVATE -Wall -Wextra)

elseif (APPLE)
    target_compile_options(press PRIVATE -Wall -Wextra)

else ()
    target_link_libraries(press PRIVATE pthread)
    target_compile_options(press PRIVATE -Wall -Wextra)
endif ()

include(GNUInstallDirs)

install(TARGETS press
    EXPORT pressTargets
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/press/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/press
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/press/version.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/press
)

install(EXPORT pressTargets
    FILE pressTargets.cmake
    NAMESPACE press::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/press
)