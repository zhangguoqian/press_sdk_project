cmake_minimum_required(VERSION 3.27)

project(press_desktop_qt VERSION 3.1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt 路径按以下优先级确定，CMake 中不硬编码：
# 1) -DCMAKE_PREFIX_PATH=... 或环境变量 CMAKE_PREFIX_PATH
# 2) PATH 中的 qmake（自动查询 QT_INSTALL_PREFIX）
if(NOT CMAKE_PREFIX_PATH)
    find_program(QT_QMAKE_EXECUTABLE NAMES qmake qmake.exe)
    if(QT_QMAKE_EXECUTABLE)
        execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_PREFIX
                        OUTPUT_VARIABLE _qt_prefix
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_qt_prefix)
            set(CMAKE_PREFIX_PATH "${_qt_prefix}")
        endif()
    endif()
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

# 单元测试：设备 JSON 解析与样例报文（ctest）
option(BUILD_QT_TESTS "Build press_desktop_qt unit tests" ON)
if(BUILD_QT_TESTS)
    enable_testing()
    add_executable(press_json_test
        tests/test_press_json.cpp
        src/core/press_json.cpp
    )
    target_include_directories(press_json_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(press_json_test PRIVATE Qt5::Core)
    target_compile_definitions(press_json_test PRIVATE
        FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME press_json_test COMMAND press_json_test)
endif()
