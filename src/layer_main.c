/*
 * layer_main.c — Waydroid XR API Layer
 *
 * This is an OpenXR API layer that lives inside the Waydroid container.
 * It intercepts the OpenXR loader chain to:
 *   1. Inject Meta extension names into xrEnumerateInstanceExtensionProperties
 *   2. Route Meta extension function pointers to our stubs
 *   3. Connect to the host IPC bridge on xrCreateInstance
 *   4. Disconnect on xrDestroyInstance
 *
 * The layer is registered via assets/openxr_layer_manifest.json in the APK,
 * or placed at /vendor/etc/openxr/1/api_layers/implicit.d/ in the Waydroid
 * system image.
 *
 * Layer name: XR_APILAYER_waydroid_meta_compat
 *
 * License: Apache 2.0
 */

#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <openxr/openxr_platform.h>
#include "ipc_bridge.h"
#include "openxr/meta_ext_stubs.h"

#include <string.h>
#include <stdlib.h>
#include <android/log.h>

#define LOG_TAG "WaydroidXR-Layer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* =========================================================================
 * Layer chain: next function pointers captured at negotiation time
 * ========================================================================= */

static PFN_xrGetInstanceProcAddr         s_next_xrGetInstanceProcAddr = NULL;
static PFN_xrCreateInstance              s_next_xrCreateInstance = NULL;
static PFN_xrDestroyInstance             s_next_xrDestroyInstance = NULL;
static PFN_xrEnumerateInstanceExtensionProperties
                            s_next_xrEnumerateInstanceExtensionProperties = NULL;

/* =========================================================================
 * xrEnumerateInstanceExtensionProperties — inject Meta extensions
 * ========================================================================= */

static XRAPI_ATTR XrResult XRAPI_CALL layer_xrEnumerateInstanceExtensionProperties(
    const char *layerName,
    uint32_t propertyCapacityInput,
    uint32_t *propertyCountOutput,
    XrExtensionProperties *properties)
{
    /* Get the real count first */
    uint32_t real_count = 0;
    XrResult result = s_next_xrEnumerateInstanceExtensionProperties(
        layerName, 0, &real_count, NULL);
    if (XR_FAILED(result)) return result;

    uint32_t meta_count = xrWaydroidMetaExtensionCount();
    uint32_t total = real_count + meta_count;
    *propertyCountOutput = total;

    if (propertyCapacityInput == 0 || properties == NULL) {
        return XR_SUCCESS;
    }
    if (propertyCapacityInput < total) {
        return XR_ERROR_SIZE_INSUFFICIENT;
    }

    /* Fetch real extensions */
    result = s_next_xrEnumerateInstanceExtensionProperties(
        layerName, real_count, &real_count, properties);
    if (XR_FAILED(result)) return result;

    /* Append Meta stub extensions (skip duplicates) */
    const char **meta_names = xrWaydroidMetaExtensionNames();
    uint32_t appended = 0;
    for (uint32_t i = 0; i < meta_count; i++) {
        /* Check for duplicate */
        bool found = false;
        for (uint32_t j = 0; j < real_count + appended; j++) {
            if (strcmp(properties[j].extensionName, meta_names[i]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            XrExtensionProperties *ep = &properties[real_count + appended];
            ep->type = XR_TYPE_EXTENSION_PROPERTIES;
            ep->next = NULL;
            strncpy(ep->extensionName, meta_names[i],
                    XR_MAX_EXTENSION_NAME_SIZE - 1);
            ep->extensionVersion = 1;
            appended++;
        }
    }

    *propertyCountOutput = real_count + appended;
    return XR_SUCCESS;
}

/* =========================================================================
 * xrCreateInstance — connect IPC bridge
 * ========================================================================= */

static XRAPI_ATTR XrResult XRAPI_CALL layer_xrCreateInstance(
    const XrInstanceCreateInfo *createInfo,
    XrInstance *instance)
{
    LOGI("layer_xrCreateInstance: connecting IPC bridge...");
    bool host_connected = ipc_bridge_init();
    LOGI("layer_xrCreateInstance: host bridge %s",
         host_connected ? "connected" : "not available (stubs active)");

    return s_next_xrCreateInstance(createInfo, instance);
}

/* =========================================================================
 * xrDestroyInstance — disconnect IPC bridge
 * ========================================================================= */

static XRAPI_ATTR XrResult XRAPI_CALL layer_xrDestroyInstance(
    XrInstance instance)
{
    ipc_bridge_shutdown();
    return s_next_xrDestroyInstance(instance);
}

/* =========================================================================
 * xrGetInstanceProcAddr — route Meta extension functions to our stubs
 * ========================================================================= */

static XRAPI_ATTR XrResult XRAPI_CALL layer_xrGetInstanceProcAddr(
    XrInstance instance,
    const char *name,
    PFN_xrVoidFunction *function)
{
    /* --- XR_FB_display_refresh_rate --- */
    if (strcmp(name, "xrEnumerateDisplayRefreshRatesFB") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateDisplayRefreshRatesFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetDisplayRefreshRateFB") == 0) {
        *function = (PFN_xrVoidFunction)xrGetDisplayRefreshRateFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrRequestDisplayRefreshRateFB") == 0) {
        *function = (PFN_xrVoidFunction)xrRequestDisplayRefreshRateFB;
        return XR_SUCCESS;
    }

    /* --- XR_FB_color_space --- */
    if (strcmp(name, "xrEnumerateColorSpacesFB") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateColorSpacesFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrSetColorSpaceFB") == 0) {
        *function = (PFN_xrVoidFunction)xrSetColorSpaceFB;
        return XR_SUCCESS;
    }

    /* --- XR_FB_foveation --- */
    if (strcmp(name, "xrCreateFoveationProfileFB") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateFoveationProfileFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrDestroyFoveationProfileFB") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyFoveationProfileFB;
        return XR_SUCCESS;
    }

    /* --- XR_FB_passthrough --- */
    if (strcmp(name, "xrCreatePassthroughFB") == 0) {
        *function = (PFN_xrVoidFunction)xrCreatePassthroughFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrDestroyPassthroughFB") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyPassthroughFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrPassthroughStartFB") == 0) {
        *function = (PFN_xrVoidFunction)xrPassthroughStartFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrPassthroughPauseFB") == 0) {
        *function = (PFN_xrVoidFunction)xrPassthroughPauseFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrCreatePassthroughLayerFB") == 0) {
        *function = (PFN_xrVoidFunction)xrCreatePassthroughLayerFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrDestroyPassthroughLayerFB") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyPassthroughLayerFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrPassthroughLayerPauseFB") == 0) {
        *function = (PFN_xrVoidFunction)xrPassthroughLayerPauseFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrPassthroughLayerResumeFB") == 0) {
        *function = (PFN_xrVoidFunction)xrPassthroughLayerResumeFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrPassthroughLayerSetStyleFB") == 0) {
        *function = (PFN_xrVoidFunction)xrPassthroughLayerSetStyleFB;
        return XR_SUCCESS;
    }

    /* --- XR_FB_spatial_entity --- */
    if (strcmp(name, "xrCreateSpatialAnchorFB") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateSpatialAnchorFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetSpaceUuidFB") == 0) {
        *function = (PFN_xrVoidFunction)xrGetSpaceUuidFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrEnumerateSpaceSupportedComponentsFB") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumerateSpaceSupportedComponentsFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrSetSpaceComponentStatusFB") == 0) {
        *function = (PFN_xrVoidFunction)xrSetSpaceComponentStatusFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetSpaceComponentStatusFB") == 0) {
        *function = (PFN_xrVoidFunction)xrGetSpaceComponentStatusFB;
        return XR_SUCCESS;
    }

    /* --- XR_FB_hand_tracking_mesh --- */
    if (strcmp(name, "xrGetHandMeshFB") == 0) {
        *function = (PFN_xrVoidFunction)xrGetHandMeshFB;
        return XR_SUCCESS;
    }

    /* --- XR_FB_face_tracking --- */
    if (strcmp(name, "xrCreateFaceTrackerFB") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateFaceTrackerFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrDestroyFaceTrackerFB") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyFaceTrackerFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetFaceExpressionWeightsFB") == 0) {
        *function = (PFN_xrVoidFunction)xrGetFaceExpressionWeightsFB;
        return XR_SUCCESS;
    }

    /* --- XR_FB_body_tracking --- */
    if (strcmp(name, "xrCreateBodyTrackerFB") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateBodyTrackerFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrDestroyBodyTrackerFB") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyBodyTrackerFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrLocateBodyJointsFB") == 0) {
        *function = (PFN_xrVoidFunction)xrLocateBodyJointsFB;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetBodySkeletonFB") == 0) {
        *function = (PFN_xrVoidFunction)xrGetBodySkeletonFB;
        return XR_SUCCESS;
    }

    /* --- XR_META_performance_metrics --- */
    if (strcmp(name, "xrEnumeratePerformanceMetricsCounterPathsMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrEnumeratePerformanceMetricsCounterPathsMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrSetPerformanceMetricsStateMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrSetPerformanceMetricsStateMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetPerformanceMetricsStateMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrGetPerformanceMetricsStateMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrQueryPerformanceMetricsCounterMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrQueryPerformanceMetricsCounterMETA;
        return XR_SUCCESS;
    }

    /* --- XR_META_boundary_visibility --- */
    if (strcmp(name, "xrRequestBoundaryVisibilityMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrRequestBoundaryVisibilityMETA;
        return XR_SUCCESS;
    }

    /* --- XR_META_virtual_keyboard --- */
    if (strcmp(name, "xrCreateVirtualKeyboardMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrCreateVirtualKeyboardMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrDestroyVirtualKeyboardMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrDestroyVirtualKeyboardMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrSuggestVirtualKeyboardLocationMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrSuggestVirtualKeyboardLocationMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetVirtualKeyboardScaleMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrGetVirtualKeyboardScaleMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrSetVirtualKeyboardModelVisibilityMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrSetVirtualKeyboardModelVisibilityMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetVirtualKeyboardModelAnimationStatesMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrGetVirtualKeyboardModelAnimationStatesMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetVirtualKeyboardDirtyTexturesMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrGetVirtualKeyboardDirtyTexturesMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetVirtualKeyboardTextureDataMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrGetVirtualKeyboardTextureDataMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrSendVirtualKeyboardInputMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrSendVirtualKeyboardInputMETA;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrChangeVirtualKeyboardTextContextMETA") == 0) {
        *function = (PFN_xrVoidFunction)xrChangeVirtualKeyboardTextContextMETA;
        return XR_SUCCESS;
    }

    /* --- Layer's own intercepts --- */
    if (strcmp(name, "xrGetInstanceProcAddr") == 0) {
        *function = (PFN_xrVoidFunction)layer_xrGetInstanceProcAddr;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrEnumerateInstanceExtensionProperties") == 0) {
        *function = (PFN_xrVoidFunction)layer_xrEnumerateInstanceExtensionProperties;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrCreateInstance") == 0) {
        *function = (PFN_xrVoidFunction)layer_xrCreateInstance;
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrDestroyInstance") == 0) {
        *function = (PFN_xrVoidFunction)layer_xrDestroyInstance;
        return XR_SUCCESS;
    }

    /* Pass through everything else */
    return s_next_xrGetInstanceProcAddr(instance, name, function);
}

/* =========================================================================
 * Layer negotiation entry point — called by the OpenXR loader
 * ========================================================================= */

XRAPI_ATTR XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(
    const XrNegotiateLoaderInfo *loaderInfo,
    const char *layerName,
    XrNegotiateApiLayerRequest *apiLayerRequest)
{
    if (!loaderInfo || !apiLayerRequest) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    if (loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    /* Negotiate API version */
    if (loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
    apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
    apiLayerRequest->getInstanceProcAddr = layer_xrGetInstanceProcAddr;
    apiLayerRequest->createApiLayerInstance = NULL; /* use getInstanceProcAddr path */

    /* Capture next chain */
    s_next_xrGetInstanceProcAddr = loaderInfo->nextInfo->nextGetInstanceProcAddr;

    /* Pre-resolve chain functions we intercept */
    s_next_xrGetInstanceProcAddr(
        XR_NULL_HANDLE, "xrCreateInstance",
        (PFN_xrVoidFunction *)&s_next_xrCreateInstance);
    s_next_xrGetInstanceProcAddr(
        XR_NULL_HANDLE, "xrDestroyInstance",
        (PFN_xrVoidFunction *)&s_next_xrDestroyInstance);
    s_next_xrGetInstanceProcAddr(
        XR_NULL_HANDLE, "xrEnumerateInstanceExtensionProperties",
        (PFN_xrVoidFunction *)&s_next_xrEnumerateInstanceExtensionProperties);

    LOGI("xrNegotiateLoaderApiLayerInterface: waydroid-meta-compat layer loaded");
    return XR_SUCCESS;
}
