cmake_minimum_required(VERSION 3.16)
project(morph VERSION 0.2.4 LANGUAGES CXX)

if(UNIX AND NOT APPLE AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/build_number.txt")
    file(READ "${CMAKE_SOURCE_DIR}/build_number.txt" BUILD_NUMBER)
    string(STRIP "${BUILD_NUMBER}" BUILD_NUMBER)
    math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")
else()
    set(BUILD_NUMBER "1")
endif()
file(WRITE "${CMAKE_SOURCE_DIR}/build_number.txt" "${BUILD_NUMBER}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Quick Qml Network Multimedia DBus Widgets Svg Core5Compat REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(morph
    resources.qrc
    src/main.cpp
    src/core/Application.h src/core/Application.cpp
    src/utils/PathProvider.h src/utils/PathProvider.cpp
    src/utils/QmlHighlighter.h src/utils/QmlHighlighter.cpp
    src/utils/CacheManager.h src/utils/CacheManager.cpp
    src/utils/FileWatcher.h src/utils/FileWatcher.cpp
    src/utils/CryptoUtils.h src/utils/CryptoUtils.cpp
    src/network/NetworkManager.h src/network/NetworkManager.cpp
    src/services/BaseService.h
    src/services/YandexService.h src/services/YandexService.cpp
    src/services/SoundCloudService.h src/services/SoundCloudService.cpp
    src/services/YouTubeService.h src/services/YouTubeService.cpp
    src/audio/AudioEngine.h src/audio/AudioEngine.cpp
    src/core/ServiceManager.h src/core/ServiceManager.cpp
    src/settings/SettingsManager.h src/settings/SettingsManager.cpp
    src/settings/PlaylistManager.h src/settings/PlaylistManager.cpp
    src/settings/TokenManager.h src/settings/TokenManager.cpp
    src/settings/HistoryManager.h src/settings/HistoryManager.cpp
    src/settings/StyleManager.h src/settings/StyleManager.cpp
    src/core/MprisManager.h src/core/MprisManager.cpp
    src/core/MprisRootAdaptor.h src/core/MprisPlayerAdaptor.h src/core/MprisPlayerAdaptor.cpp
    src/utils/DiscordManager.h src/utils/DiscordManager.cpp
)

target_include_directories(morph PRIVATE src)
target_compile_definitions(morph PRIVATE 
    MORPH_BUILD_NUMBER="${BUILD_NUMBER}"
    MORPH_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(morph PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Multimedia
    Qt6::DBus
    Qt6::Widgets
    Qt6::Svg
    Qt6::Core5Compat
    OpenSSL::Crypto
)

include(Packaging.cmake)
