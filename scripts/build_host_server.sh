#!/bin/bash
#
# build_host_server.sh — Build the Waydroid XR host server
#
# This script compiles waydroid-xr-server which runs on the host Linux system
# to receive frames from the Waydroid container and composite them to the PCVR runtime.
#
# usage: ./scripts/build_host_server.sh [--install] [--clean]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build-host}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
DO_INSTALL=0
DO_CLEAN=0

# Parse arguments
for arg in "$@"; do
    case "$arg" in
        --install)
            DO_INSTALL=1
            ;;
        --clean)
            DO_CLEAN=1
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --install         Install after build"
            echo "  --clean          Clean build directory first"
            echo "  --help           Show this help message"
            echo ""
            echo "Environment variables:"
            echo "  BUILD_DIR         Build directory (default: build-host/)"
            echo "  INSTALL_PREFIX    Install path (default: /usr/local)"
            exit 0
            ;;
    esac
done

# Clean if requested
if [ $DO_CLEAN -eq 1 ]; then
    echo "[*] Cleaning build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
echo "[*] Configuring CMake..."
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DCMAKE_BUILD_HOST_SERVER=ON \
    "$PROJECT_ROOT"

# Build
echo "[*] Building waydroid-xr-server..."
cmake --build . --config Release -j$(nproc)

# Install if requested
if [ $DO_INSTALL -eq 1 ]; then
    echo "[*] Installing to $INSTALL_PREFIX..."
    cmake --install .
    echo "[+] Installation complete!"
    echo ""
    echo "You can now run:"
    echo "  $INSTALL_PREFIX/bin/waydroid-xr-server"
fi

echo "[+] Build complete!"
echo ""
echo "Output: $BUILD_DIR"
echo "Executable: $BUILD_DIR/waydroid_xr_server"
echo ""
echo "To run the server:"
echo "  $BUILD_DIR/waydroid_xr_server"
echo ""
echo "To install system-wide:"
echo "  sudo cmake --install $BUILD_DIR --prefix /usr/local"
