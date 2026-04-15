/*
 * openxr_host.h — Waydroid XR Host OpenXR Session Management
 *
 * Manages the host-side OpenXR session, connecting to the local runtime
 * (Monado, SteamVR, etc.) and compositing container frame buffers as quad layers.
 *
 * Workflow:
 *   1. oxr_host_init() — Create instance, system, environment blend mode selection
 *   2. oxr_host_create_session() — Set up swapchain for framing
 *   3. oxr_host_begin_frame() — Get predictions, start rendering
 *   4. oxr_host_submit_layer() — Queue a quad layer with an imported DMA-BUF image
 *   5. oxr_host_end_frame() — Submit frame to runtime
 *   6. oxr_host_destroy_session() — Clean up
 *   7. oxr_host_shutdown() — Destroy instance
 *
 * License: Apache 2.0
 */

#pragma once

#include <openxr/openxr.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OxrHost OxrHost;

/* =========================================================================
 * Instance and runtime discovery
 * ========================================================================= */

/*
 * oxr_host_init — Create OpenXR instance connected to the local runtime.
 * Returns NULL on failure (no runtime, extension unavailable, etc.).
 */
OxrHost *oxr_host_init(void);

/*
 * oxr_host_shutdown — Destroy instance and free resources.
 */
void oxr_host_shutdown(OxrHost *host);

/*
 * oxr_host_get_instance — Retrieve the raw XrInstance (for advanced use).
 */
XrInstance oxr_host_get_instance(OxrHost *host);

/* =========================================================================
 * Session and frame submission
 * ========================================================================= */

/*
 * oxr_host_create_session — Create a session for frame composition.
 * Sets up an internal swapchain and initial frame state.
 * Returns false on failure.
 */
bool oxr_host_create_session(OxrHost *host);

/*
 * oxr_host_destroy_session — Clean up session and swapchain.
 */
void oxr_host_destroy_session(OxrHost *host);

/*
 * oxr_host_begin_frame — Start a new frame (call xrBeginFrame, get predictions).
 * Must be called before oxr_host_submit_layer().
 * Returns false on error.
 */
bool oxr_host_begin_frame(OxrHost *host);

/*
 * oxr_host_submit_layer — Register a quad layer for this frame.
 *   texture_handle: Opaque GPU handle to the imported DMA-BUF texture
 *   width, height: Layer dimensions
 *   x, y, z: Layer position (meters in world space)
 *   scale: Size scaling factor
 *   rotation: Unit quaternion (x, y, z, w)
 *
 * Multiple calls per frame create multiple layers (one per eye buffer, typically).
 * Returns false on error.
 */
typedef struct {
    void       *texture_handle;  /* GPU driver's texture identifier */
    uint32_t    width;
    uint32_t    height;
    float       x, y, z;         /* Layer pose relative to head */
    float       scale;
    float       quat_x, quat_y, quat_z, quat_w;  /* Rotation */
} OxrLayerParams;

bool oxr_host_submit_layer(OxrHost *host, const OxrLayerParams *layer);

/*
 * oxr_host_end_frame — Submit the frame to the runtime.
 * Returns false on failure.
 */
bool oxr_host_end_frame(OxrHost *host);

/* =========================================================================
 * Runtime capabilities
 * ========================================================================= */

/*
 * oxr_host_get_display_refresh_rates — Query supported display refresh rates.
 *   out_rates: Caller-provided array
 *   count: On input, array capacity; on output, actual count
 * Returns true on success.
 */
bool oxr_host_get_display_refresh_rates(OxrHost *host,
                                       float *out_rates,
                                       uint32_t *count);

/*
 * oxr_host_get_display_refresh_rate — Query current display refresh rate.
 */
bool oxr_host_get_display_refresh_rate(OxrHost *host, float *out_rate);

/*
 * oxr_host_request_display_refresh_rate — Request a specific refresh rate.
 * Some runtimes may not honor the request.
 */
bool oxr_host_request_display_refresh_rate(OxrHost *host, float rate);

#ifdef __cplusplus
}
#endif
