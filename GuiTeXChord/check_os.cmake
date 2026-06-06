# ── check_os.cmake ────────────────────────────────────────────────────────────
#
# Detects the build platform and delegates to win32/ or unix/.
# Included by CMakeLists.txt AFTER add_executable().
#
# Optional override (useful for WSL2 builds from build.sh):
#   cmake -DFORCE_PLATFORM_FILE=unix/unix_wsl.cmake ...
#   → skips auto-detection and loads the specified file directly.
#
# After this include the following are guaranteed:
#   setup_qt_target(<target>)                        – defined by platform cmake
#   CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG / _RELEASE  – set by platform cmake
# ─────────────────────────────────────────────────────────────────────────────

if(DEFINED FORCE_PLATFORM_FILE AND NOT FORCE_PLATFORM_FILE STREQUAL "")
    message(STATUS "[check_os] Override active → ${FORCE_PLATFORM_FILE}")
    include("${CMAKE_SOURCE_DIR}/${FORCE_PLATFORM_FILE}")
elseif(WIN32)
    message(STATUS "[check_os] Platform: Windows  →  win32/win32.cmake")
    include("${CMAKE_SOURCE_DIR}/win32/win32.cmake")
elseif(UNIX)
    message(STATUS "[check_os] Platform: Unix/Linux  →  unix/unix_native.cmake")
    include("${CMAKE_SOURCE_DIR}/unix/unix_native.cmake")
else()
    message(FATAL_ERROR "[check_os] Unsupported platform. "
            "Add a branch in check_os.cmake and a matching platform directory.")
endif()
