# ── unix/UnixQt.cmake ─────────────────────────────────────────────────────────
#
# Linux Qt setup – functional equivalent of win32/SetupQt.cmake.
# Called via unix/unix_native.cmake (or unix_wsl.cmake) → check_os.cmake
#
# USAGE / INTEGRATION
# ───────────────────
# Nothing in CMakeLists.txt needs to change – check_os.cmake handles the
# platform switch transparently.
#
# If Qt is NOT in the system path (manual install under ~/Qt):
#   cmake -B build/linux -DCMAKE_PREFIX_PATH=~/Qt/6.x.x/gcc_64
#
# PREREQUISITES (Ubuntu/Debian)
# ──────────────────────────────
# Run unix/config.sh once to install all dependencies, then unix/build.sh
# to configure and build.
#
# PACKAGING
# ─────────
# See unix/build.sh for AppImage generation via appimage-builder.
# ─────────────────────────────────────────────────────────────────────────────

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
        ${CMAKE_SOURCE_DIR}/build/debug_app)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
        ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}_${APPVERSION}_unix)

# Qt via system package manager – no PREFIX_PATH required.
# Override on the cmake command line if needed (see above).
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 QUIET   COMPONENTS Multimedia MultimediaWidgets)

if(Qt6Multimedia_FOUND)
    message(STATUS "[UnixQt] Multimedia found – native video playback enabled.")
else()
    message(STATUS "[UnixQt] Multimedia NOT found.")
    add_compile_definitions(TV_NO_MULTIMEDIA)
endif()

function(setup_qt_target target_name)
    set_target_properties(${target_name} PROPERTIES
        AUTOMOC ON AUTOUIC ON AUTORCC ON)

    target_link_libraries(${target_name} PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets)

    if(Qt6Multimedia_FOUND)
        target_link_libraries(${target_name} PRIVATE
            Qt6::Multimedia Qt6::MultimediaWidgets)
    endif()

    # RPATH: look for bundled libs in ./lib next to the binary.
    # Needed if you ship Qt libs alongside the AppImage binary.
    set_target_properties(${target_name} PROPERTIES
        INSTALL_RPATH        "$ORIGIN/lib"
        BUILD_WITH_INSTALL_RPATH FALSE)
endfunction()
