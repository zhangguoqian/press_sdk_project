cmake_minimum_required(VERSION 3.16)

project(press_desktop_qt VERSION 3.1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "C:/Qt/5.15.2/mingw81_32")
endif()

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(press_desktop_qt WIN32 ${APP_SOURCES})

target_include_directories(press_desktop_qt PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(press_desktop_qt PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

# SDK：作为根工程子目录构建时目标已存在；
# 在本目录独立构建时自动引入同级 press 工程。
if(NOT TARGET press::press)
    set(PRESS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../press")
    if(EXISTS "${PRESS_SDK_DIR}/CMakeLists.txt")
        add_subdirectory("${PRESS_SDK_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/press_sdk")
    endif()
endif()

if(TARGET press::press)
    target_link_libraries(press_desktop_qt PRIVATE press::press)
    target_compile_definitions(press_desktop_qt PRIVATE PRESS_SDK_ENABLED=1)
    if(WIN32)
        target_link_libraries(press_desktop_qt PRIVATE ws2_32 setupapi)
    endif()
    add_custom_command(TARGET press_desktop_qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:press>" "$<TARGET_FILE_DIR:press_desktop_qt>")
else()
    message(WARNING "未找到 press::press，设备通讯将不可用（仅本地模拟）")
endif()
