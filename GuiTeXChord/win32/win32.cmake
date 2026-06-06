# ── win32/win32.cmake ─────────────────────────────────────────────────────────
#
# Windows build orchestration.
# Included by check_os.cmake when WIN32 is set.
#
# Responsibilities:
#   1. Set output directories
#   2. Include Qt setup  (SetupQt.cmake)
#   3. Include any other Windows-specific tooling (signing, NSIS, …)
#
# setup_qt_target() is defined by SetupQt.cmake and called by CMakeLists.txt.
# ─────────────────────────────────────────────────────────────────────────────

include("${CMAKE_SOURCE_DIR}/win32/SetupQt.cmake")

# Placeholder: add Windows-specific steps here if needed in the future.
# Examples:
#   include("${CMAKE_SOURCE_DIR}/win32/codesign.cmake")
#   include("${CMAKE_SOURCE_DIR}/win32/installer_nsis.cmake")
