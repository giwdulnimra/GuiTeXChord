# ── check_os.cmake ────────────────────────────────────────────────────────────
#
# Detects the build platform and delegates to win32/ or unix/.
# Included by CMakeLists.txt BEFORE add_executable().
#
# Build variants:
#   Windows (native)      → win32/win32.cmake       (auto-detected)
#   Linux   (native)      → unix/unix_native.cmake  (auto-detected)
#   Linux → Windows cross → unix/unix_cross.cmake
#     Usage:
#       cmake -B build/cross_win \
#             -DCMAKE_TOOLCHAIN_FILE=unix/mingw-w64-toolchain.cmake \
#             -DFORCE_PLATFORM_FILE=unix/unix_cross.cmake
#
# After this include, setup_qt_target(<target>) is defined.
# ─────────────────────────────────────────────────────────────────────────────

if(DEFINED FORCE_PLATFORM_FILE AND NOT FORCE_PLATFORM_FILE STREQUAL "")
    message(STATUS "[check_os] Override → ${FORCE_PLATFORM_FILE}")
    include("${CMAKE_SOURCE_DIR}/${FORCE_PLATFORM_FILE}")
elseif(WIN32)
    message(STATUS "[check_os] Windows → win32/win32.cmake")
    include("${CMAKE_SOURCE_DIR}/win32/win32.cmake")
elseif(UNIX)
    message(STATUS "[check_os] Linux → unix/unix_native.cmake")
    include("${CMAKE_SOURCE_DIR}/unix/unix_native.cmake")
else()
    message(FATAL_ERROR "[check_os] Unsupported platform.")
endif()
