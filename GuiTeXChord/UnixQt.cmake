# ── UnixQt.cmake ──────────────────────────────────────────────────────────────
#
# Linux-Äquivalent zu SetupQt.cmake für native Linux-Builds.
# Ersetzt SetupQt.cmake vollständig — CMakeLists.txt bleibt identisch,
# nur der Dateiname ändert sich.
#
# EINBINDUNG in CMakeLists.txt:
#   Aktuelle Zeile:
#       include("${CMAKE_SOURCE_DIR}/SetupQt.cmake")
#   Ersetzen durch:
#       if(WIN32)
#           include("${CMAKE_SOURCE_DIR}/SetupQt.cmake")
#       else()
#           include("${CMAKE_SOURCE_DIR}/UnixQt.cmake")
#       endif()
#
# ÄNDERUNGEN gegenüber SetupQt.cmake:
#   - CMAKE_PREFIX_PATH entfällt (Qt kommt vom Paketmanager, nicht aus C:/Qt/...)
#   - Kein windeployqt6 / WIN32_EXECUTABLE
#   - Kein CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG/_RELEASE (optional, s.u.)
#   - find_package-Aufrufe identisch → selbe Qt-Komponenten
#   - setup_qt_target() hat dieselbe Signatur → CMakeLists.txt unverändert
#
# VORAUSSETZUNGEN (Ubuntu/Debian):
#   sudo apt install qt6-base-dev qt6-multimedia-dev cmake ninja-build
#   Optional für Release-Packaging: linuxdeployqt oder appimage-builder
#
# BUILD:
#   cmake -B build/linux -G Ninja   (oder "Unix Makefiles")
#   cmake --build build/linux
# ──────────────────────────────────────────────────────────────────────────────

# Ausgabeverzeichnisse — auf Linux konventionell flach, analog zu SetupQt.cmake
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
        ${CMAKE_SOURCE_DIR}/build/debug_app)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
        ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}_${APPVERSION})

# Qt via Paketmanager — kein PREFIX_PATH nötig.
# Falls Qt nicht im Systempfad liegt (z.B. manuelles Qt-Install unter ~/Qt):
#   cmake ... -DCMAKE_PREFIX_PATH=~/Qt/6.x.x/gcc_64
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 QUIET   COMPONENTS Multimedia MultimediaWidgets)

if(Qt6Multimedia_FOUND)
    message(STATUS "[UnixQt] Multimedia found – native video playback enabled.")
else()
    message(STATUS "[UnixQt] Multimedia NOT found – ffmpeg frame-cycling fallback only.")
    add_compile_definitions(TV_NO_MULTIMEDIA)
endif()

function(setup_qt_target target_name)
    set_target_properties(${target_name} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON)

    target_link_libraries(${target_name} PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets)

    if(Qt6Multimedia_FOUND)
        target_link_libraries(${target_name} PRIVATE
            Qt6::Multimedia Qt6::MultimediaWidgets)
    endif()

    # Linux Release: RPATH so setzen dass Qt-Libs neben dem Binary gefunden
    # werden, falls man sie später bündelt (z.B. mit linuxdeployqt)
    set_target_properties(${target_name} PROPERTIES
        INSTALL_RPATH "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH FALSE)
endfunction()
