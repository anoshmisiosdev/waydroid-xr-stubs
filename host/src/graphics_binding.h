/*
 * graphics_binding.h — OpenXR Graphics Binding Setup
 *
 * Configures graphics API support for OpenXR (EGL/OpenGL vs Vulkan).
 * Detects available APIs and creates appropriate binding structures for
 * XrSessionCreateInfo.
 *
 * License: Apache 2.0
 */

#pragma once

#include <openxr/openxr.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GRAPHICS_API_UNKNOWN,
    GRAPHICS_API_OPENGL,
    GRAPHICS_API_VULKAN,
    GRAPHICS_API_OPENGL_ES,
} GraphicsApi;

/* =========================================================================
 * Graphics API Detection and Initialization
 * ========================================================================= */

/*
 * graphics_binding_detect_available — Check which APIs are available on system.
 * Returns bitmask of available GraphicsApi types.
 */
uint32_t graphics_binding_detect_available(void);

/*
 * graphics_binding_init_egl — Initialize EGL/OpenGL bindings.
 * Returns the graphics binding structure to attach to XrSessionCreateInfo.
 * Returns NULL if EGL is not available.
 */
void *graphics_binding_init_egl(void);

/*
 * graphics_binding_init_vulkan — Initialize Vulkan bindings.
 * Returns the graphics binding structure to attach to XrSessionCreateInfo.
 * Returns NULL if Vulkan is not available.
 */
void *graphics_binding_init_vulkan(XrInstance instance, XrSystemId system_id);

/*
 * graphics_binding_get_selected_api — Get currently selected graphics API.
 */
GraphicsApi graphics_binding_get_selected_api(void);

/*
 * graphics_binding_shutdown — Clean up graphics resources.
 */
void graphics_binding_shutdown(void);

#ifdef __cplusplus
}
#endif
