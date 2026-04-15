/*
 * graphics_binding.c — OpenXR Graphics Binding Implementation
 *
 * Detects graphics API support and creates XrGraphicsBinding structures
 * for OpenXR session creation.
 *
 * License: Apache 2.0
 */

#include "graphics_binding.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

/* Optional OpenXR graphics extensions */
#ifdef HAS_EGL
#include <openxr/openxr_platform.h>
#endif

#define LOG(level, fmt, ...) \
    fprintf(stderr, "[waydroid-xr-server] " level ": " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)

/* =========================================================================
 * Graphics Binding State
 * ========================================================================= */

static struct {
    GraphicsApi selected_api;
    uint32_t available_apis;
    bool initialized;

#ifdef HAS_EGL
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLConfig egl_config;
#endif
} g_graphics = {0};

/* =========================================================================
 * API Availability Detection
 * ========================================================================= */

uint32_t graphics_binding_detect_available(void) {
    if (g_graphics.available_apis != 0) {
        return g_graphics.available_apis;
    }

    uint32_t available = 0;

    /* Check for EGL/OpenGL support */
#ifdef HAS_EGL
    /* Try to open EGL library */
    void *egl_lib = dlopen("libEGL.so.1", RTLD_LAZY);
    if (!egl_lib) {
        egl_lib = dlopen("libEGL.so", RTLD_LAZY);
    }

    if (egl_lib) {
        LOGI("EGL library found");
        available |= (1 << GRAPHICS_API_OPENGL);
        dlclose(egl_lib);
    }
#endif

    /* Check for Vulkan support */
    void *vk_lib = dlopen("libvulkan.so.1", RTLD_LAZY);
    if (!vk_lib) {
        vk_lib = dlopen("libvulkan.so", RTLD_LAZY);
    }

    if (vk_lib) {
        LOGI("Vulkan library found");
        available |= (1 << GRAPHICS_API_VULKAN);
        dlclose(vk_lib);
    }

    if (available == 0) {
        LOGW("No graphics APIs available; rendering will be limited");
    }

    g_graphics.available_apis = available;
    return available;
}

/* =========================================================================
 * EGL/OpenGL Graphics Binding
 * ========================================================================= */

#ifdef HAS_EGL

#include <EGL/egl.h>

static bool egl_setup(void) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
        LOGE("eglInitialize failed: %d", eglGetError());
        return false;
    }

    LOGI("EGL initialized: version %d.%d", major, minor);

    /* Choose a configuration */
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };

    EGLint num_configs;
    EGLConfig config;
    if (!eglChooseConfig(display, attribs, &config, 1, &num_configs) || num_configs == 0) {
        LOGE("eglChooseConfig failed");
        eglTerminate(display);
        return false;
    }

    /* Create a context */
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed: %d", eglGetError());
        eglTerminate(display);
        return false;
    }

    g_graphics.selected_api = GRAPHICS_API_OPENGL;
    g_graphics.egl_display = display;
    g_graphics.egl_context = context;
    g_graphics.egl_config = config;

    LOGI("EGL/OpenGL graphics binding configured");
    return true;
}

void *graphics_binding_init_egl(void) {
    if (!(g_graphics.available_apis & (1 << GRAPHICS_API_OPENGL))) {
        LOGW("EGL/OpenGL not available");
        return NULL;
    }

    if (!egl_setup()) {
        return NULL;
    }

    /* In production, this would create and return an XrGraphicsBindingEGLKHR
     * or similar structure. For MVP, we just ensure EGL is initialized. */

    return (void *)&g_graphics;
}

#else

void *graphics_binding_init_egl(void) {
    LOGW("EGL/OpenGL support not compiled (HAS_EGL not defined)");
    return NULL;
}

#endif

/* =========================================================================
 * Vulkan Graphics Binding
 * ========================================================================= */

void *graphics_binding_init_vulkan(XrInstance instance, XrSystemId system_id) {
    if (!(g_graphics.available_apis & (1 << GRAPHICS_API_VULKAN))) {
        LOGW("Vulkan not available");
        return NULL;
    }

    /*
     * Full implementation would:
     *   1. Create VkInstance (or use existing)
     *   2. Query system graphics requirements via xrGetVulkanGraphicsRequirementsKHR
     *   3. Create VkPhysicalDevice matching XR requirements
     *   4. Create VkDevice with required features
     *   5. Create VkQueue for rendering
     *   6. Return XrGraphicsBindingVulkanKHR structure
     *
     * For MVP, log and return placeholder
     */

    LOGW("Vulkan graphics binding not yet fully implemented");

    g_graphics.selected_api = GRAPHICS_API_VULKAN;
    return (void *)&g_graphics;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

GraphicsApi graphics_binding_get_selected_api(void) {
    return g_graphics.selected_api;
}

void graphics_binding_shutdown(void) {
#ifdef HAS_EGL
    if (g_graphics.egl_context != EGL_NO_CONTEXT) {
        eglDestroyContext(g_graphics.egl_display, g_graphics.egl_context);
        g_graphics.egl_context = EGL_NO_CONTEXT;
    }
    if (g_graphics.egl_display != EGL_NO_DISPLAY) {
        eglTerminate(g_graphics.egl_display);
        g_graphics.egl_display = EGL_NO_DISPLAY;
    }
#endif

    g_graphics.selected_api = GRAPHICS_API_UNKNOWN;
    g_graphics.initialized = false;
    LOGI("Graphics binding shut down");
}
