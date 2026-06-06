# ── unix/unix_native.cmake ────────────────────────────────────────────────────
#
# Native Linux build.
# Used when building directly on the Linux laptop (or inside WSL2 for a
# Linux binary).
#
# Included by check_os.cmake when UNIX is set.
# Also included explicitly by unix/build.sh for WSL2 Linux builds.
#
# setup_qt_target() is defined by UnixQt.cmake and called by CMakeLists.txt.
# ─────────────────────────────────────────────────────────────────────────────

include("${CMAKE_SOURCE_DIR}/unix/UnixQt.cmake")

# Placeholder: add Linux-specific steps here if needed.
# Examples:
#   include("${CMAKE_SOURCE_DIR}/unix/appimage.cmake")
#   include("${CMAKE_SOURCE_DIR}/unix/install_rules.cmake")
