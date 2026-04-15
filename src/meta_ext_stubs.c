/*
 * waydroid-xr-stubs: Meta OpenXR Extension Stub Implementations
 *
 * These stubs allow Quest APKs running inside Waydroid to:
 *   1. Successfully enumerate and enable Meta extensions
 *   2. Not crash when calling extension functions
 *   3. Return sensible fallback values where possible
 *   4. Forward to host OpenXR runtime via IPC bridge (when implemented)
 *
 * Strategy per extension:
 *   - Core rendering extensions (display refresh rate, color space,
 *     foveation): forward to host runtime or return safe defaults
 *   - Passthrough: stub as "not supported" — no camera on PC
 *   - Spatial entities / scene understanding: stub with empty results
 *   - Face/body/eye tracking: stub as "not supported"
 *   - Performance metrics: return zero counters
 *   - Boundary / guardian: always return "boundary not visible"
 *
 * License: Apache 2.0
 */

#include "openxr/meta_ext_stubs.h"
#include "ipc_bridge.h"
#include <string.h>
#include <android/log.h>

#define LOG_TAG "WaydroidXR"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* =========================================================================
 * Extension enumeration — advertise all Meta extensions as "supported"
 * so apps don't bail out during xrCreateInstance.
 * The host will only actually implement the ones it can.
 * ========================================================================= */

static const char *s_meta_extensions[] = {
    XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME,
    XR_FB_COLOR_SPACE_EXTENSION_NAME,
    XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME,
    XR_FB_FOVEATION_EXTENSION_NAME,
    XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME,
    XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME,
    XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME,
    XR_FB_HAND_TRACKING_CAPSULES_EXTENSION_NAME,
    XR_FB_PASSTHROUGH_EXTENSION_NAME,
    XR_FB_SPATIAL_ENTITY_EXTENSION_NAME,
    XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME,
    XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME,
    XR_FB_SCENE_EXTENSION_NAME,
    XR_FB_SCENE_CAPTURE_EXTENSION_NAME,
    XR_FB_FACE_TRACKING_EXTENSION_NAME,
    XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME,
    XR_FB_BODY_TRACKING_EXTENSION_NAME,
    XR_FB_HAPTIC_PCM_EXTENSION_NAME,
    XR_FB_HAPTIC_AMPLITUDE_ENVELOPE_EXTENSION_NAME,
    XR_FB_TOUCH_CONTROLLER_PRO_EXTENSION_NAME,
    XR_META_PERFORMANCE_METRICS_EXTENSION_NAME,
    XR_META_VULKAN_SWAPCHAIN_CREATE_INFO_EXTENSION_NAME,
    XR_META_VIRTUAL_KEYBOARD_EXTENSION_NAME,
    XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME,
    XR_META_BOUNDARY_VISIBILITY_EXTENSION_NAME,
    XR_META_SPATIAL_ENTITY_MESH_EXTENSION_NAME,
    XR_META_AUTOMATIC_LAYER_FILTER_EXTENSION_NAME,
    XR_META_TOUCH_CONTROLLER_PLUS_EXTENSION_NAME,
    XR_META_BODY_TRACKING_FIDELITY_EXTENSION_NAME,
    XR_OCULUS_ANDROID_SESSION_STATE_ENABLE_EXTENSION_NAME,
    XR_OCULUS_AUDIO_DEVICE_GUID_EXTENSION_NAME,
};

#define META_EXT_COUNT (sizeof(s_meta_extensions) / sizeof(s_meta_extensions[0]))

/*
 * xrWaydroidEnumerateMetaExtensions
 *
 * Called from our custom OpenXR loader layer to inject Meta extension
 * names into the list returned by xrEnumerateInstanceExtensionProperties.
 */
uint32_t xrWaydroidMetaExtensionCount(void) {
    return (uint32_t)META_EXT_COUNT;
}

const char **xrWaydroidMetaExtensionNames(void) {
    return s_meta_extensions;
}

/* =========================================================================
 * Helper: allocate a stub handle (simple monotonic counter)
 * Real implementations would use the IPC bridge to create host-side objects
 * ========================================================================= */

static uint64_t s_handle_counter = 0x1000;

static uint64_t alloc_stub_handle(void) {
    return __sync_fetch_and_add(&s_handle_counter, 1);
}

/* =========================================================================
 * XR_FB_display_refresh_rate — FORWARD to host
 * The host runtime knows what refresh rates the PCVR headset supports.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateDisplayRefreshRatesFB(
    XrSession session,
    uint32_t displayRefreshRateCapacityInput,
    uint32_t *displayRefreshRateCountOutput,
    float *displayRefreshRates)
{
    LOGI("xrEnumerateDisplayRefreshRatesFB: forwarding to host");

    /* Try to forward to host IPC bridge */
    if (ipc_bridge_is_connected()) {
        return ipc_bridge_EnumerateDisplayRefreshRates(
            session, displayRefreshRateCapacityInput,
            displayRefreshRateCountOutput, displayRefreshRates);
    }

    /* Fallback: advertise 72/90/120 Hz as typical PCVR rates */
    static const float fallback_rates[] = {72.0f, 90.0f, 120.0f};
    static const uint32_t fallback_count = 3;

    *displayRefreshRateCountOutput = fallback_count;
    if (displayRefreshRates && displayRefreshRateCapacityInput >= fallback_count) {
        memcpy(displayRefreshRates, fallback_rates,
               fallback_count * sizeof(float));
    }
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetDisplayRefreshRateFB(
    XrSession session,
    float *displayRefreshRate)
{
    if (ipc_bridge_is_connected()) {
        return ipc_bridge_GetDisplayRefreshRate(session, displayRefreshRate);
    }
    *displayRefreshRate = 90.0f; /* safe default */
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrRequestDisplayRefreshRateFB(
    XrSession session,
    float displayRefreshRate)
{
    LOGI("xrRequestDisplayRefreshRateFB: requesting %.1f Hz", displayRefreshRate);
    if (ipc_bridge_is_connected()) {
        return ipc_bridge_RequestDisplayRefreshRate(session, displayRefreshRate);
    }
    /* Silently accept — host runtime controls actual rate */
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_FB_color_space — PARTIAL FORWARD
 * Return sRGB/REC709 as the PC monitor default; apps won't crash.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateColorSpacesFB(
    XrSession session,
    uint32_t colorSpaceCapacityInput,
    uint32_t *colorSpaceCountOutput,
    XrColorSpaceFB *colorSpaces)
{
    static const XrColorSpaceFB supported[] = {
        XR_COLOR_SPACE_REC709_FB,
        XR_COLOR_SPACE_REC2020_FB,
    };
    *colorSpaceCountOutput = 2;
    if (colorSpaces && colorSpaceCapacityInput >= 2) {
        colorSpaces[0] = supported[0];
        colorSpaces[1] = supported[1];
    }
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSetColorSpaceFB(
    XrSession session,
    XrColorSpaceFB colorSpace)
{
    LOGI("xrSetColorSpaceFB: color space %d (stub, ignoring)", colorSpace);
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_FB_foveation — STUB (return success, do nothing)
 * Fixed foveation is Quest-specific hardware. On PC with full-res rendering
 * we can silently accept foveation profile creation and do nothing.
 * FFR is safe to ignore — apps render full res instead.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrCreateFoveationProfileFB(
    XrSession session,
    const XrFoveationProfileCreateInfoFB *createInfo,
    XrFoveationProfileFB *profile)
{
    LOGI("xrCreateFoveationProfileFB: stubbed (full-res rendering on PC)");
    *profile = (XrFoveationProfileFB)(uintptr_t)alloc_stub_handle();
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyFoveationProfileFB(
    XrFoveationProfileFB profile)
{
    /* Nothing to free in stub */
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_FB_passthrough — STUBBED AS UNSUPPORTED
 * No camera feed available on a PCVR headset (unless the headset has one,
 * in which case the host runtime should handle it natively via OpenXR).
 * We create the objects but passthrough rendering produces nothing.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrCreatePassthroughFB(
    XrSession session,
    const XrPassthroughCreateInfoFB *createInfo,
    XrPassthroughFB *outPassthrough)
{
    LOGW("xrCreatePassthroughFB: no passthrough on PC, returning stub handle");
    *outPassthrough = (XrPassthroughFB)(uintptr_t)alloc_stub_handle();
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyPassthroughFB(
    XrPassthroughFB passthrough)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughStartFB(
    XrPassthroughFB passthrough)
{
    LOGW("xrPassthroughStartFB: passthrough not available on PC (stub)");
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughPauseFB(
    XrPassthroughFB passthrough)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreatePassthroughLayerFB(
    XrSession session,
    const XrPassthroughLayerCreateInfoFB *createInfo,
    XrPassthroughLayerFB *outLayer)
{
    LOGW("xrCreatePassthroughLayerFB: stubbed — layer will be empty");
    *outLayer = (XrPassthroughLayerFB)(uintptr_t)alloc_stub_handle();
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyPassthroughLayerFB(
    XrPassthroughLayerFB layer)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughLayerPauseFB(
    XrPassthroughLayerFB layer)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughLayerResumeFB(
    XrPassthroughLayerFB layer)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrPassthroughLayerSetStyleFB(
    XrPassthroughLayerFB layer,
    const void *style)
{
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_FB_spatial_entity / XR_FB_scene — EMPTY STUBS
 * Room mapping and spatial anchors require real world data.
 * Return empty results so apps degrade gracefully.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrCreateSpatialAnchorFB(
    XrSession session,
    const XrSpatialAnchorCreateInfoFB *info,
    XrAsyncRequestIdFB *requestId)
{
    LOGW("xrCreateSpatialAnchorFB: spatial entities not available (stub)");
    *requestId = (XrAsyncRequestIdFB)alloc_stub_handle();
    /*
     * We intentionally don't fire the async completion event.
     * Apps that check for event completion will timeout and skip
     * spatial features rather than crash.
     */
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetSpaceUuidFB(
    XrSpace space,
    XrUuidEXT *uuid)
{
    memset(uuid, 0, sizeof(*uuid));
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSpaceSupportedComponentsFB(
    XrSpace space,
    uint32_t componentTypeCapacityInput,
    uint32_t *componentTypeCountOutput,
    XrSpaceComponentTypeFB *componentTypes)
{
    /* No components supported in stub */
    *componentTypeCountOutput = 0;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSetSpaceComponentStatusFB(
    XrSpace space,
    const XrSpaceComponentStatusSetInfoFB *info,
    XrAsyncRequestIdFB *requestId)
{
    *requestId = (XrAsyncRequestIdFB)alloc_stub_handle();
    return XR_ERROR_SPACE_COMPONENT_NOT_SUPPORTED_FB;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetSpaceComponentStatusFB(
    XrSpace space,
    XrSpaceComponentTypeFB componentType,
    XrSpaceComponentStatusFB *status)
{
    status->enabled = XR_FALSE;
    status->changePending = XR_FALSE;
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_FB_hand_tracking_mesh — FORWARD mesh from host if available
 * Falls back to zero-joint mesh if host doesn't support hand tracking.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrGetHandMeshFB(
    XrHandTrackerEXT handTracker,
    XrHandTrackingMeshFB *mesh)
{
    if (ipc_bridge_is_connected() && ipc_bridge_supports_hand_mesh()) {
        return ipc_bridge_GetHandMesh(handTracker, mesh);
    }

    /* Return empty mesh — hand rendering will just be invisible */
    LOGW("xrGetHandMeshFB: hand mesh not available on this PC runtime");
    if (mesh->jointCapacityInput > 0) mesh->jointCountOutput = 0;
    if (mesh->vertexCapacityInput > 0) mesh->vertexCountOutput = 0;
    if (mesh->indexCapacityInput > 0)  mesh->indexCountOutput = 0;
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_FB_face_tracking — STUBBED AS NOT SUPPORTED
 * No facial camera on PCVR headsets.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrCreateFaceTrackerFB(
    XrSession session,
    const void *createInfo,
    XrFaceTrackerFB *faceTracker)
{
    LOGW("xrCreateFaceTrackerFB: face tracking not available on PC");
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyFaceTrackerFB(
    XrFaceTrackerFB faceTracker)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetFaceExpressionWeightsFB(
    XrFaceTrackerFB faceTracker,
    const void *expressionInfo,
    void *expressionWeights)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

/* =========================================================================
 * XR_FB_body_tracking — STUBBED AS NOT SUPPORTED
 * Full body tracking requires Quest Pro / Quest 3 sensor suite.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrCreateBodyTrackerFB(
    XrSession session,
    const void *createInfo,
    XrBodyTrackerFB *bodyTracker)
{
    LOGW("xrCreateBodyTrackerFB: body tracking not available on PC");
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyBodyTrackerFB(
    XrBodyTrackerFB bodyTracker)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrLocateBodyJointsFB(
    XrBodyTrackerFB bodyTracker,
    const void *locateInfo,
    void *locations)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetBodySkeletonFB(
    XrBodyTrackerFB bodyTracker,
    void *skeleton)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

/* =========================================================================
 * XR_META_performance_metrics — RETURN ZERO COUNTERS
 * Apps use this for debugging. Returning 0 is safe.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrEnumeratePerformanceMetricsCounterPathsMETA(
    XrInstance instance,
    uint32_t counterPathCapacityInput,
    uint32_t *counterPathCountOutput,
    XrPath *counterPaths)
{
    *counterPathCountOutput = 0;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSetPerformanceMetricsStateMETA(
    XrSession session,
    const XrPerformanceMetricsStateMETA *state)
{
    LOGI("xrSetPerformanceMetricsStateMETA: enabled=%d (stub)", state->enabled);
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetPerformanceMetricsStateMETA(
    XrSession session,
    XrPerformanceMetricsStateMETA *state)
{
    state->enabled = XR_FALSE;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrQueryPerformanceMetricsCounterMETA(
    XrSession session,
    XrPath counterPath,
    XrPerformanceMetricsCounterMETA *counter)
{
    counter->counterFlags = 0;
    counter->uintValue = 0;
    counter->floatValue = 0.0f;
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_META_boundary_visibility — ALWAYS SUPPRESS
 * No Guardian boundary on PC. Return "suppressed" so apps don't try to
 * draw boundary visualizations in the rendered frame.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrRequestBoundaryVisibilityMETA(
    XrSession session,
    XrBoundaryVisibilityMETA boundaryVisibility)
{
    LOGI("xrRequestBoundaryVisibilityMETA: %d (stub, boundary suppressed on PC)",
         boundaryVisibility);
    return XR_SUCCESS;
}

/* =========================================================================
 * XR_META_virtual_keyboard — STUB (return not supported)
 * Virtual keyboard is Quest-specific UI. On PC the system keyboard is used.
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrCreateVirtualKeyboardMETA(
    XrSession session,
    const XrVirtualKeyboardCreateInfoMETA *createInfo,
    XrVirtualKeyboardMETA *keyboard)
{
    LOGW("xrCreateVirtualKeyboardMETA: not supported on PC (stub)");
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyVirtualKeyboardMETA(
    XrVirtualKeyboardMETA keyboard)
{
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSuggestVirtualKeyboardLocationMETA(
    XrVirtualKeyboardMETA keyboard, const void *locationInfo)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetVirtualKeyboardScaleMETA(
    XrVirtualKeyboardMETA keyboard, float *scale)
{
    *scale = 1.0f;
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSetVirtualKeyboardModelVisibilityMETA(
    XrVirtualKeyboardMETA keyboard, const void *modelVisibility)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetVirtualKeyboardModelAnimationStatesMETA(
    XrVirtualKeyboardMETA keyboard, void *animationStates)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetVirtualKeyboardDirtyTexturesMETA(
    XrVirtualKeyboardMETA keyboard,
    uint32_t textureIdCapacityInput,
    uint32_t *textureIdCountOutput,
    uint64_t *textureIds)
{
    *textureIdCountOutput = 0;
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetVirtualKeyboardTextureDataMETA(
    XrVirtualKeyboardMETA keyboard,
    uint64_t textureId, void *textureData)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrSendVirtualKeyboardInputMETA(
    XrVirtualKeyboardMETA keyboard,
    const void *info,
    XrPosef *interactorRootPose)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

XRAPI_ATTR XrResult XRAPI_CALL xrChangeVirtualKeyboardTextContextMETA(
    XrVirtualKeyboardMETA keyboard, const void *changeInfo)
{
    return XR_ERROR_FEATURE_UNSUPPORTED;
}

/* =========================================================================
 * Dispatch table loader
 * Tries xrGetInstanceProcAddr first; falls back to our stub symbols.
 * ========================================================================= */

#define LOAD_EXT(name) \
    xrGetInstanceProcAddr(instance, "xr" #name, \
        (PFN_xrVoidFunction *)&table->name); \
    if (!table->name) table->name = (PFN_xr##name)xr##name;

XrResult xrMetaExtLoadDispatchTable(
    XrInstance instance,
    XrMetaExtensionDispatchTable *table)
{
    memset(table, 0, sizeof(*table));

    LOAD_EXT(EnumerateDisplayRefreshRatesFB)
    LOAD_EXT(GetDisplayRefreshRateFB)
    LOAD_EXT(RequestDisplayRefreshRateFB)
    LOAD_EXT(EnumerateColorSpacesFB)
    LOAD_EXT(SetColorSpaceFB)
    LOAD_EXT(CreateFoveationProfileFB)
    LOAD_EXT(DestroyFoveationProfileFB)
    LOAD_EXT(CreatePassthroughFB)
    LOAD_EXT(DestroyPassthroughFB)
    LOAD_EXT(PassthroughStartFB)
    LOAD_EXT(PassthroughPauseFB)
    LOAD_EXT(CreatePassthroughLayerFB)
    LOAD_EXT(DestroyPassthroughLayerFB)
    LOAD_EXT(PassthroughLayerPauseFB)
    LOAD_EXT(PassthroughLayerResumeFB)
    LOAD_EXT(PassthroughLayerSetStyleFB)
    LOAD_EXT(CreateSpatialAnchorFB)
    LOAD_EXT(GetSpaceUuidFB)
    LOAD_EXT(EnumerateSpaceSupportedComponentsFB)
    LOAD_EXT(SetSpaceComponentStatusFB)
    LOAD_EXT(GetSpaceComponentStatusFB)
    LOAD_EXT(GetHandMeshFB)
    LOAD_EXT(CreateFaceTrackerFB)
    LOAD_EXT(DestroyFaceTrackerFB)
    LOAD_EXT(GetFaceExpressionWeightsFB)
    LOAD_EXT(CreateBodyTrackerFB)
    LOAD_EXT(DestroyBodyTrackerFB)
    LOAD_EXT(LocateBodyJointsFB)
    LOAD_EXT(GetBodySkeletonFB)
    LOAD_EXT(EnumeratePerformanceMetricsCounterPathsMETA)
    LOAD_EXT(SetPerformanceMetricsStateMETA)
    LOAD_EXT(GetPerformanceMetricsStateMETA)
    LOAD_EXT(QueryPerformanceMetricsCounterMETA)
    LOAD_EXT(RequestBoundaryVisibilityMETA)
    LOAD_EXT(CreateVirtualKeyboardMETA)
    LOAD_EXT(DestroyVirtualKeyboardMETA)
    LOAD_EXT(SuggestVirtualKeyboardLocationMETA)
    LOAD_EXT(GetVirtualKeyboardScaleMETA)
    LOAD_EXT(SetVirtualKeyboardModelVisibilityMETA)
    LOAD_EXT(GetVirtualKeyboardModelAnimationStatesMETA)
    LOAD_EXT(GetVirtualKeyboardDirtyTexturesMETA)
    LOAD_EXT(GetVirtualKeyboardTextureDataMETA)
    LOAD_EXT(SendVirtualKeyboardInputMETA)
    LOAD_EXT(ChangeVirtualKeyboardTextContextMETA)

    return XR_SUCCESS;
}
