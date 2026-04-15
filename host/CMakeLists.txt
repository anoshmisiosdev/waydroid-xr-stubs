cmake_minimum_required(VERSION 3.22)
project(waydroid_xr_server C)

# ============================================================================
# Dependencies
# ============================================================================

# OpenXR SDK (Khronos) — fetch from GitHub or point to local checkout
include(FetchContent)
FetchContent_Declare(
    openxr
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG        release-1.1.38
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(openxr)

# ============================================================================
# waydroid_xr_server executable
# ============================================================================

add_executable(waydroid_xr_server
    src/waydroid_xr_server.c
    src/ipc_server.c
    src/openxr_host.c
    src/dma_buf_import.c
)

target_include_directories(waydroid_xr_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${openxr_SOURCE_DIR}/include
)

target_link_libraries(waydroid_xr_server PRIVATE
    openxr_loader
    pthread
)

target_compile_options(waydroid_xr_server PRIVATE
    -Wall
    -Wextra
    -O2
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS waydroid_xr_server
    RUNTIME DESTINATION bin
)

# ============================================================================
# Optional: Development utilities
# ============================================================================

# Uncomment to build debug tools later:
# add_executable(test_ipc_client ...)
