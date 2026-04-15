#!/usr/bin/env bash
# build_ndk.sh — Build the Waydroid XR meta compat layer for Android arm64
#
# Requirements:
#   - Android NDK r25+ (set NDK_PATH or ANDROID_NDK_ROOT)
#   - cmake 3.22+
#   - ninja
#
# Usage:
#   NDK_PATH=/path/to/ndk ./scripts/build_ndk.sh
#   NDK_PATH=/path/to/ndk ./scripts/build_ndk.sh --install /path/to/waydroid/rootfs

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$ROOT/build/android-arm64"
INSTALL_DIR="${2:-$ROOT/dist/android-arm64}"

NDK="${NDK_PATH:-${ANDROID_NDK_ROOT:-}}"
if [ -z "$NDK" ]; then
    echo "ERROR: Set NDK_PATH or ANDROID_NDK_ROOT to your Android NDK installation"
    exit 1
fi

TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
if [ ! -f "$TOOLCHAIN" ]; then
    echo "ERROR: Cannot find NDK toolchain at $TOOLCHAIN"
    exit 1
fi

echo "==> Building waydroid-xr-meta-compat (arm64-v8a, Android API 32)"
echo "    NDK:     $NDK"
echo "    Build:   $BUILD_DIR"
echo "    Install: $INSTALL_DIR"
echo ""

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$ROOT" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-32 \
    -DANDROID_STL=c++_static \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"

ninja -j"$(nproc)"
ninja install

echo ""
echo "==> Build complete."
echo ""
echo "To inject into a running Waydroid system image:"
echo ""
echo "  # Stop Waydroid"
echo "  waydroid session stop"
echo ""
echo "  # Mount the Waydroid system image (adjust path as needed)"
echo "  WAYDROID_IMG=\$(waydroid prop get waydroid.system_ota)"
echo "  sudo mount -o rw,remount /var/lib/waydroid/images/system.img /opt/waydroid"
echo ""
echo "  # Install the layer"
echo "  sudo cp $INSTALL_DIR/vendor/lib64/openxr/libopenxr_waydroid_meta_compat.so \\"
echo "      /opt/waydroid/vendor/lib64/openxr/"
echo "  sudo mkdir -p /opt/waydroid/vendor/etc/openxr/1/api_layers/implicit.d"
echo "  sudo cp $INSTALL_DIR/vendor/etc/openxr/1/api_layers/implicit.d/*.json \\"
echo "      /opt/waydroid/vendor/etc/openxr/1/api_layers/implicit.d/"
echo ""
echo "  sudo umount /opt/waydroid"
echo ""
echo "  # Also start the host IPC server (separate project - waydroid-xr-server)"
echo "  waydroid-xr-server --runtime monado &"
echo ""
echo "  # Start Waydroid"
echo "  waydroid session start"
