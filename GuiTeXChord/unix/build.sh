#!/usr/bin/env bash
# ── unix/build.sh ─────────────────────────────────────────────────────────────
# Build script for ChordGen on Linux (native or WSL2).
#
# Usage:
#   ./unix/build.sh                   # Release build
#   ./unix/build.sh --debug           # Debug build
#   ./unix/build.sh --appimage        # Release + package as AppImage
#   ./unix/build.sh --wsl             # Force WSL2 cmake variant
#   ./unix/build.sh --qt-prefix PATH  # Override Qt prefix path
#   ./unix/build.sh --jobs N          # Parallel jobs (default: nproc)
#
# Run from the repository root:
#   cd ChordGen
#   ./unix/build.sh
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

# ── Defaults ──────────────────────────────────────────────────────────────────
BUILD_TYPE="Release"
MAKE_APPIMAGE=0
PLATFORM_FILE=""   # empty = check_os.cmake decides automatically
QT_PREFIX=""
JOBS=$(nproc 2>/dev/null || echo 4)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)      BUILD_TYPE="Debug" ;;
        --appimage)   MAKE_APPIMAGE=1 ;;
        --wsl)        PLATFORM_FILE="unix/unix_wsl.cmake" ;;
        --qt-prefix)  QT_PREFIX="$2"; shift ;;
        --jobs)       JOBS="$2"; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

# ── Derive build directory from CMakeLists version ───────────────────────────
# Read version from CMakeLists.txt (line: project(ChordGen VERSION x.y.z ...))
VERSION=$(grep -oP '(?<=VERSION )\d+\.\d+\.\d+' "${REPO_ROOT}/CMakeLists.txt" | head -1)
MAJOR=${VERSION%%.*}; REST=${VERSION#*.}; MINOR=${REST%%.*}; PATCH=${REST##*.}
PAD() { printf "%02d" "$1"; }
APPVERSION="v${MAJOR}_$(PAD $MINOR)$(PAD $PATCH)"

if [[ "$BUILD_TYPE" == "Debug" ]]; then
    BUILD_DIR="${REPO_ROOT}/build/debug_app"
else
    BUILD_DIR="${REPO_ROOT}/build/ChordGen_${APPVERSION}_unix"
fi

echo "==> ChordGen build"
echo "    Version   : ${VERSION} (${APPVERSION})"
echo "    Type      : ${BUILD_TYPE}"
echo "    Build dir : ${BUILD_DIR}"
echo "    Jobs      : ${JOBS}"

# ── CMake configure ───────────────────────────────────────────────────────────
CMAKE_ARGS=(
    -B "${BUILD_DIR}"
    -S "${REPO_ROOT}"
    -G Ninja
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
)

if [[ -n "$QT_PREFIX" ]]; then
    CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="${QT_PREFIX}")
fi

# If --wsl was passed, override check_os.cmake by injecting the WSL variant.
# We do this by temporarily symlinking/copying; cleanest way without patching
# check_os.cmake is to pass a cache variable that unix_wsl.cmake can read.
if [[ -n "$PLATFORM_FILE" ]]; then
    CMAKE_ARGS+=(-DFORCE_PLATFORM_FILE="${PLATFORM_FILE}")
fi

cmake "${CMAKE_ARGS[@]}"

# ── Build ─────────────────────────────────────────────────────────────────────
echo "==> Building…"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j "${JOBS}"

echo ""
echo "✓ Build complete: ${BUILD_DIR}"

# ── AppImage packaging ────────────────────────────────────────────────────────
if [[ "$MAKE_APPIMAGE" -eq 1 && "$BUILD_TYPE" == "Release" ]]; then
    echo ""
    echo "==> Packaging AppImage…"
    APPIMAGE_RECIPE="${SCRIPT_DIR}/AppImageBuilder.yml"
    if [[ ! -f "$APPIMAGE_RECIPE" ]]; then
        echo "✗ AppImageBuilder.yml not found in unix/. Run unix/build.sh without --appimage first."
        exit 1
    fi
    cd "${BUILD_DIR}"
    appimage-builder --recipe "${APPIMAGE_RECIPE}" --skip-test
    echo "✓ AppImage created in ${BUILD_DIR}"
fi
