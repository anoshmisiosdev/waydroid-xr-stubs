/*
 * dma_buf_import.h — DMA-BUF Buffer Import for GPU Rendering
 *
 * Handles the import of DMA-BUF file descriptors from the container into
 * GPU driver memory objects that can be rendered with OpenGL/Vulkan.
 *
 * The typical flow:
 *   1. Container sends xrEndFrame with eye buffers
 *   2. Host receives DMA-BUF file descriptors via socket SCM_RIGHTS
 *   3. dmabuf_import() wraps the FD in a GPU texture/buffer object
 *   4. GPU can render on top or composite the layer
 *   5. dmabuf_release() frees the GPU resource
 *
 * For SteamVR/OpenXR, we use EGL_EXT_image_dma_buf_import and Vulkan
 * VK_EXT_external_memory_dma_buf as fallbacks.
 *
 * License: Apache 2.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Buffer metadata and import context
 * ========================================================================= */

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;        /* VkFormat or GL format enum */
    uint32_t stride;        /* bytes per row */
    int      fourcc;        /* DRM fourcc code (e.g., DRM_FORMAT_ABGR8888) */
    int      modifier;      /* DRM format modifier (or ~0ULL if unused) */
} DmaBufMeta;

typedef struct DmaBufBuffer DmaBufBuffer;

/* =========================================================================
 * Import and release
 * ========================================================================= */

/*
 * dmabuf_import_gl — Import a DMA-BUF FD as an OpenGL texture.
 *
 * Parameters:
 *   fd:  DMA-BUF file descriptor (ownership transferred; will be closed)
 *   meta: Buffer metadata (width, height, format, etc.)
 *   out_handle: Receives opaque GPU texture handle
 *
 * Returns true on success. On success, the caller owns *out_handle and must
 * eventually call dmabuf_release_gl() to free it.
 */
bool dmabuf_import_gl(int fd, const DmaBufMeta *meta, uint32_t *out_handle);

/*
 * dmabuf_release_gl — Free an OpenGL texture imported from DMA-BUF.
 */
void dmabuf_release_gl(uint32_t handle);

/*
 * dmabuf_import_vulkan — Import a DMA-BUF FD as a Vulkan image.
 *
 * Returns opaque DmaBufBuffer handle on success, NULL on failure.
 * The caller owns the result and must call dmabuf_release_vulkan() to free it.
 */
DmaBufBuffer *dmabuf_import_vulkan(int fd, const DmaBufMeta *meta);

/*
 * dmabuf_release_vulkan — Free a Vulkan image imported from DMA-BUF.
 */
void dmabuf_release_vulkan(DmaBufBuffer *buf);

/* =========================================================================
 * GPU driver detection and initialization
 * ========================================================================= */

typedef enum {
    GPU_DRIVER_UNKNOWN,
    GPU_DRIVER_NVIDIA,    /* NVIDIA proprietary driver */
    GPU_DRIVER_AMD,       /* AMDGPU open-source driver */
    GPU_DRIVER_INTEL,     /* Intel i915/iris open-source driver */
    GPU_DRIVER_NOUVEAU,   /* NVIDIA nouveau open-source driver */
} GpuDriver;

/*
 * gpu_detect_driver — Detect the active GPU driver.
 */
GpuDriver gpu_detect_driver(void);

/*
 * dmabuf_init — Initialize GPU import layer for the detected driver.
 * Returns true if a suitable backend is available.
 */
bool dmabuf_init(void);

/*
 * dmabuf_shutdown — Clean up GPU resources.
 */
void dmabuf_shutdown(void);

#ifdef __cplusplus
}
#endif
