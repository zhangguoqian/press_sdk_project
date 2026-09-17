include(FetchContent)

# ====================== FetchContent 全局设置 ======================
# 必须放在 FetchContent_Declare 之前，否则对当前声明不生效
set(FETCHCONTENT_QUIET OFF)                  # 关闭安静模式，显示下载/构建日志（调试时有用）
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)    # 不自动更新仓库，只使用已下载好的源码
# set(FETCHCONTENT_FULLY_DISCONNECTED ON)    # 完全阻止网络访问（离线编译时取消注释）

# ====================== CSerialPort 构建选项 ======================
# 这些选项在 FetchContent_MakeAvailable 之前设置，会传递给 CSerialPort 的 CMakeLists.txt
option(CSERIALPORT_USE_LOCAL "使用本地已存在的 CSerialPort 源码（跳过下载）" OFF)
option(CSERIALPORT_BUILD_STATIC "构建 CSerialPort 为静态库" ON)
option(CSERIALPORT_ENABLE_DEBUG_LOG "启用 CSerialPort 调试日志" OFF)
option(CSERIALPORT_ENABLE_UTF8 "启用 CSerialPort UTF8 支持" OFF)

# ====================== 本地源码路径 ======================
set(CSERIALPORT_LOCAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cserialport")
set(CSERIALPORT_BINARY_DIR       "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/cserialport")

# 检测本地是否已有 CSerialPort 源码
if(EXISTS "${CSERIALPORT_LOCAL_SOURCE_DIR}/CMakeLists.txt")
    set(CSERIALPORT_USE_LOCAL ON)
    message(STATUS "[CSerialPort] 检测到本地源码，使用本地副本: ${CSERIALPORT_LOCAL_SOURCE_DIR}")
else()
    message(STATUS "[CSerialPort] 未检测到本地源码，将从远程仓库下载")
endif()

# ====================== 配置 CSerialPort 构建参数 ======================
# BUILD_SHARED_LIBS 控制动态/静态库
if(CSERIALPORT_BUILD_STATIC)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
endif()

# CSerialPort 自身选项
set(CSERIALPORT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CSERIALPORT_BUILD_BINDING_C OFF CACHE BOOL "" FORCE)
set(CSERIALPORT_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(CSERIALPORT_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(CSERIALPORT_ENABLE_DEBUG ${CSERIALPORT_ENABLE_DEBUG_LOG} CACHE BOOL "" FORCE)
set(CSERIALPORT_ENABLE_UTF8  ${CSERIALPORT_ENABLE_UTF8}      CACHE BOOL "" FORCE)

# ====================== 声明并获取 CSerialPort ======================
FetchContent_Declare(
        cserialport
        GIT_REPOSITORY https://gitee.com/itas109/CSerialPort.git
        GIT_TAG        v5.0.0
        GIT_SHALLOW    ON            # 浅克隆，只拉取指定 tag，减少下载量
        GIT_PROGRESS   ON            # 显示克隆进度
        TLS_VERIFY     ON            # 验证 HTTPS 证书

        SOURCE_DIR     ${CSERIALPORT_LOCAL_SOURCE_DIR}
        BINARY_DIR     ${CSERIALPORT_BINARY_DIR}
)

# 执行下载/更新 + add_subdirectory
FetchContent_MakeAvailable(cserialport)

# ====================== 构建后处理 ======================
# CSerialPort v5.0.0 内部使用老式 include_directories()，不会把 include 路径
# 传递给上层依赖方。这里手动为目标补上 PUBLIC include 目录，
# 这样主工程 target_link_libraries(... cserialport) 时就能自动拿到头文件搜索路径。
set(CSERIALPORT_INCLUDE_DIR "${CSERIALPORT_LOCAL_SOURCE_DIR}/include")

if(TARGET libcserialport)
    target_include_directories(libcserialport
        PUBLIC  ${CSERIALPORT_INCLUDE_DIR}
        PRIVATE ${CSERIALPORT_INCLUDE_DIR}
    )
    set_target_properties(libcserialport PROPERTIES
        CXX_STANDARD          17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF
    )
    if(MSVC)
        target_compile_options(libcserialport PRIVATE /W0)
    endif()
    if(MINGW)
        target_compile_options(libcserialport PRIVATE -w)
    endif()
    if(NOT TARGET cserialport)
        add_library(cserialport ALIAS libcserialport)
    endif()
    message(STATUS "[CSerialPort] 目标 'libcserialport' 已就绪 (别名: cserialport)")
    message(STATUS "[CSerialPort] include 目录: ${CSERIALPORT_INCLUDE_DIR}")
    message(STATUS "[CSerialPort] 库类型: ${BUILD_SHARED_LIBS}")
elseif(TARGET cserialport)
    target_include_directories(cserialport
        PUBLIC  ${CSERIALPORT_INCLUDE_DIR}
        PRIVATE ${CSERIALPORT_INCLUDE_DIR}
    )
    set_target_properties(cserialport PROPERTIES
        CXX_STANDARD          17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF
    )
    if(MSVC)
        target_compile_options(cserialport PRIVATE /W0)
    endif()
    if(MINGW)
        target_compile_options(cserialport PRIVATE -w)
    endif()
    message(STATUS "[CSerialPort] 目标 'cserialport' 已就绪")
    message(STATUS "[CSerialPort] include 目录: ${CSERIALPORT_INCLUDE_DIR}")
    message(STATUS "[CSerialPort] 库类型: ${BUILD_SHARED_LIBS}")
else()
    message(FATAL_ERROR "[CSerialPort] 未找到预期的 CSerialPort 目标，请检查版本兼容性")
endif()