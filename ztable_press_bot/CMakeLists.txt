cmake_minimum_required(VERSION 3.27)
project(ztable_press_bot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(./)

if (MSVC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_PREFIX_PATH D:/Qt/5.15.2/msvc2019_64)
        message(build type = msvc x64)
    else()
        set(CMAKE_PREFIX_PATH D:/Qt/5.15.2/msvc2019)
        message(build type = msvc x86)
    endif()
elseif (MINGW)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_PREFIX_PATH D:/Qt/5.15.2/mingw81_64)
        message(build type = mingw x64)
    else()
        set(CMAKE_PREFIX_PATH D:/Qt/5.15.2/mingw81_32)
        message(current build type = mingw x86)
    endif()
else()
    set(CMAKE_PREFIX_PATH /opt/rootfs-s5p4418/usr/local/Trolltech/Qt-5.10.0-nexell32)
    message(build type = gun)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#include(FindCSerialPort)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        Charts
        SerialPort
        Sql
        REQUIRED)

file(GLOB_RECURSE UI_SOURCES ui/*.h ui/*.cpp ui/*.ui)
file(GLOB_RECURSE DATA_SOURCES data/*.h data/*.cpp)
file(GLOB_RECURSE CONTROL_SOURCES control/*.h control/*.cpp)
file(GLOB_RECURSE SQL_SOURCES sql/*.h sql/*.cpp)
file(GLOB_RECURSE LOGO_SOURCES logo.rc)


add_executable(${PROJECT_NAME} main.cpp ${UI_SOURCES} ${DATA_SOURCES} ${CONTROL_SOURCES} ${SQL_SOURCES} ${LOGO_SOURCES})

target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Charts
        Qt5::SerialPort
        Qt5::Sql
)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlodbc${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlpsql${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
    endif ()
    foreach (QT_LIB Core Gui Widgets Charts SerialPort Sql)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt5${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()

if(MINGW)
    file(GLOB ZGQ_MINGW_DLL_LIST "D:/Qt/Tools/mingw810_32/bin/*.dll")
    foreach (ZGQ_MINGW_DLL ${ZGQ_MINGW_DLL_LIST})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${ZGQ_MINGW_DLL}
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach ()
endif (MINGW)

set(CMAKE_C_FLAGS_RELEASE "-mwindows")
