# C++ interface test example / C++ 接口测试示例
#
# Build: 构建后在 build/test/cpp/ 下生成 press_cpp_test(.exe)

cmake_minimum_required(VERSION 3.27)

project(press_cpp_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(press_cpp_test main.cpp)

if (MSVC)
    target_compile_options(press_cpp_test PRIVATE /utf-8)
endif ()

target_link_libraries(press_cpp_test PRIVATE
    press::press
)

if (WIN32)
    target_link_libraries(press_cpp_test PRIVATE ws2_32 setupapi)
endif ()
