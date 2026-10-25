# SPDX-License-Identifier: GPL-3.0-or-later
# The QML module is a static library, so the app and the UI test both link it.
qt_add_library(stroopy_ui STATIC)

set_source_files_properties(qml/AppTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(stroopy_ui
    URI Stroopy
    SOURCES
        appsettings.hpp appsettings.cpp
        backup.hpp backup.cpp
        gamecontroller.hpp gamecontroller.cpp
        gamepad.hpp gamepad.cpp
        haptics.hpp haptics.cpp
        highscores.hpp highscores.cpp
        links.hpp links.cpp
        multiplayer.hpp multiplayer.cpp
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
        qml/BackupPage.qml
        qml/MultiplayerPage.qml
        qml/LobbyPage.qml
)

target_include_directories(stroopy_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Qt6 REQUIRED COMPONENTS Network)
target_link_libraries(stroopy_ui PUBLIC Qt6::Quick Qt6::QuickControls2 Qt6::Network stroopy_core stroopy_net)

qt_add_executable(stroopy main.cpp)
target_compile_definitions(stroopy PRIVATE STROOPY_VERSION="${PROJECT_VERSION}")
target_link_libraries(stroopy PRIVATE Qt6::Quick Qt6::QuickControls2 stroopy_ui stroopy_uiplugin)
# Window icon; rendered from assets/artwork/icon-full.svg by tools/render-assets.sh.
qt_add_resources(stroopy "icons" PREFIX "/" FILES icons/stroopy.png)

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
    QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
    QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE 1
    QT_ANDROID_MIN_SDK_VERSION 31
    QT_ANDROID_TARGET_SDK_VERSION 37
    QT_ANDROID_COMPILE_SDK_VERSION 37
)

if(ANDROID)
    # TLS for wss:// relays: Qt's OpenSSL backend needs libssl/libcrypto in the APK.
    # Prebuilt OpenSSL 3 (Apache-2.0) from KDAB, as recommended by the Qt documentation.
    include(FetchContent)
    FetchContent_Declare(android_openssl
        URL https://github.com/KDAB/android_openssl/archive/b71f1470962019bd89534a2919f5925f93bc5779.tar.gz
        URL_HASH SHA256=6c1dfff0af367bba2f2c2a27f1c2061019537524dc7892ee8efbdc8e1c6224fa
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(stroopy)
endif()

if(NOT ANDROID)
    include(GNUInstallDirs)
    # Keep finding a Qt outside the system paths (e.g. ~/Qt-aqt) after installation.
    set_target_properties(stroopy PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
    install(TARGETS stroopy BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(UNIX AND NOT APPLE)
        # Menu entry and handler of stroopy:// links (x-scheme-handler/stroopy).
        install(FILES ../../linux/de.maggu2810.android.stroopy.qt.desktop
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
        install(FILES icons/stroopy.png RENAME de.maggu2810.android.stroopy.qt.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
    endif()
endif()
