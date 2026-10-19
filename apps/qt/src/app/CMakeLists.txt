# SPDX-License-Identifier: GPL-3.0-or-later
qt_add_executable(stroopy main.cpp)

set_source_files_properties(qml/AppTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(stroopy
    URI Stroopy
    SOURCES
        gamecontroller.hpp gamecontroller.cpp
    QML_FILES
        qml/Main.qml
        qml/AppTheme.qml
        qml/HomePage.qml
        qml/GamePage.qml
        qml/ResultPage.qml
        qml/RingTimer.qml
)

target_link_libraries(stroopy PRIVATE Qt6::Quick Qt6::QuickControls2 stroopy_core)

set_target_properties(stroopy PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    QT_ANDROID_PACKAGE_NAME de.maggu2810.android.stroopy.qt
    QT_ANDROID_APP_NAME Stroopy
    QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE 1
    QT_ANDROID_MIN_SDK_VERSION 31
    QT_ANDROID_TARGET_SDK_VERSION 37
    QT_ANDROID_COMPILE_SDK_VERSION 37
)

if(NOT ANDROID)
    include(GNUInstallDirs)
    install(TARGETS stroopy BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
