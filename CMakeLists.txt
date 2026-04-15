cmake_minimum_required(VERSION 3.22)
project(waydroid_xr_meta_compat C)

# ============================================================================
# Dependencies
# ============================================================================

# OpenXR SDK (Khronos) — fetch from GitHub or point to local checkout
include(FetchContent)
FetchContent_Declare(
    openxr
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG        release-1.1.38   # pin to a known release
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(openxr)

# ============================================================================
# waydroid_xr_meta_compat shared library
# This is the OpenXR API layer .so that gets installed into the Waydroid image
# ============================================================================

add_library(openxr_waydroid_meta_compat SHARED
    src/layer_main.c
    src/meta_ext_stubs.c
    src/ipc_bridge.c
)

target_include_directories(openxr_waydroid_meta_compat PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${openxr_SOURCE_DIR}/include
)

target_link_libraries(openxr_waydroid_meta_compat PRIVATE
    log          # Android __android_log_print
    openxr_loader
)

target_compile_options(openxr_waydroid_meta_compat PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden   # don't export internal symbols
    -O2
)

# Export only the negotiation entry point
target_link_options(openxr_waydroid_meta_compat PRIVATE
    -Wl,--version-script=${CMAKE_SOURCE_DIR}/android/layer_exports.map
)

# ============================================================================
# Install targets for Waydroid system image injection
# ============================================================================

install(TARGETS openxr_waydroid_meta_compat
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/vendor/lib64/openxr
)

install(FILES android/openxr_layer_manifest.json
    DESTINATION ${CMAKE_INSTALL_PREFIX}/vendor/etc/openxr/1/api_layers/implicit.d
    RENAME XR_APILAYER_waydroid_meta_compat.json
)

# ============================================================================
# Android NDK build wrapper (alternative to CMake — see scripts/build_ndk.sh)
# ============================================================================
