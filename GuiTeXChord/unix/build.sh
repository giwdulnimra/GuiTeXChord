#!/usr/bin/env bash
# ── unix/build.sh ─────────────────────────────────────────────────────────────
# Build script for GuiTeXChord on Linux.
#
# Usage (run from repo root):
#   ./unix/build.sh                        # Release, Linux binary
#   ./unix/build.sh --debug                # Debug, Linux binary
#   ./unix/build.sh --cross                # Release, Windows binary (cross)
#   ./unix/build.sh --appimage             # Release + AppImage
#   ./unix/build.sh --qt-prefix PATH       # Override Qt prefix path
#   ./unix/build.sh --mxe-root  PATH       # MXE root for cross-compile
#   ./unix/build.sh --jobs N               # Parallel jobs (default: nproc)
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

BUILD_TYPE="Release"
CROSS=0
MAKE_APPIMAGE=0
QT_PREFIX=""
MXE_ROOT="${HOME}/mxe"
JOBS=$(nproc 2>/dev/null || echo 4)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)      BUILD_TYPE="Debug" ;;
        --cross)      CROSS=1 ;;
        --appimage)   MAKE_APPIMAGE=1 ;;
        --qt-prefix)  QT_PREFIX="$2"; shift ;;
        --mxe-root)   MXE_ROOT="$2";  shift ;;
        --jobs)       JOBS="$2";       shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

VERSION=$(grep -oP '(?<=VERSION )\d+\.\d+\.\d+' "${REPO_ROOT}/CMakeLists.txt" | head -1)
MAJOR=${VERSION%%.*}; REST=${VERSION#*.}; MINOR=${REST%%.*}; PATCH=${REST##*.}
PAD(){ printf "%02d" "$1"; }
APPVERSION="v${MAJOR}_$(PAD $MINOR)$(PAD $PATCH)"

if [[ $CROSS -eq 1 ]]; then
    BUILD_DIR="${REPO_ROOT}/build/GuiTeXChord_${APPVERSION}_win_cross"
    echo "==> Cross-compile: Linux → Windows"
    echo "    MXE root: ${MXE_ROOT}"
else
    if [[ "$BUILD_TYPE" == "Debug" ]]; then
        BUILD_DIR="${REPO_ROOT}/build/debug_app"
    else
        BUILD_DIR="${REPO_ROOT}/build/GuiTeXChord_${APPVERSION}_unix"
    fi
    echo "==> Linux native build"
fi
echo "    Version : ${VERSION} (${APPVERSION})"
echo "    Type    : ${BUILD_TYPE}"
echo "    Output  : ${BUILD_DIR}"

CMAKE_ARGS=(-B "${BUILD_DIR}" -S "${REPO_ROOT}" -G Ninja
            -DCMAKE_BUILD_TYPE="${BUILD_TYPE}")

if [[ $CROSS -eq 1 ]]; then
    CMAKE_ARGS+=(
        -DCMAKE_TOOLCHAIN_FILE="${REPO_ROOT}/unix/mingw-w64-toolchain.cmake"
        -DFORCE_PLATFORM_FILE="unix/unix_cross.cmake"
        -DMXE_ROOT="${MXE_ROOT}"
    )
fi
[[ -n "$QT_PREFIX" ]] && CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="${QT_PREFIX}")

cmake "${CMAKE_ARGS[@]}"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j "${JOBS}"
echo "✓ Done: ${BUILD_DIR}"

if [[ "$MAKE_APPIMAGE" -eq 1 && "$BUILD_TYPE" == "Release" && $CROSS -eq 0 ]]; then
    echo "==> Packaging AppImage…"
    cd "${BUILD_DIR}"
    appimage-builder --recipe "${SCRIPT_DIR}/AppImageBuilder.yml" --skip-test
    echo "✓ AppImage created in ${BUILD_DIR}"
fi
