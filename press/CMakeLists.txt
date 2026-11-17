cmake_minimum_required(VERSION 3.16)

project(press
    VERSION 1.0.0
    DESCRIPTION "Cross-platform machine control library"
    LANGUAGES CXX
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(JNI REQUIRED)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/press/version.h"
    @ONLY
)

set(LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/press.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/c/cpress.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jni/press_jni.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pressprivate.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/unity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/port/serialport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/port/tcpsocket.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/tool/jsondata.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/tool/jsontovalue.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/json/json_reader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/json/json_value.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/json/json_writer.cpp"
)

add_library(press SHARED ${LIB_SOURCES})

set_target_properties(press PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_library(press::press ALIAS press)

if (MSVC)
    set_target_properties(press PROPERTIES DEBUG_POSTFIX "d")
endif ()

target_include_directories(press
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/press>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/press>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${JNI_INCLUDE_DIRS}
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
include(CMakePackageConfigHelpers)

set(press_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/press")

install(TARGETS press
    EXPORT pressTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/press/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/press
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/press/version.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/press
)

# Install language binding sources so SDK consumers can access the same
# APIs from Java, C#, Python, JavaScript, and Dart without needing the repo tree.
# 安装各语言绑定源代码，方便 Java / C# / Python / JavaScript / Dart
# 使用方直接获取同一套跨语言 API，而不必依赖源码仓库目录结构。
install(DIRECTORY ${CMAKE_SOURCE_DIR}/interface/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/press_sdk/interface
    FILES_MATCHING
        PATTERN "*.cs" PATTERN "*.java" PATTERN "*.py" PATTERN "*.js" PATTERN "*.dart" PATTERN "*.yaml" PATTERN "*.yml"
        PATTERN ".gitignore" EXCLUDE
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pressConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pressConfig.cmake"
    INSTALL_DESTINATION ${press_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/pressConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT pressTargets
    FILE pressTargets.cmake
    NAMESPACE press::
    DESTINATION ${press_INSTALL_CMAKEDIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pressConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/pressConfigVersion.cmake"
    DESTINATION ${press_INSTALL_CMAKEDIR}
)