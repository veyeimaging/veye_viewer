cmake_minimum_required(VERSION 3.16)

project(veye_viewer VERSION 1.1.3 LANGUAGES CXX)
# 若未指定构建类型，默认设为 Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 显式指定 Release 模式的编译优化（覆盖默认）
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -DNDEBUG -ffast-math")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -Wall -DNDEBUG -ffast-math")

# Qt 专属优化：禁用调试宏，减少运行时开销
add_definitions(-DQT_NO_DEBUG -DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT)
# 基础编译配置
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DQT_MESSAGELOGCONTEXT)
# 复制配置文件到编译目录
file(COPY  ${CMAKE_SOURCE_DIR}/config/ DESTINATION DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

file(COPY  ${CMAKE_SOURCE_DIR}/start_veye_viewer.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

# 生成版本头文件
configure_file(
    ${CMAKE_SOURCE_DIR}/Version.h.in
    ${CMAKE_SOURCE_DIR}/Version.h
    @ONLY
)

# -------------------------- Mali 库核心配置 --------------------------
# set(SYSTEM_LIB_DIR "/opt/atk-dlrk3588-toolchain/aarch64-buildroot-linux-gnu/sysroot/usr/lib")
# set(MALI_LIB_DIR "${SYSTEM_LIB_DIR}/mali")

# # 强制指定 Mali 库绝对路径
# set(GLES_LIB "${MALI_LIB_DIR}/libGLESv2.so")
# set(EGL_LIB "${MALI_LIB_DIR}/libEGL.so")
# set(MALI_LIB "${SYSTEM_LIB_DIR}/libmali.so")
# set(MALI_HOOK_LIB "${SYSTEM_LIB_DIR}/libmali-hook.so")

# # 库存在性验证
# if(NOT EXISTS ${GLES_LIB})
#     message(FATAL_ERROR "GLESv2 library not found: ${GLES_LIB}")
# endif()
# if(NOT EXISTS ${EGL_LIB})
#     message(FATAL_ERROR "EGL library not found: ${EGL_LIB}")
# endif()
# if(NOT EXISTS ${MALI_LIB})
#     message(FATAL_ERROR "Mali core library not found: ${MALI_LIB}")
# endif()
# if(NOT EXISTS ${MALI_HOOK_LIB})
#     message(FATAL_ERROR "Mali hook library not found: ${MALI_HOOK_LIB}")
# endif()

# # 设置链接器搜索路径
# link_directories(${MALI_LIB_DIR} ${SYSTEM_LIB_DIR})
# ----------------------------------------------------------------------------

# 查找 Qt 依赖库
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGL LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL LinguistTools)

# 翻译文件配置（关键修改1：指定绝对路径 + 确保文件存在）
set(TS_FILES
    ${CMAKE_SOURCE_DIR}/veye_viewer_zh_CN.ts
    ${CMAKE_SOURCE_DIR}/veye_viewer_en_US.ts
)
# 首次构建时创建空TS文件，避免文件缺失导致覆盖
foreach(ts_file ${TS_FILES})
    if(NOT EXISTS ${ts_file})
        file(WRITE ${ts_file} "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE TS>\n<TS version=\"2.1\"></TS>")
        message(STATUS "📄 创建初始翻译文件: ${ts_file}")
    endif()
endforeach()

# 头文件包含目录
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
)

# -------------------------- 目录自动搜索源文件（排除build目录） --------------------------
set(PROJECT_SOURCES)

# 1. 手动指定关键文件
list(APPEND PROJECT_SOURCES
    "Main.cpp"          # 主入口
    "resource.qrc"      # 资源文件
    "Version.h.in"      # 版本配置
)

# 2. 自动搜索业务代码文件（核心：排除build目录）
file(GLOB_RECURSE BUSINESS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

# 关键优化：排除所有build目录及其子目录（编译生成文件存放地）
list(FILTER BUSINESS_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER BUSINESS_SOURCES EXCLUDE REGEX ".*/out/.*")
list(FILTER BUSINESS_SOURCES EXCLUDE REGEX ".*/build-.*")
# 关键修改2：过滤TS文件，避免被加入源文件列表
list(FILTER BUSINESS_SOURCES EXCLUDE REGEX ".+\\.ts$")

# 3. 处理JSON第三方目录
file(GLOB_RECURSE JSON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/json/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/json/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/json/*.inl"
)

# 从业务代码中移除JSON文件
foreach(json_file ${JSON_SOURCES})
    list(REMOVE_ITEM BUSINESS_SOURCES ${json_file})
endforeach()

# 4. 组合所有源文件（关键修改3：移除TS_FILES，避免被当作源文件编译）
list(APPEND PROJECT_SOURCES ${BUSINESS_SOURCES} ${JSON_SOURCES})
list(REMOVE_DUPLICATES PROJECT_SOURCES)
list(SORT PROJECT_SOURCES)

# 5. 打印搜索结果
list(LENGTH PROJECT_SOURCES SRC_COUNT)
message(STATUS "✅ 自动搜索到的源文件（共 ${SRC_COUNT} 个）：")
foreach(file ${PROJECT_SOURCES})
    message(STATUS "  - ${file}")
endforeach()
# ----------------------------------------------------------------------------

# 生成可执行文件（适配 Qt 5/6）
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(veye_viewer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    # 关键修改4：Qt6 增量更新TS文件（-update：增量，-no-obsolete：移除过时翻译）
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES}
        OPTIONS -update -no-obsolete
    )
else()
    if(ANDROID)
        add_library(veye_viewer SHARED ${PROJECT_SOURCES})
    else()
        add_executable(veye_viewer ${PROJECT_SOURCES})
    endif()
    # 关键修改4：Qt5 增量更新TS文件
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES}
        OPTIONS -update -no-obsolete
    )
endif()

# 链接依赖库
target_link_libraries(veye_viewer PRIVATE
    pthread
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    # ${GLES_LIB}
    # ${EGL_LIB}
    # ${MALI_LIB}
    # ${MALI_HOOK_LIB}
)

# 目标属性配置
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.veye_viewer)
endif()
set_target_properties(veye_viewer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    INSTALL_RPATH "/usr/lib/aarch64-linux-gnu:/usr/lib/aarch64-linux-gnu/mali"
)

# 安装配置
include(GNUInstallDirs)
install(TARGETS veye_viewer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Qt 6 最终化配置
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(veye_viewer)
endif()
