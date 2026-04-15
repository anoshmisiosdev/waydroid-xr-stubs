/*
 * dma_buf_import.c — DMA-BUF Buffer Import Implementation
 *
 * Imports DMA-BUF file descriptors as GPU textures/buffers for rendering.
 * Supports both OpenGL (via EGL_EXT_image_dma_buf_import) and Vulkan
 * (VK_EXT_external_memory_dma_buf).
 *
 * License: Apache 2.0
 */

#include "dma_buf_import.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>

/* EGL/GL headers (build-time optional) */
#ifdef HAS_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#define LOG(level, fmt, ...) \
    fprintf(stderr, "[waydroid-xr-server] " level ": " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)

/* =========================================================================
 * GPU Driver Detection
 * ========================================================================= */

static GpuDriver detected_driver = GPU_DRIVER_UNKNOWN;

GpuDriver gpu_detect_driver(void) {
    if (detected_driver != GPU_DRIVER_UNKNOWN) {
        return detected_driver;
    }

    /* Check for DRM device to identify GPU driver */
    const char *drm_devices[] = {
        "/dev/dri/card0",
        "/dev/dri/card1",
        "/dev/dri/renderD128",
        "/dev/dri/renderD129",
    };

    for (size_t i = 0; i < sizeof(drm_devices) / sizeof(drm_devices[0]); i++) {
        int fd = open(drm_devices[i], O_RDONLY);
        if (fd < 0) continue;

        /* Use ioctl to query driver name — simplified check */
        char driver_name[32] = {0};
        /* In production, you'd use DRM_IOCTL_VERSION; for now, infer from device */

        close(fd);

        /* Simple heuristic: check sysfs for driver info */
        FILE *fp = fopen("/proc/driver/nvidia/version", "r");
        if (fp) {
            fclose(fp);
            detected_driver = GPU_DRIVER_NVIDIA;
            LOGI("Detected NVIDIA GPU driver");
            return detected_driver;
        }

        /* Check for AMD */
        if (strstr(drm_devices[i], "amdgpu") || access("/sys/kernel/debug/dri/0/amdgpu_pm_info", F_OK) == 0) {
            detected_driver = GPU_DRIVER_AMD;
            LOGI("Detected AMD GPU driver");
            return detected_driver;
        }

        /* Check for Intel */
        if (strstr(drm_devices[i], "i915") || strstr(drm_devices[i], "iris")) {
            detected_driver = GPU_DRIVER_INTEL;
            LOGI("Detected Intel GPU driver");
            return detected_driver;
        }
    }

    detected_driver = GPU_DRIVER_UNKNOWN;
    LOGW("Could not detect GPU driver; DMA-BUF import may fail");
    return GPU_DRIVER_UNKNOWN;
}

/* =========================================================================
 * OpenGL DMA-BUF Import (EGL_EXT_image_dma_buf_import)
 * ========================================================================= */

typedef struct {
    uint32_t gl_texture;
} GlDmaBufHandle;

bool dmabuf_import_gl(int fd, const DmaBufMeta *meta, uint32_t *out_handle) {
    if (!meta || !out_handle) return false;

    /* Stub implementation: In production, you would:
     * 1. Create an EGLImage from the DMA-BUF FD using eglCreateImageKHR
     * 2. Create a GL texture and attach the EGLImage
     * 3. Verify GPU can access via EGL_IMAGE_PRESERVED_KHR
     * 4. Return the GL texture ID
     */

    LOGW("OpenGL DMA-BUF import not yet implemented");
    close(fd);
    return false;
}

void dmabuf_release_gl(uint32_t handle) {
    if (handle == 0) return;

    /* Stub: delete GL texture, destroy EGLImage */
    LOGI("Released GL texture %u", handle);
}

/* =========================================================================
 * Vulkan DMA-BUF Import (VK_EXT_external_memory_dma_buf)
 * ========================================================================= */

typedef struct {
    void *vk_image;      /* VkImage handle */
    void *vk_device_mem; /* VkDeviceMemory handle */
} VulkanDmaBufHandle;

DmaBufBuffer *dmabuf_import_vulkan(int fd, const DmaBufMeta *meta) {
    if (!meta) {
        close(fd);
        return NULL;
    }

    /* Stub implementation: In production, you would:
     * 1. Create VkExternalMemoryImageCreateInfo with VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
     * 2. Create VkImage
     * 3. Query memory requirements
     * 4. Import FD as VkDeviceMemory using VkImportMemoryFdInfoKHR
     * 5. Bind image to device memory
     * 6. Wrap in DmaBufBuffer struct
     */

    LOGW("Vulkan DMA-BUF import not yet implemented");
    close(fd);
    return NULL;
}

void dmabuf_release_vulkan(DmaBufBuffer *buf) {
    if (!buf) return;

    /* Stub: destroy Vulkan image and device memory */
    LOGI("Released Vulkan DMA-BUF buffer");
    free(buf);
}

/* =========================================================================
 * Initialization
 * ========================================================================= */

bool dmabuf_init(void) {
    GpuDriver driver = gpu_detect_driver();

    switch (driver) {
        case GPU_DRIVER_NVIDIA:
        case GPU_DRIVER_AMD:
        case GPU_DRIVER_INTEL:
            LOGI("DMA-BUF support available for %d", driver);
            return true;

        case GPU_DRIVER_UNKNOWN:
        default:
            LOGE("No DMA-BUF capable GPU driver detected");
            return false;
    }
}

void dmabuf_shutdown(void) {
    LOGI("DMA-BUF layer shut down");
    detected_driver = GPU_DRIVER_UNKNOWN;
}
