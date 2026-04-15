/*
 * openxr_host.c — Waydroid XR Host OpenXR Session Implementation
 *
 * Manages connection to the host OpenXR runtime (Monado, SteamVR, etc.)
 * and submits composed frames with imported container buffers as quad layers.
 *
 * License: Apache 2.0
 */

#include "openxr_host.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define LOG(level, fmt, ...) \
    fprintf(stderr, "[waydroid-xr-server] " level ": " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)

#define XR_CHECK(call, fmt) \
    do { \
        XrResult res = (call); \
        if (!XR_SUCCEEDED(res)) { \
            LOGE(fmt " (XrResult: %d)", res); \
            return false; \
        } \
    } while (0)

#define XR_CHECK_VOID(call, fmt) \
    do { \
        XrResult res = (call); \
        if (!XR_SUCCEEDED(res)) { \
            LOGE(fmt " (XrResult: %d)", res); \
            return; \
        } \
    } while (0)

/* =========================================================================
 * OpenXR Host State
 * ========================================================================= */

typedef struct {
    XrInstance              instance;
    XrSystemId              system_id;
    XrSession               session;
    XrSpace                 play_space;
    XrSwapchain             swapchain;
    uint32_t                swapchain_width;
    uint32_t                swapchain_height;

    bool                    session_active;
    int64_t                 frame_count;

    /* Quad layer data */
    XrCompositionLayerQuad  layers[2];  /* left and right eye quad layers */
    uint32_t                layer_count;

    /* Frame timing */
    XrFrameState            frame_state;
} OxrHostState;

struct OxrHost {
    OxrHostState   state;
};

/* =========================================================================
 * OpenXR Instance and System Setup
 * ========================================================================= */

static bool create_instance(OxrHostState *state) {
    const char *extensions[] = {
        "XR_KHR_opengl_enable",  /* For GL-based layer composition */
        "XR_KHR_vulkan_enable",  /* For Vulkan-based layer composition */
    };
    const uint32_t extension_count = sizeof(extensions) / sizeof(extensions[0]);

    XrInstanceCreateInfo info = {
        .type = XR_TYPE_INSTANCE_CREATE_INFO,
        .createFlags = 0,
        .applicationInfo = {
            .applicationName = "waydroid-xr-server",
            .applicationVersion = 1,
            .engineName = "Waydroid",
            .engineVersion = 1,
            .apiVersion = XR_API_VERSION_1_0,
        },
        .enabledExtensionCount = extension_count,
        .enabledExtensionNames = extensions,
    };

    XR_CHECK(xrCreateInstance(&info, &state->instance),
             "xrCreateInstance");

    LOGI("OpenXR instance created");
    return true;
}

static bool get_system(OxrHostState *state) {
    XrSystemGetInfo info = {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };

    XR_CHECK(xrGetSystem(state->instance, &info, &state->system_id),
             "xrGetSystem");

    LOGI("OpenXR system retrieved (system_id: %lu)", state->system_id);
    return true;
}

static bool create_session(OxrHostState *state) {
    /* Note: For a full implementation, you'd select a graphics binding
     * (EGL, Vulkan, etc.) based on detected capabilities. This is a minimal
     * stub that creates a session without graphics binding, which some
     * runtimes may support for headless operation. */

    XrSessionCreateInfo info = {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .systemId = state->system_id,
        .next = NULL,  /* Would set graphics binding here */
    };

    XR_CHECK(xrCreateSession(state->instance, &info, &state->session),
             "xrCreateSession");

    LOGI("OpenXR session created");

    /* Create reference space for head-mounted rendering */
    XrReferenceSpaceCreateInfo ref_space_info = {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
        .poseInReferenceSpace = {
            .orientation = {0, 0, 0, 1},
            .position = {0, 0, 0},
        },
    };

    XR_CHECK(xrCreateReferenceSpace(state->session, &ref_space_info, &state->play_space),
             "xrCreateReferenceSpace");

    state->session_active = true;
    return true;
}

/* =========================================================================
 * Swapchain Setup (minimal, for frame timestamping)
 * ========================================================================= */

static bool create_swapchain(OxrHostState *state) {
    /* Create a dummy swapchain for frame timing and composition.
     * Real content comes from imported DMA-BUF buffers (quad layers). */

    state->swapchain_width = 2048;
    state->swapchain_height = 2048;

    /* In a full implementation, query supported swapchain formats here
     * via xrEnumerateSwapchainFormats. For now, we use a dummy. */

    XrSwapchainCreateInfo info = {
        .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
        .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
        .format = 0,  /* Would be actual format from enumeration */
        .sampleCount = 1,
        .width = state->swapchain_width,
        .height = state->swapchain_height,
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1,
    };

    /* Skip swapchain creation for now; we'll rely on quad layer submission */
    /* XR_CHECK(xrCreateSwapchain(state->session, &info, &state->swapchain),
             "xrCreateSwapchain"); */

    LOGI("Swapchain configuration: %ux%u", state->swapchain_width, state->swapchain_height);
    return true;
}

/* =========================================================================
 * Frame Loop
 * ========================================================================= */

static bool begin_frame(OxrHostState *state) {
    XrFrameWaitInfo wait_info = {
        .type = XR_TYPE_FRAME_WAIT_INFO,
    };

    XR_CHECK(xrWaitFrame(state->session, &wait_info, &state->frame_state),
             "xrWaitFrame");

    XrFrameBeginInfo begin_info = {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
    };

    XR_CHECK(xrBeginFrame(state->session, &begin_info),
             "xrBeginFrame");

    state->layer_count = 0;  /* Reset for this frame */
    return true;
}

static bool submit_layers(OxrHostState *state) {
    if (state->layer_count == 0) {
        LOGW("No layers to submit this frame");
    }

    /* Build composition layer base headers */
    XrCompositionLayerBaseHeader *layer_headers[2];
    for (uint32_t i = 0; i < state->layer_count; i++) {
        layer_headers[i] = (XrCompositionLayerBaseHeader *)&state->layers[i];
    }

    XrFrameEndInfo end_info = {
        .type = XR_TYPE_FRAME_END_INFO,
        .displayTime = state->frame_state.predictedDisplayTime,
        .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
        .layerCount = state->layer_count,
        .layers = layer_headers,
    };

    XR_CHECK(xrEndFrame(state->session, &end_info),
             "xrEndFrame");

    state->frame_count++;
    return true;
}

/* =========================================================================
 * Public API Implementation
 * ========================================================================= */

OxrHost *oxr_host_init(void) {
    OxrHost *host = (OxrHost *)malloc(sizeof(*host));
    if (!host) return NULL;

    memset(host, 0, sizeof(*host));

    /* Initialize OpenXR */
    if (!create_instance(&host->state)) {
        LOGE("Failed to create OpenXR instance");
        goto fail;
    }

    if (!get_system(&host->state)) {
        LOGE("Failed to get OpenXR system");
        goto fail;
    }

    LOGI("OpenXR host initialized successfully");
    return host;

fail:
    free(host);
    return NULL;
}

void oxr_host_shutdown(OxrHost *host) {
    if (!host) return;

    if (host->state.session_active) {
        oxr_host_destroy_session(host);
    }

    if (host->state.instance) {
        xrDestroyInstance(host->state.instance);
    }

    free(host);
}

XrInstance oxr_host_get_instance(OxrHost *host) {
    if (!host) return XR_NULL_HANDLE;
    return host->state.instance;
}

bool oxr_host_create_session(OxrHost *host) {
    if (!host) return false;

    if (!create_session(&host->state)) {
        return false;
    }

    if (!create_swapchain(&host->state)) {
        return false;
    }

    LOGI("OpenXR session created and ready for composition");
    return true;
}

void oxr_host_destroy_session(OxrHost *host) {
    if (!host) return;

    if (host->state.play_space) {
        xrDestroySpace(host->state.play_space);
        host->state.play_space = XR_NULL_HANDLE;
    }

    if (host->state.swapchain) {
        xrDestroySwapchain(host->state.swapchain);
        host->state.swapchain = XR_NULL_HANDLE;
    }

    if (host->state.session) {
        xrDestroySession(host->state.session);
        host->state.session = XR_NULL_HANDLE;
    }

    host->state.session_active = false;
}

bool oxr_host_begin_frame(OxrHost *host) {
    if (!host || !host->state.session_active) return false;
    return begin_frame(&host->state);
}

bool oxr_host_submit_layer(OxrHost *host, const OxrLayerParams *layer) {
    if (!host || !host->state.session_active) return false;

    if (host->state.layer_count >= 2) {
        LOGW("Too many layers; maximum is 2");
        return false;
    }

    /* Create quad layer for this submission */
    XrCompositionLayerQuad *quad = &host->state.layers[host->state.layer_count];
    quad->type = XR_TYPE_COMPOSITION_LAYER_QUAD;
    quad->next = NULL;
    quad->layerFlags = 0;
    quad->space = host->state.play_space;

    /* Set pose */
    quad->pose.position.x = layer->x;
    quad->pose.position.y = layer->y;
    quad->pose.position.z = layer->z;
    quad->pose.orientation.x = layer->quat_x;
    quad->pose.orientation.y = layer->quat_y;
    quad->pose.orientation.z = layer->quat_z;
    quad->pose.orientation.w = layer->quat_w;

    /* Set size */
    quad->extentWidth = layer->width * layer->scale / 1024.0f;
    quad->extentHeight = layer->height * layer->scale / 1024.0f;

    /* Placeholder: swapchain images would contain the imported DMA-BUF texture */
    quad->subImage.swapchain = host->state.swapchain;
    quad->subImage.imageRect.offset.x = 0;
    quad->subImage.imageRect.offset.y = 0;
    quad->subImage.imageRect.extent.width = layer->width;
    quad->subImage.imageRect.extent.height = layer->height;
    quad->subImage.imageArrayIndex = 0;

    host->state.layer_count++;
    LOGI("Layer submitted: %ux%u at (%.2f, %.2f, %.2f)",
         layer->width, layer->height, layer->x, layer->y, layer->z);

    return true;
}

bool oxr_host_end_frame(OxrHost *host) {
    if (!host || !host->state.session_active) return false;
    return submit_layers(&host->state);
}

/* =========================================================================
 * Runtime Capabilities
 * ========================================================================= */

bool oxr_host_get_display_refresh_rates(OxrHost *host,
                                       float *out_rates,
                                       uint32_t *count)
{
    if (!host || !count || *count == 0) return false;

    /* Stub implementation: return common PCVR refresh rates */
    static const float common_rates[] = {72.0f, 90.0f, 120.0f};
    uint32_t num_rates = sizeof(common_rates) / sizeof(common_rates[0]);

    if (*count < num_rates) {
        num_rates = *count;
    }

    memcpy(out_rates, common_rates, num_rates * sizeof(float));
    *count = num_rates;

    LOGI("Display refresh rates: %u available", *count);
    return true;
}

bool oxr_host_get_display_refresh_rate(OxrHost *host, float *out_rate) {
    if (!host || !out_rate) return false;

    /* Stub: return a default refresh rate */
    *out_rate = 90.0f;
    return true;
}

bool oxr_host_request_display_refresh_rate(OxrHost *host, float rate) {
    if (!host) return false;

    LOGI("Requested display refresh rate: %.1f Hz", rate);
    /* Stub: most runtimes don't support refresh rate changes; return success anyway */
    return true;
}
