cmake_minimum_required(VERSION 3.16)

project(veye_viewer VERSION 1.3.0 LANGUAGES CXX)
# 若未指定构建类型，默认设为 Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 显式指定 Release 模式的编译优化（覆盖默认）
# 注意：这里刻意不加 -ffast-math。它会允许编译器重排/合并浮点运算并放松 IEEE 语义，
#       对上位机的增益、白平衡等浮点结果没有可感知收益，却可能引入像素值偏差。
#       同理：不要定义 QT_NO_DEBUG_OUTPUT / QT_NO_WARNING_OUTPUT，
#       那会把 qDebug()/qWarning() 在编译期整个去掉，导致日志文件基本为空。
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -Wall -DNDEBUG")

# 基础编译配置
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 保留日志中的文件/行号信息（配合 Main.cpp 的日志落盘）
add_definitions(-DQT_MESSAGELOGCONTEXT)
# 复制配置文件到编译目录（原写法把 DESTINATION 写了两遍，第二个被当成多余参数）
file(COPY  ${CMAKE_SOURCE_DIR}/config/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

file(COPY  ${CMAKE_SOURCE_DIR}/start_veye_viewer.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

# 生成版本头文件
# 必须生成到构建目录：之前生成到源码树，导致 Version.h 既是生成产物又被 git 跟踪，
# 每次 configure 都会把源码树改脏；而带引号的 #include "Version.h" 会先命中源码树里的旧副本。
configure_file(
    ${CMAKE_SOURCE_DIR}/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Version.h
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

# 翻译文件统一放在 config/ 下（仓库根目录的同名副本已删除，避免两份不同步）
set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/config/veye_viewer_zh_CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/config/veye_viewer_en_US.ts
)

# 构建时把 .ts 编译成 .qm，并同步到运行目录 build/config/（程序从 applicationDirPath()/config 加载 .qm）
# 说明：原来用的 qt5_create_translation() 会在每次构建时跑 lupdate 扫描整个源码树，
#       既把源码树里的 .ts 改脏（还会因 -no-obsolete 删掉存量译文），又拖慢构建，
#       所以改成“构建只做 lrelease，lupdate 交给显式目标”。
find_program(LRELEASE_EXECUTABLE NAMES lrelease-qt5 lrelease lrelease-qt6)
if(NOT LRELEASE_EXECUTABLE)
    message(WARNING "未找到 lrelease，跳过 .qm 生成（将直接使用仓库内已有的 .qm）")
else()
    set(TS_BASENAMES veye_viewer_zh_CN veye_viewer_en_US)
    set(TS_DEP_FILES "")
    set(TS_REL_FILES "")
    set(QM_SRC_FILES "")
    set(QM_OUT_FILES "")
    foreach(name ${TS_BASENAMES})
        list(APPEND TS_DEP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/${name}.ts)
        list(APPEND TS_REL_FILES ${name}.ts)
        list(APPEND QM_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/${name}.qm)
        list(APPEND QM_OUT_FILES ${CMAKE_CURRENT_BINARY_DIR}/config/${name}.qm)
    endforeach()
    # lrelease 的 -qm 只支持单个 .ts，因此在 config/ 目录内用相对文件名执行，生成的 .qm 会留在 config/（随仓库发布）
    add_custom_command(
        OUTPUT ${QM_OUT_FILES}
        COMMAND ${LRELEASE_EXECUTABLE} -silent ${TS_REL_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_SRC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/config/
        DEPENDS ${TS_DEP_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config
        COMMENT "lrelease: config/*.ts -> *.qm -> build/config/"
        VERBATIM
    )
    add_custom_target(translations ALL DEPENDS ${QM_OUT_FILES})
endif()

# 头文件包含目录（构建目录要排在前面，因为 Version.h 现在生成在那里）
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_BINARY_DIR}
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
# CONFIGURE_DEPENDS：新增/删除源文件后无需手动重新 configure，避免漏文件导致的“假成功”构建
file(GLOB_RECURSE BUSINESS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

# 关键优化：排除所有build目录及其子目录（编译生成文件存放地），以及本次 configure 的构建目录
# （构建目录可能在源码树之外，也可能叫 build-xxx / 其它名字，单靠正则盖不住）
list(FILTER BUSINESS_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER BUSINESS_SOURCES EXCLUDE REGEX ".*/out/.*")
list(FILTER BUSINESS_SOURCES EXCLUDE REGEX ".*/build-.*")
foreach(biz_file ${BUSINESS_SOURCES})
    if(biz_file MATCHES "^${CMAKE_BINARY_DIR}/")
        list(REMOVE_ITEM BUSINESS_SOURCES ${biz_file})
    endif()
endforeach()
# 过滤TS文件，避免被加入源文件列表
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

# 第三方代码不归本仓库管，不参与告警收敛（否则 -Wall 统计会被它几百条告警污染）
set_source_files_properties(${JSON_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")

# 4. 组合所有源文件（不再把 TS_FILES 当源文件传给编译器）
list(APPEND PROJECT_SOURCES ${BUSINESS_SOURCES} ${JSON_SOURCES})
list(REMOVE_DUPLICATES PROJECT_SOURCES)
list(SORT PROJECT_SOURCES)

# 5. 打印搜索结果（默认只报个数，-DPRINT_SOURCE_LIST=ON 时列出全部）
list(LENGTH PROJECT_SOURCES SRC_COUNT)
message(STATUS "✅ 自动搜索到的源文件（共 ${SRC_COUNT} 个）")
if(PRINT_SOURCE_LIST)
    foreach(file ${PROJECT_SOURCES})
        message(STATUS "  - ${file}")
    endforeach()
endif()

# 供 lupdate 扫描的代码文件（只喂真正的 .cpp/.h/.ui，避开 Version.h.in、json 里的 .inl）
set(CODE_FILES_FOR_LUPDATE)
foreach(file ${PROJECT_SOURCES})
    if(file MATCHES "\\.(cpp|h|ui)$" AND NOT file MATCHES "/json/")
        list(APPEND CODE_FILES_FOR_LUPDATE ${file})
    endif()
endforeach()
# ----------------------------------------------------------------------------

# 生成可执行文件（适配 Qt 5/6）
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(veye_viewer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(veye_viewer SHARED ${PROJECT_SOURCES})
    else()
        add_executable(veye_viewer ${PROJECT_SOURCES})
    endif()
endif()

# 翻译不再挂在默认构建流程里的 lupdate 上。需要把新增 tr() 字符串同步进 .ts 时显式执行：
#   cmake --build <build> --target update_translations
# 新串会以 type="unfinished" 写入 config/*.ts，人工补好译文后再构建，translations 目标会跑 lrelease。
find_program(LUPDATE_EXECUTABLE NAMES lupdate-qt5 lupdate lupdate-qt6)
if(LUPDATE_EXECUTABLE)
    add_custom_target(update_translations
        COMMAND ${LUPDATE_EXECUTABLE} ${CODE_FILES_FOR_LUPDATE} -ts ${TS_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "lupdate: 扫描代码更新 config/*.ts（新串需人工翻译，未完成前不会出英文）"
        VERBATIM
    )
    # 注意：故意不把 update_translations 挂到 translations/all 上，否则又变成每次构建都改源码树的老问题
endif()

if(TARGET translations)
    add_dependencies(veye_viewer translations)
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

# 安装配置：运行时从 applicationDirPath()/config/ 读配置与 .qm，所以 config 必须装到可执行文件旁边
include(GNUInstallDirs)
install(TARGETS veye_viewer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# 运行时要用的配置：.qm 译文、json 配置、探测脚本（脚本必须保留可执行位，程序会直接 exec）
# .ts 只是翻译源文件，不进发布目录
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/config/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/config
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING
            PATTERN "*.qm"
            PATTERN "*.json"
            PATTERN "*.sh")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/start_veye_viewer.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)

# Qt 6 最终化配置
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(veye_viewer)
endif()
