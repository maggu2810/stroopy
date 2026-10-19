# SPDX-License-Identifier: GPL-3.0-or-later
# The QML module is a static library, so the app and the UI test both link it.
qt_add_library(stroopy_ui STATIC)

set_source_files_properties(qml/AppTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(stroopy_ui
    URI Stroopy
    SOURCES
        appsettings.hpp appsettings.cpp
        gamecontroller.hpp gamecontroller.cpp
        gamepad.hpp gamepad.cpp
        haptics.hpp haptics.cpp
        highscores.hpp highscores.cpp
        sharing.hpp sharing.cpp
    QML_FILES
        qml/Main.qml
        qml/AppTheme.qml
        qml/SubPage.qml
        qml/HomePage.qml
        qml/GamePage.qml
        qml/ResultPage.qml
        qml/RingTimer.qml
        qml/HighscoresPage.qml
        qml/EnterCodePage.qml
        qml/SettingsPage.qml
        qml/HelpPage.qml
)

target_include_directories(stroopy_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stroopy_ui PUBLIC Qt6::Quick Qt6::QuickControls2 stroopy_core)

qt_add_executable(stroopy main.cpp)
target_compile_definitions(stroopy PRIVATE STROOPY_VERSION="${PROJECT_VERSION}")
target_link_libraries(stroopy PRIVATE Qt6::Quick Qt6::QuickControls2 stroopy_ui stroopy_uiplugin)

# Translations: i18n/Stroopy_<lang>.ts, compiled into the :/i18n resource prefix of the app.
qt_add_translations(stroopy
    SOURCE_TARGETS stroopy_ui
    TS_FILE_DIR i18n
    LUPDATE_OPTIONS -no-obsolete -locations none
)

set_target_properties(stroopy PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    QT_ANDROID_PACKAGE_NAME de.maggu2810.android.stroopy.qt
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../android
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
