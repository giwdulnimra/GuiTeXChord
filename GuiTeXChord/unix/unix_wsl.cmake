# ── unix/unix_wsl.cmake ───────────────────────────────────────────────────────
#
# WSL2 build variant.
#
# Background: WSL2 runs a real Linux kernel — gcc inside WSL2 is a NATIVE
# Linux compiler, not a cross-compiler.  The resulting binary is a normal
# Linux ELF, not a Windows PE.  There is no "cross-compilation" in the
# traditional sense; WSL2 is simply a second build host living inside Windows.
#
# This file exists as a named variant so build.sh can invoke it explicitly
# (e.g. when Qt lives in a non-standard WSL path) and to document the
# intent clearly in version control.
#
# If your WSL2 Qt installation is the same as the native Linux one,
# unix_wsl.cmake and unix_native.cmake are interchangeable.
#
# To use a custom Qt path inside WSL2:
#   cmake -B build/linux -DCMAKE_PREFIX_PATH=~/Qt/6.x.x/gcc_64 \
#         -DPLATFORM_FILE=unix/unix_wsl.cmake
# ─────────────────────────────────────────────────────────────────────────────

# Re-uses the same Qt setup; only the output directory differs.
# The output dir is set by UnixQt.cmake but can be overridden here:
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
#     ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}_${APPVERSION}_unix_wsl)

include("${CMAKE_SOURCE_DIR}/unix/UnixQt.cmake")

# Placeholder for WSL2-specific steps (e.g. different Qt prefix).
