include(FetchContent)

# ====================== FetchContent 全局设置 ======================
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# ====================== JsonCpp 构建选项 ======================
option(JSONCPP_USE_LOCAL "使用本地已存在的 JsonCpp 源码（跳过下载）" OFF)
option(JSONCPP_BUILD_STATIC "构建 JsonCpp 为静态库" ON)

# ====================== 本地源码路径 ======================
set(JSONCPP_LOCAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jsoncpp")
set(JSONCPP_BINARY_DIR       "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/jsoncpp")

if(EXISTS "${JSONCPP_LOCAL_SOURCE_DIR}/CMakeLists.txt")
    set(JSONCPP_USE_LOCAL ON)
    message(STATUS "[JsonCpp] 检测到本地源码，使用本地副本: ${JSONCPP_LOCAL_SOURCE_DIR}")
else()
    message(STATUS "[JsonCpp] 未检测到本地源码，将从远程仓库下载")
endif()

# ====================== 配置 JsonCpp 构建参数 ======================
if(JSONCPP_BUILD_STATIC)
    set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS  ON  CACHE BOOL "" FORCE)
else()
    set(BUILD_SHARED_LIBS  ON  CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS  OFF CACHE BOOL "" FORCE)
endif()

set(BUILD_OBJECT_LIBS                OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_TESTS               OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_EXAMPLE             OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_CMAKE_PACKAGE       OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_PKGCONFIG_SUPPORT   OFF CACHE BOOL "" FORCE)

# ====================== 声明并获取 JsonCpp ======================
FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://gitee.com/keeyou/jsoncpp.git
        GIT_TAG        1.9.6
        GIT_SHALLOW    ON
        GIT_PROGRESS   ON
        TLS_VERIFY     ON

        SOURCE_DIR     ${JSONCPP_LOCAL_SOURCE_DIR}
        BINARY_DIR     ${JSONCPP_BINARY_DIR}
)

FetchContent_MakeAvailable(jsoncpp)

# ====================== 构建后处理 ======================
# 手动补全 PUBLIC include 目录，确保上层工程能正确找到 <json/json.h>
set(JSONCPP_INCLUDE_DIR "${JSONCPP_LOCAL_SOURCE_DIR}/include")

if(TARGET jsoncpp_static)
    target_include_directories(jsoncpp_static
        PUBLIC  ${JSONCPP_INCLUDE_DIR}
        PRIVATE ${JSONCPP_INCLUDE_DIR}
    )
    set_target_properties(jsoncpp_static PROPERTIES
        CXX_STANDARD          17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF
    )
    if(MSVC)
        target_compile_options(jsoncpp_static PRIVATE /W0)
    endif()
    if(MINGW)
        target_compile_options(jsoncpp_static PRIVATE -w)
    endif()
    if(NOT TARGET jsoncpp)
        add_library(jsoncpp ALIAS jsoncpp_static)
    endif()
    message(STATUS "[JsonCpp] 目标 'jsoncpp_static' 已就绪 (别名: jsoncpp)")
    message(STATUS "[JsonCpp] include 目录: ${JSONCPP_INCLUDE_DIR}")
    message(STATUS "[JsonCpp] 库类型: 静态")
elseif(TARGET jsoncpp_lib)
    target_include_directories(jsoncpp_lib
        PUBLIC  ${JSONCPP_INCLUDE_DIR}
        PRIVATE ${JSONCPP_INCLUDE_DIR}
    )
    set_target_properties(jsoncpp_lib PROPERTIES
        CXX_STANDARD          17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF
    )
    if(MSVC)
        target_compile_options(jsoncpp_lib PRIVATE /W0)
    endif()
    if(MINGW)
        target_compile_options(jsoncpp_lib PRIVATE -w)
    endif()
    if(NOT TARGET jsoncpp)
        add_library(jsoncpp ALIAS jsoncpp_lib)
    endif()
    message(STATUS "[JsonCpp] 目标 'jsoncpp_lib' 已就绪 (别名: jsoncpp)")
    message(STATUS "[JsonCpp] include 目录: ${JSONCPP_INCLUDE_DIR}")
    message(STATUS "[JsonCpp] 库类型: 动态")
else()
    message(FATAL_ERROR "[JsonCpp] 未找到预期的 JsonCpp 目标，请检查版本兼容性")
endif()