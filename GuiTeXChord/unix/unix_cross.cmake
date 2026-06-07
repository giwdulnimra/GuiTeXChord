# ── unix/unix_cross.cmake ─────────────────────────────────────────────────────
#
# Cross-compilation: Linux host → Windows target
# Uses MinGW-w64 cross toolchain to produce a Windows PE binary from Linux.
#
# STATUS: Advanced / optional. Only needed if you want to build the Windows
# binary FROM the Linux laptop without touching the Windows machine.
# For most workflows, build Windows binaries natively on Windows.
#
# PREREQUISITES (Ubuntu/Debian):
#   sudo apt install mingw-w64
#   # Qt for Windows must be built from source for mingw32 or obtained via
#   # mxe (M cross environment): https://mxe.cc
#   # MXE provides a pre-built Qt6 for mingw:
#   #   make MXE_TARGETS=x86_64-w64-mingw32.static qtbase
#
# USAGE:
#   cmake -B build/cross_win \
#         -DCMAKE_TOOLCHAIN_FILE=unix/mingw-w64-toolchain.cmake \
#         -DFORCE_PLATFORM_FILE=unix/unix_cross.cmake \
#         -DMXE_ROOT=~/mxe
#   cmake --build build/cross_win
#
# OUTPUT: build/GuiTeXChord_<Version>_win_cross/
#
# NOTE: This is a best-effort setup. Qt6 cross-compilation via MXE is
# well-documented but time-consuming to set up the first time (~1-2h).
# See https://mxe.cc/#tutorial for the full guide.
# ─────────────────────────────────────────────────────────────────────────────

if(NOT DEFINED MXE_ROOT)
    set(MXE_ROOT "$ENV{HOME}/mxe" CACHE PATH "Path to MXE installation")
endif()

set(MXE_TARGET "x86_64-w64-mingw32.static")
set(MXE_BIN    "${MXE_ROOT}/usr/${MXE_TARGET}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}_${APPVERSION}_win_cross)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    ${CMAKE_SOURCE_DIR}/build/debug_app)

set(CMAKE_PREFIX_PATH "${MXE_ROOT}/usr/${MXE_TARGET}/qt6")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 QUIET   COMPONENTS Multimedia MultimediaWidgets)

if(Qt6Multimedia_FOUND)
    message(STATUS "[unix_cross] Multimedia found.")
else()
    message(STATUS "[unix_cross] Multimedia NOT found.")
    add_compile_definitions(TV_NO_MULTIMEDIA)
endif()

function(setup_qt_target target_name)
    set_target_properties(${target_name} PROPERTIES
        AUTOMOC ON AUTOUIC ON AUTORCC ON)
    target_link_libraries(${target_name} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    if(Qt6Multimedia_FOUND)
        target_link_libraries(${target_name} PRIVATE
            Qt6::Multimedia Qt6::MultimediaWidgets)
    endif()
    # Cross-compiled Windows binary: no console window in release
    set_target_properties(${target_name} PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)
endfunction()
