cmake_minimum_required(VERSION 3.16)
project(morph VERSION 0.1.0 LANGUAGES CXX)

if(EXISTS "${CMAKE_SOURCE_DIR}/build_number.txt")
    file(READ "${CMAKE_SOURCE_DIR}/build_number.txt" BUILD_NUMBER)
    string(STRIP "${BUILD_NUMBER}" BUILD_NUMBER)
    math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")
else()
    set(BUILD_NUMBER "1")
endif()
file(WRITE "${CMAKE_SOURCE_DIR}/build_number.txt" "${BUILD_NUMBER}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Quick Qml Network Multimedia DBus Widgets REQUIRED)

add_executable(morph
    resources.qrc
    src/main.cpp
    src/core/Application.h src/core/Application.cpp
    src/utils/PathProvider.h src/utils/PathProvider.cpp
    src/utils/CacheManager.h src/utils/CacheManager.cpp
    src/utils/FileWatcher.h src/utils/FileWatcher.cpp
    src/utils/CryptoUtils.h src/utils/CryptoUtils.cpp
    src/network/NetworkManager.h src/network/NetworkManager.cpp
    src/services/BaseService.h
    src/services/YandexService.h src/services/YandexService.cpp
    src/services/SoundCloudService.h src/services/SoundCloudService.cpp
    src/audio/AudioEngine.h src/audio/AudioEngine.cpp
    src/core/ServiceManager.h src/core/ServiceManager.cpp
    src/settings/SettingsManager.h src/settings/SettingsManager.cpp
    src/core/MprisManager.h src/core/MprisManager.cpp
    src/utils/DiscordManager.h src/utils/DiscordManager.cpp
)

target_include_directories(morph PRIVATE src)
target_compile_definitions(morph PRIVATE 
    MORPH_BUILD_NUMBER="${BUILD_NUMBER}"
    MORPH_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(morph PRIVATE
    Qt5::Core
    Qt5::Quick
    Qt5::Qml
    Qt5::Network
    Qt5::Multimedia
    Qt5::DBus
    Qt5::Widgets
)
