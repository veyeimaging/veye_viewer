cmake_minimum_required(VERSION 3.16)

project(veye_viewer VERSION 0.1.2 LANGUAGES CXX)


# add_compile_definitions(QT_NO_INFO_OUTPUT)
# add_compile_definitions(QT_NO_DEBUG_OUTPUT)
# add_compile_definitions(QT_NO_WARNING_OUTPUT)

file(COPY config/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/)
file(COPY config/start_veye_viewer.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_SOURCE_DIR}/version.h
    @ONLY
)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES veye_viewer_zh_CN.ts
  veye_viewer_en_US.ts)

INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
)

set(PROJECT_SOURCES
        main.cpp
        mainwidget.cpp
        mainwidget.h
        mainwidget.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(veye_viewer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET CamerViewer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(veye_viewer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(veye_viewer
            ${PROJECT_SOURCES}
            qxhelp.cpp qxhelp.h
            resource.qrc
            camera.h camera.cpp
            capture.h capture.cpp
            probedevice.h probedevice.cpp
            commheader.h
            capturemplane.h capturemplane.cpp
            capturesplane.h capturesplane.cpp
            json/autolink.h json/config.h json/features.h json/forwards.h json/json.h json/json_batchallocator.h json/json_internalarray.inl json/json_internalmap.inl json/json_reader.cpp json/json_value.cpp json/json_valueiterator.inl json/json_writer.cpp json/reader.h json/value.h json/writer.h
            qxtoast.cpp qxtoast.h qxtoast.ui
            msgbox.cpp msgbox.h msgbox.ui
            aboutdialog.cpp aboutdialog.h aboutdialog.ui
            operatecmd.h operatecmd.cpp
            commstuct.h
            expgaindlg.h expgaindlg.cpp expgaindlg.ui
            triggerdlg.h triggerdlg.cpp triggerdlg.ui
            imgattrdlg.h imgattrdlg.cpp imgattrdlg.ui
            registerdlg.h registerdlg.cpp registerdlg.ui
            curattrdlg.h curattrdlg.cpp curattrdlg.ui
            codemap.h codemap.cpp
            framelesswidget.cpp framelesswidget.h
            version.h.in
          )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(veye_viewer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.veye_viewer)
endif()
set_target_properties(veye_viewer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS veye_viewer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(veye_viewer)
endif()
