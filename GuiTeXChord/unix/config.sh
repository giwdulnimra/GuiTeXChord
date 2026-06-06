#!/usr/bin/env bash
# ── unix/config.sh ────────────────────────────────────────────────────────────
# One-time setup: installs all build dependencies for ChordGen on Ubuntu/Debian.
# Run once after cloning the repository.
# Works on the Linux laptop and inside WSL2.
#
# Usage:
#   chmod +x unix/config.sh
#   ./unix/config.sh
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

echo "==> Updating package lists…"
sudo apt-get update

echo "==> Installing build tools…"
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pdflatex \
    texlive-latex-base \
    texlive-pictures        # for tikz

echo "==> Installing Qt6…"
sudo apt-get install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libqt6widgets6 \
    libqt6gui6 \
    libqt6core6

# Optional: Multimedia support
echo "==> Installing Qt6 Multimedia (optional)…"
sudo apt-get install -y qt6-multimedia-dev || \
    echo "  (Multimedia not available – skipping, TV_NO_MULTIMEDIA will be set)"

# appimage-builder for packaging
echo "==> Installing appimage-builder…"
sudo apt-get install -y python3-pip
pip3 install appimage-builder --break-system-packages || \
    pip3 install appimage-builder

echo ""
echo "✓ Setup complete. Run ./unix/build.sh to configure and build."
