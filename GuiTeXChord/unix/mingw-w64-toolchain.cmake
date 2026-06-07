# ── unix/mingw-w64-toolchain.cmake ───────────────────────────────────────────
# CMake toolchain file for cross-compiling to Windows from Linux using MinGW-w64
# Pass to cmake via: -DCMAKE_TOOLCHAIN_FILE=unix/mingw-w64-toolchain.cmake
# ─────────────────────────────────────────────────────────────────────────────
set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_VERSION 10)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CROSS_PREFIX x86_64-w64-mingw32)
find_program(CMAKE_C_COMPILER   ${CROSS_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER ${CROSS_PREFIX}-g++)
find_program(CMAKE_RC_COMPILER  ${CROSS_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
