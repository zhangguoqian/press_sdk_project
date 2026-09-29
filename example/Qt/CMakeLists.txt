cmake_minimum_required(VERSION 3.27)

project(press_bot_qtgui
    VERSION 1.0.0
    DESCRIPTION "Press Bot - Qt GUI application"
    LANGUAGES CXX
)

if (MSVC)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_PREFIX_PATH D:/Qt/5.15.2/msvc2019_64)
        message(STATUS "Qt example build type: msvc x64")
    else ()
        set(CMAKE_PREFIX_PATH D:/Qt/5.15.2/msvc2019)
        message(STATUS "Qt example build type: msvc x86")
    endif ()
elseif (MINGW)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_PREFIX_PATH D:/Qt/6.8.3/llvm-mingw_64)
        message(STATUS "Qt example build type: mingw x64")
    else ()
        set(CMAKE_PREFIX_PATH D:/Qt/5.15.2/mingw81_32)
        message(STATUS "Qt example build type: mingw x86")
    endif ()
elseif (ANDROID)
    set(CMAKE_PREFIX_PATH /opt/Qt/6.8.0/android_arm64_v8a)
    message(STATUS "Qt example build type: android arm64")
elseif (APPLE)
    set(CMAKE_PREFIX_PATH /opt/Qt/6.8.0/macos)
    message(STATUS "Qt example build type: macos")
else ()
    set(CMAKE_PREFIX_PATH /opt/rootfs-s6p4418/usr/local/Trolltech/Qt-6.10.0-nexell32)
    message(STATUS "Qt example build type: linux")
endif ()

find_package(Qt6 COMPONENTS Core Gui Widgets Charts QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Gui Widgets Charts REQUIRED)
    set(QT_VERSION 5)
else ()
    set(QT_VERSION 6)
endif ()

file(GLOB_RECURSE EXAMPLE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
)

add_executable(${PROJECT_NAME} WIN32 ${EXAMPLE_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE
    press::press
)

if (QT_VERSION EQUAL 5)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core Qt5::Gui Qt5::Widgets
        Qt5::Charts 
    )
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets
        Qt6::Charts 
    )
endif ()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/example
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 setupapi)

    if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
                set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            endif ()
        endif ()

        set(DEBUG_SUFFIX_GENEX "$<$<CONFIG:Debug>:d>")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX_GENEX}.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
        foreach (SQLDRIVER qsqlite qsqlodbc qsqlpsql)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_INSTALL_PATH}/plugins/sqldrivers/${SQLDRIVER}${DEBUG_SUFFIX_GENEX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
        endforeach ()

        foreach (QT_LIB Core Gui Widgets Charts)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_INSTALL_PATH}/bin/Qt${QT_VERSION}${QT_LIB}${DEBUG_SUFFIX_GENEX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endforeach ()
    endif ()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:press>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif ()