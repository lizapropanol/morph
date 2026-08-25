if(WIN32)
    if(TARGET Qt6::windeployqt)
        get_target_property(WINDEPLOYQT_EXECUTABLE Qt6::windeployqt IMPORTED_LOCATION)
    endif()
    if(NOT WINDEPLOYQT_EXECUTABLE)
        get_filename_component(_qt_core_dir "${Qt6Core_DIR}/../../.." ABSOLUTE)
        find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6
            HINTS
            "${_qt_core_dir}/bin"
            "${QT_DIR}/bin"
            "C:/Qt/6.8.0/msvc2022_64/bin"
        )
    endif()

    find_program(INNOSETUP_COMPILER NAMES ISCC iscc
        HINTS
        "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
        "$ENV{ProgramFiles}/Inno Setup 6"
        "$ENV{ProgramFiles\(x86\)}/Inno Setup 6"
        "C:/Program Files/Inno Setup 6"
        "C:/Program Files (x86)/Inno Setup 6"
    )

    if(DEFINED OPENSSL_ROOT_DIR)
        set(_openssl_root "${OPENSSL_ROOT_DIR}")
    elseif(DEFINED OpenSSL_DIR)
        get_filename_component(_openssl_root "${OpenSSL_DIR}/../../.." ABSOLUTE)
    elseif(DEFINED OPENSSL_INCLUDE_DIR)
        get_filename_component(_openssl_root "${OPENSSL_INCLUDE_DIR}/.." ABSOLUTE)
    else()
        set(_openssl_root "C:/Qt/Tools/OpenSSLv3/Win_x64")
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "windeployqt not found! Qt runtime deployment will be skipped.")
    endif()

    if(INNOSETUP_COMPILER)
        message(STATUS "Found Inno Setup Compiler (ISCC): ${INNOSETUP_COMPILER}")
    else()
        message(STATUS "Inno Setup Compiler (ISCC) not found. To build Windows setup installer, install Inno Setup via 'winget install JRSoftware.InnoSetup'.")
    endif()

    set(_deploy_commands "")
    if(WINDEPLOYQT_EXECUTABLE)
        set(_deploy_commands
            COMMAND ${CMAKE_COMMAND} -E echo "==> Running windeployqt..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE} "$<TARGET_FILE:morph>"
                    --qmldir "${CMAKE_SOURCE_DIR}/ui"
                    --openssl-root "${_openssl_root}"
                    --verbose 0
        )
    endif()

    set(_installer_commands "")
    if(INNOSETUP_COMPILER)
        set(_installer_commands
            COMMAND ${CMAKE_COMMAND} -E echo "==> Building Inno Setup Windows installer..."
            COMMAND ${INNOSETUP_COMPILER}
                    "/DMyAppVersion=${PROJECT_VERSION}"
                    "/DSourceDir=$<TARGET_FILE_DIR:morph>"
                    "/DOutputDir=${CMAKE_BINARY_DIR}"
                    "/DAssetsDir=${CMAKE_SOURCE_DIR}/assets"
                    "/DLicenseFile=${CMAKE_SOURCE_DIR}/LICENSE"
                    "${CMAKE_SOURCE_DIR}/installer/morph.iss"
            COMMAND ${CMAKE_COMMAND} -E echo "==> Setup installer created at ${CMAKE_BINARY_DIR}/morph-v${PROJECT_VERSION}-setup.exe"
        )
    endif()

    if(_deploy_commands OR _installer_commands)
        add_custom_command(TARGET morph POST_BUILD
            ${_deploy_commands}
            ${_installer_commands}
            COMMENT "Deploying Qt dependencies and generating Windows installer"
        )
    endif()

    add_custom_target(installer
        DEPENDS morph
        COMMENT "Windows installer target"
    )

    set(CPACK_GENERATOR "INNOSETUP;ZIP")
    set(CPACK_INNOSETUP_USE_CMAKE_BOOL_FORMAT ON)
    set(CPACK_PACKAGE_NAME "morph")
    set(CPACK_PACKAGE_VENDOR "lizapropanol")
    set(CPACK_PACKAGE_CONTACT "https://github.com/lizapropanol/morph")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern music player for Yandex Music and SoundCloud")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/assets/morph.ico")
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS morph DESTINATION bin)
    install(FILES morph.desktop DESTINATION share/applications)
    install(FILES assets/logo.svg DESTINATION share/icons/hicolor/scalable/apps RENAME morph.svg)
    install(FILES assets/morph.png DESTINATION share/icons/hicolor/128x128/apps)
    install(FILES assets/morph.png DESTINATION share/pixmaps)

    set(CPACK_PACKAGE_NAME "morph")
    set(CPACK_PACKAGE_VENDOR "lizapropanol")
    set(CPACK_PACKAGE_CONTACT "https://github.com/lizapropanol/morph")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern music player for Yandex Music and SoundCloud")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "lizapropanol")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "qml6-module-qtquick, qml6-module-qtquick-layouts, qml6-module-qtquick-controls, qml6-module-qtmultimedia, qt6-multimedia-plugins, qml6-module-qt5compat-graphicaleffects")
    set(CPACK_GENERATOR "DEB")
endif()

include(CPack)
