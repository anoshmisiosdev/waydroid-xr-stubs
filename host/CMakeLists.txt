cmake_minimum_required(VERSION 3.22)
project(waydroid_xr_server C)

# ============================================================================
# Dependencies
# ============================================================================

# OpenXR SDK (Khronos) — fetch from GitHub or point to local checkout
include(FetchContent)

# Disable OpenXR's own tests/docs to avoid build issues
set(BUILD_TESTS OFF CACHE BOOL "Disable OpenXR tests" FORCE)
set(BUILD_DOCS OFF CACHE BOOL "Disable OpenXR docs" FORCE)

FetchContent_Declare(
    openxr
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG        release-1.1.38
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(openxr)

# Optional: EGL/OpenGL support for GPU buffer import
find_package(OpenGL QUIET)
find_package(EGL QUIET)

# ============================================================================
# Compiler flags
# ============================================================================

option(ENABLE_EGL_SUPPORT "Enable EGL/OpenGL DMA-BUF import" ON)
if(ENABLE_EGL_SUPPORT AND EGL_FOUND AND OPENGL_FOUND)
    add_definitions(-DHAS_EGL)
    message(STATUS "EGL/OpenGL support enabled")
endif()

# ============================================================================
# waydroid_xr_server executable
# ============================================================================

add_executable(waydroid_xr_server
    src/waydroid_xr_server.c
    src/ipc_server.c
    src/openxr_host.c
    src/dma_buf_import.c
    src/gpu_sync.c
    src/graphics_binding.c
)

target_include_directories(waydroid_xr_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${openxr_SOURCE_DIR}/include
)

target_link_libraries(waydroid_xr_server PRIVATE
    openxr_loader
    pthread
)

# Link EGL and OpenGL if available
if(ENABLE_EGL_SUPPORT AND EGL_FOUND AND OPENGL_FOUND)
    target_link_libraries(waydroid_xr_server PRIVATE
        ${EGL_LIBRARIES}
        ${OPENGL_LIBRARIES}
    )
    target_include_directories(waydroid_xr_server PRIVATE
        ${EGL_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
    )
endif()

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
# Optional: Testing utilities
# ============================================================================

option(BUILD_GPU_TESTS "Build GPU integration tests" ON)

if(BUILD_GPU_TESTS)
    enable_testing()

    add_executable(test_gpu_integration
        test/test_gpu_integration.c
        src/dma_buf_import.c
        src/gpu_sync.c
        src/graphics_binding.c
    )

    target_include_directories(test_gpu_integration PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${openxr_SOURCE_DIR}/include
    )

    target_link_libraries(test_gpu_integration PRIVATE
        pthread
    )

    if(ENABLE_EGL_SUPPORT AND EGL_FOUND AND OPENGL_FOUND)
        target_link_libraries(test_gpu_integration PRIVATE
            ${EGL_LIBRARIES}
            ${OPENGL_LIBRARIES}
        )
        target_include_directories(test_gpu_integration PRIVATE
            ${EGL_INCLUDE_DIRS}
            ${OPENGL_INCLUDE_DIRS}
        )
    endif()

    target_compile_options(test_gpu_integration PRIVATE
        -Wall
        -Wextra
        -O2
    )

    add_test(
        NAME gpu_integration_tests
        COMMAND test_gpu_integration
    )

    message(STATUS "GPU integration tests enabled")
endif()

# ============================================================================
# Optional: Development utilities
# ============================================================================

# Uncomment to build debug tools later:
# add_executable(test_ipc_client ...)
