/*
 * waydroid-xr-stubs: Meta/FB/Oculus OpenXR Extension Stubs
 *
 * Stub implementations of Meta-proprietary OpenXR vendor extensions
 * for use inside a Waydroid (Android-in-Linux) container bridging to
 * a host OpenXR runtime (Monado / SteamVR).
 *
 * All extension names, struct types, and enumerants are derived from:
 *   - The public Khronos OpenXR Registry (xr.xml)
 *   - Meta's public OpenXR SDK (github.com/meta-quest/Meta-OpenXR-SDK)
 *   - The OpenXR 1.1 specification
 *
 * No proprietary Meta source code is used or reproduced.
 * This is a clean-room stub implementation.
 *
 * License: Apache 2.0
 */

#pragma once

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Extension name strings (from public Khronos registry)
 * ========================================================================= */

/* --- FB extensions (legacy / pre-Meta rebrand) --- */
#define XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME     "XR_FB_display_refresh_rate"
#define XR_FB_COLOR_SPACE_EXTENSION_NAME              "XR_FB_color_space"
#define XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME   "XR_FB_swapchain_update_state"
#define XR_FB_FOVEATION_EXTENSION_NAME                "XR_FB_foveation"
#define XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME  "XR_FB_foveation_configuration"
#define XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME       "XR_FB_hand_tracking_mesh"
#define XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME        "XR_FB_hand_tracking_aim"
#define XR_FB_HAND_TRACKING_CAPSULES_EXTENSION_NAME   "XR_FB_hand_tracking_capsules"
#define XR_FB_PASSTHROUGH_EXTENSION_NAME              "XR_FB_passthrough"
#define XR_FB_TRIANGLE_MESH_EXTENSION_NAME            "XR_FB_triangle_mesh"
#define XR_FB_SPATIAL_ENTITY_EXTENSION_NAME           "XR_FB_spatial_entity"
#define XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME     "XR_FB_spatial_entity_query"
#define XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME   "XR_FB_spatial_entity_storage"
#define XR_FB_SCENE_EXTENSION_NAME                    "XR_FB_scene"
#define XR_FB_SCENE_CAPTURE_EXTENSION_NAME            "XR_FB_scene_capture"
#define XR_FB_FACE_TRACKING_EXTENSION_NAME            "XR_FB_face_tracking"
#define XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME      "XR_FB_eye_tracking_social"
#define XR_FB_BODY_TRACKING_EXTENSION_NAME            "XR_FB_body_tracking"
#define XR_FB_HAPTIC_PCM_EXTENSION_NAME               "XR_FB_haptic_pcm"
#define XR_FB_HAPTIC_AMPLITUDE_ENVELOPE_EXTENSION_NAME "XR_FB_haptic_amplitude_envelope"
#define XR_FB_TOUCH_CONTROLLER_PRO_EXTENSION_NAME     "XR_FB_touch_controller_pro"

/* --- META extensions (post-rebrand) --- */
#define XR_META_PERFORMANCE_METRICS_EXTENSION_NAME        "XR_META_performance_metrics"
#define XR_META_VULKAN_SWAPCHAIN_CREATE_INFO_EXTENSION_NAME "XR_META_vulkan_swapchain_create_info"
#define XR_META_VIRTUAL_KEYBOARD_EXTENSION_NAME           "XR_META_virtual_keyboard"
#define XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME          "XR_META_environment_depth"
#define XR_META_BOUNDARY_VISIBILITY_EXTENSION_NAME        "XR_META_boundary_visibility"
#define XR_META_SPATIAL_ENTITY_MESH_EXTENSION_NAME        "XR_META_spatial_entity_mesh"
#define XR_META_AUTOMATIC_LAYER_FILTER_EXTENSION_NAME     "XR_META_automatic_layer_filter"
#define XR_META_TOUCH_CONTROLLER_PLUS_EXTENSION_NAME      "XR_META_touch_controller_plus"
#define XR_META_DETACHED_CONTROLLERS_EXTENSION_NAME       "XR_META_detached_controllers"
#define XR_META_BODY_TRACKING_FIDELITY_EXTENSION_NAME     "XR_META_body_tracking_fidelity"
#define XR_META_SPATIAL_ENTITY_SEMANTIC_LABEL_EXTENSION_NAME "XR_META_spatial_entity_semantic_label"

/* --- OCULUS extensions (legacy / mostly deprecated) --- */
#define XR_OCULUS_ANDROID_SESSION_STATE_ENABLE_EXTENSION_NAME "XR_OCULUS_android_session_state_enable"
#define XR_OCULUS_AUDIO_DEVICE_GUID_EXTENSION_NAME            "XR_OCULUS_audio_device_guid"

/* =========================================================================
 * XR_TYPE enumerants for Meta extension structs
 * Values match the public Khronos registry xr.xml
 * ========================================================================= */

typedef enum XrStructureTypeMetaExt {
    /* XR_FB_display_refresh_rate */
    XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB = 1000101000,

    /* XR_FB_color_space */
    XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB = 1000108000,

    /* XR_FB_swapchain_update_state */
    XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB        = 1000115000,
    XR_TYPE_SWAPCHAIN_STATE_SAMPLER_OPENGL_ES_FB = 1000078001,
    XR_TYPE_SWAPCHAIN_STATE_SAMPLER_VULKAN_FB   = 1000116000,

    /* XR_FB_foveation */
    XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB    = 1000114000,
    XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB  = 1000114001,
    XR_TYPE_SWAPCHAIN_STATE_FOVEATION_ENABLE_FB = 1000114002,

    /* XR_FB_passthrough */
    XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB    = 1000118000,
    XR_TYPE_PASSTHROUGH_CREATE_INFO_FB          = 1000118001,
    XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB    = 1000118002,
    XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB    = 1000118003,
    XR_TYPE_GEOMETRY_INSTANCE_CREATE_INFO_FB    = 1000118004,
    XR_TYPE_GEOMETRY_INSTANCE_TRANSFORM_FB      = 1000118005,
    XR_TYPE_SYSTEM_PASSTHROUGH_COLOR_LUT_PROPERTIES_FB = 1000266000,
    XR_TYPE_PASSTHROUGH_COLOR_LUT_CREATE_INFO_FB  = 1000266001,
    XR_TYPE_PASSTHROUGH_COLOR_LUT_UPDATE_INFO_FB  = 1000266002,
    XR_TYPE_PASSTHROUGH_COLOR_MAP_LUT_FB          = 1000266003,
    XR_TYPE_PASSTHROUGH_COLOR_MAP_INTERPOLATED_LUT_FB = 1000266004,
    XR_TYPE_PASSTHROUGH_BRIGHTNESS_CONTRAST_SATURATION_FB = 1000118009,

    /* XR_FB_spatial_entity */
    XR_TYPE_SYSTEM_SPATIAL_ENTITY_PROPERTIES_FB = 1000113004,
    XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_FB       = 1000113003,
    XR_TYPE_SPACE_COMPONENT_STATUS_SET_INFO_FB  = 1000113007,
    XR_TYPE_SPACE_COMPONENT_STATUS_FB           = 1000113001,
    XR_TYPE_EVENT_DATA_SPATIAL_ANCHOR_CREATE_COMPLETE_FB = 1000113005,
    XR_TYPE_EVENT_DATA_SPACE_SET_STATUS_COMPLETE_FB = 1000113006,

    /* XR_FB_scene */
    XR_TYPE_EXTENT_3D_F_FB                      = 1000417004,
    XR_TYPE_OFFSET_3D_F_FB                      = 1000417005,
    XR_TYPE_RECT_3D_F_FB                        = 1000417006,
    XR_TYPE_SEMANTIC_LABELS_FB                  = 1000175000,
    XR_TYPE_ROOM_LAYOUT_FB                      = 1000175001,
    XR_TYPE_BOUNDARY_2D_FB                      = 1000175002,

    /* XR_FB_hand_tracking_mesh */
    XR_TYPE_HAND_TRACKING_MESH_FB               = 1000110001,
    XR_TYPE_HAND_TRACKING_SCALE_FB              = 1000110003,

    /* XR_FB_face_tracking */
    XR_TYPE_SYSTEM_FACE_TRACKING_PROPERTIES_FB  = 1000201004,
    XR_TYPE_FACE_TRACKER_CREATE_INFO_FB         = 1000201002,
    XR_TYPE_FACE_EXPRESSION_INFO_FB             = 1000201003,
    XR_TYPE_FACE_EXPRESSION_WEIGHTS_FB          = 1000201006,

    /* XR_FB_body_tracking */
    XR_TYPE_SYSTEM_BODY_TRACKING_PROPERTIES_FB  = 1000076004,
    XR_TYPE_BODY_TRACKER_CREATE_INFO_FB         = 1000076001,
    XR_TYPE_BODY_JOINTS_LOCATE_INFO_FB          = 1000076002,
    XR_TYPE_BODY_JOINT_LOCATIONS_FB             = 1000076003,
    XR_TYPE_BODY_SKELETON_FB                    = 1000076005,

    /* XR_META_performance_metrics */
    XR_TYPE_PERFORMANCE_METRICS_STATE_META      = 1000232001,
    XR_TYPE_PERFORMANCE_METRICS_COUNTER_META    = 1000232002,

    /* XR_META_boundary_visibility */
    XR_TYPE_SYSTEM_BOUNDARY_VISIBILITY_PROPERTIES_META = 1000528001,

    /* XR_META_virtual_keyboard */
    XR_TYPE_SYSTEM_VIRTUAL_KEYBOARD_PROPERTIES_META = 1000219001,
    XR_TYPE_VIRTUAL_KEYBOARD_CREATE_INFO_META   = 1000219002,
    XR_TYPE_VIRTUAL_KEYBOARD_SPACE_CREATE_INFO_META = 1000219003,
    XR_TYPE_VIRTUAL_KEYBOARD_LOCATION_INFO_META = 1000219004,
    XR_TYPE_VIRTUAL_KEYBOARD_MODEL_VISIBILITY_SET_INFO_META = 1000219005,
    XR_TYPE_VIRTUAL_KEYBOARD_ANIMATION_STATE_META = 1000219006,
    XR_TYPE_VIRTUAL_KEYBOARD_MODEL_ANIMATION_STATES_META = 1000219007,
    XR_TYPE_VIRTUAL_KEYBOARD_TEXTURE_DATA_META  = 1000219009,
    XR_TYPE_VIRTUAL_KEYBOARD_INPUT_INFO_META    = 1000219010,
    XR_TYPE_VIRTUAL_KEYBOARD_TEXT_CONTEXT_CHANGE_INFO_META = 1000219011,
    XR_TYPE_EVENT_DATA_VIRTUAL_KEYBOARD_COMMIT_TEXT_META = 1000219014,
    XR_TYPE_EVENT_DATA_VIRTUAL_KEYBOARD_BACKSPACE_META = 1000219015,
    XR_TYPE_EVENT_DATA_VIRTUAL_KEYBOARD_ENTER_META = 1000219016,
    XR_TYPE_EVENT_DATA_VIRTUAL_KEYBOARD_SHOWN_META = 1000219017,
    XR_TYPE_EVENT_DATA_VIRTUAL_KEYBOARD_HIDDEN_META = 1000219018,
} XrStructureTypeMetaExt;

/* =========================================================================
 * XR_FB_display_refresh_rate
 * ========================================================================= */

typedef XrResult (XRAPI_PTR *PFN_xrEnumerateDisplayRefreshRatesFB)(
    XrSession session,
    uint32_t displayRefreshRateCapacityInput,
    uint32_t *displayRefreshRateCountOutput,
    float *displayRefreshRates);

typedef XrResult (XRAPI_PTR *PFN_xrGetDisplayRefreshRateFB)(
    XrSession session,
    float *displayRefreshRate);

typedef XrResult (XRAPI_PTR *PFN_xrRequestDisplayRefreshRateFB)(
    XrSession session,
    float displayRefreshRate);

/* =========================================================================
 * XR_FB_color_space
 * ========================================================================= */

typedef enum XrColorSpaceFB {
    XR_COLOR_SPACE_UNMANAGED_FB  = 0,
    XR_COLOR_SPACE_REC2020_FB    = 1,
    XR_COLOR_SPACE_REC709_FB     = 2,
    XR_COLOR_SPACE_RIFT_CV1_FB   = 3,
    XR_COLOR_SPACE_RIFT_S_FB     = 4,
    XR_COLOR_SPACE_QUEST_FB      = 5,
    XR_COLOR_SPACE_P3_FB         = 6,
    XR_COLOR_SPACE_ADOBE_RGB_FB  = 7,
    XR_COLOR_SPACE_MAX_ENUM_FB   = 0x7FFFFFFF
} XrColorSpaceFB;

typedef struct XrSystemColorSpacePropertiesFB {
    XrStructureType type; /* XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB */
    void *next;
    XrColorSpaceFB colorSpace;
} XrSystemColorSpacePropertiesFB;

typedef XrResult (XRAPI_PTR *PFN_xrSetColorSpaceFB)(
    XrSession session,
    XrColorSpaceFB colorSpace);

typedef XrResult (XRAPI_PTR *PFN_xrEnumerateColorSpacesFB)(
    XrSession session,
    uint32_t colorSpaceCapacityInput,
    uint32_t *colorSpaceCountOutput,
    XrColorSpaceFB *colorSpaces);

/* =========================================================================
 * XR_FB_foveation + XR_FB_foveation_configuration
 * ========================================================================= */

XR_DEFINE_HANDLE(XrFoveationProfileFB)

typedef enum XrFoveationLevelFB {
    XR_FOVEATION_LEVEL_NONE_FB    = 0,
    XR_FOVEATION_LEVEL_LOW_FB     = 1,
    XR_FOVEATION_LEVEL_MEDIUM_FB  = 2,
    XR_FOVEATION_LEVEL_HIGH_FB    = 3,
    XR_FOVEATION_LEVEL_MAX_ENUM_FB = 0x7FFFFFFF
} XrFoveationLevelFB;

typedef enum XrFoveationDynamicFB {
    XR_FOVEATION_DYNAMIC_DISABLED_FB         = 0,
    XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB    = 1,
    XR_FOVEATION_DYNAMIC_MAX_ENUM_FB         = 0x7FFFFFFF
} XrFoveationDynamicFB;

typedef struct XrFoveationProfileCreateInfoFB {
    XrStructureType type; /* XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB */
    void *next;
} XrFoveationProfileCreateInfoFB;

typedef struct XrSwapchainCreateInfoFoveationFB {
    XrStructureType type; /* XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB */
    void *next;
    XrSwapchainCreateFoveationFlagsFB flags;
} XrSwapchainCreateInfoFoveationFB;

typedef uint64_t XrSwapchainCreateFoveationFlagsFB;

typedef struct XrFoveationLevelProfileCreateInfoFB {
    XrStructureType type;
    void *next;
    XrFoveationLevelFB level;
    float verticalOffset;
    XrFoveationDynamicFB dynamic;
} XrFoveationLevelProfileCreateInfoFB;

typedef XrResult (XRAPI_PTR *PFN_xrCreateFoveationProfileFB)(
    XrSession session,
    const XrFoveationProfileCreateInfoFB *createInfo,
    XrFoveationProfileFB *profile);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyFoveationProfileFB)(
    XrFoveationProfileFB profile);

/* =========================================================================
 * XR_FB_passthrough
 * ========================================================================= */

XR_DEFINE_HANDLE(XrPassthroughFB)
XR_DEFINE_HANDLE(XrPassthroughLayerFB)
XR_DEFINE_HANDLE(XrGeometryInstanceFB)

typedef enum XrPassthroughLayerPurposeFB {
    XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB    = 0,
    XR_PASSTHROUGH_LAYER_PURPOSE_PROJECTED_FB         = 1,
    XR_PASSTHROUGH_LAYER_PURPOSE_TRACKED_KEYBOARD_HANDS_FB = 1000203001,
    XR_PASSTHROUGH_LAYER_PURPOSE_MAX_ENUM_FB          = 0x7FFFFFFF
} XrPassthroughLayerPurposeFB;

typedef uint64_t XrPassthroughFlagsFB;
typedef uint64_t XrPassthroughStatePassthroughFB;

typedef struct XrSystemPassthroughPropertiesFB {
    XrStructureType type;
    void *next;
    XrBool32 supportsPassthrough;
} XrSystemPassthroughPropertiesFB;

typedef struct XrPassthroughCreateInfoFB {
    XrStructureType type;
    const void *next;
    XrPassthroughFlagsFB flags;
} XrPassthroughCreateInfoFB;

typedef struct XrPassthroughLayerCreateInfoFB {
    XrStructureType type;
    const void *next;
    XrPassthroughFB passthrough;
    XrPassthroughFlagsFB flags;
    XrPassthroughLayerPurposeFB purpose;
} XrPassthroughLayerCreateInfoFB;

typedef struct XrCompositionLayerPassthroughFB {
    XrStructureType type;
    const void *next;
    XrCompositionLayerFlags flags;
    XrSpace space;
    XrPassthroughLayerFB layerHandle;
} XrCompositionLayerPassthroughFB;

typedef XrResult (XRAPI_PTR *PFN_xrCreatePassthroughFB)(
    XrSession session,
    const XrPassthroughCreateInfoFB *createInfo,
    XrPassthroughFB *outPassthrough);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyPassthroughFB)(
    XrPassthroughFB passthrough);

typedef XrResult (XRAPI_PTR *PFN_xrPassthroughStartFB)(
    XrPassthroughFB passthrough);

typedef XrResult (XRAPI_PTR *PFN_xrPassthroughPauseFB)(
    XrPassthroughFB passthrough);

typedef XrResult (XRAPI_PTR *PFN_xrCreatePassthroughLayerFB)(
    XrSession session,
    const XrPassthroughLayerCreateInfoFB *createInfo,
    XrPassthroughLayerFB *outLayer);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyPassthroughLayerFB)(
    XrPassthroughLayerFB layer);

typedef XrResult (XRAPI_PTR *PFN_xrPassthroughLayerPauseFB)(
    XrPassthroughLayerFB layer);

typedef XrResult (XRAPI_PTR *PFN_xrPassthroughLayerResumeFB)(
    XrPassthroughLayerFB layer);

typedef XrResult (XRAPI_PTR *PFN_xrPassthroughLayerSetStyleFB)(
    XrPassthroughLayerFB layer,
    const void *style); /* XrPassthroughStyleFB* */

/* =========================================================================
 * XR_FB_spatial_entity (Anchors, Scene Understanding)
 * ========================================================================= */

XR_DEFINE_ATOM(XrAsyncRequestIdFB)

typedef enum XrSpaceComponentTypeFB {
    XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB       = 0,
    XR_SPACE_COMPONENT_TYPE_STORABLE_FB        = 1,
    XR_SPACE_COMPONENT_TYPE_SHARABLE_FB        = 2,
    XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB      = 3,
    XR_SPACE_COMPONENT_TYPE_BOUNDED_3D_FB      = 4,
    XR_SPACE_COMPONENT_TYPE_SEMANTIC_LABELS_FB = 5,
    XR_SPACE_COMPONENT_TYPE_ROOM_LAYOUT_FB     = 6,
    XR_SPACE_COMPONENT_TYPE_SPACE_CONTAINER_FB = 7,
    XR_SPACE_COMPONENT_TYPE_MAX_ENUM_FB        = 0x7FFFFFFF
} XrSpaceComponentTypeFB;

typedef struct XrSystemSpatialEntityPropertiesFB {
    XrStructureType type;
    const void *next;
    XrBool32 supportsSpatialEntity;
} XrSystemSpatialEntityPropertiesFB;

typedef struct XrSpatialAnchorCreateInfoFB {
    XrStructureType type;
    const void *next;
    XrSpace space;
    XrPosef poseInSpace;
    XrTime time;
} XrSpatialAnchorCreateInfoFB;

typedef struct XrSpaceComponentStatusSetInfoFB {
    XrStructureType type;
    const void *next;
    XrSpaceComponentTypeFB componentType;
    XrBool32 enabled;
    XrDuration timeout;
} XrSpaceComponentStatusSetInfoFB;

typedef struct XrSpaceComponentStatusFB {
    XrStructureType type;
    void *next;
    XrBool32 enabled;
    XrBool32 changePending;
} XrSpaceComponentStatusFB;

typedef XrResult (XRAPI_PTR *PFN_xrCreateSpatialAnchorFB)(
    XrSession session,
    const XrSpatialAnchorCreateInfoFB *info,
    XrAsyncRequestIdFB *requestId);

typedef XrResult (XRAPI_PTR *PFN_xrGetSpaceUuidFB)(
    XrSpace space,
    XrUuidEXT *uuid);

typedef XrResult (XRAPI_PTR *PFN_xrEnumerateSpaceSupportedComponentsFB)(
    XrSpace space,
    uint32_t componentTypeCapacityInput,
    uint32_t *componentTypeCountOutput,
    XrSpaceComponentTypeFB *componentTypes);

typedef XrResult (XRAPI_PTR *PFN_xrSetSpaceComponentStatusFB)(
    XrSpace space,
    const XrSpaceComponentStatusSetInfoFB *info,
    XrAsyncRequestIdFB *requestId);

typedef XrResult (XRAPI_PTR *PFN_xrGetSpaceComponentStatusFB)(
    XrSpace space,
    XrSpaceComponentTypeFB componentType,
    XrSpaceComponentStatusFB *status);

/* =========================================================================
 * XR_META_performance_metrics
 * ========================================================================= */

typedef enum XrPerformanceMetricsCounterUnitMETA {
    XR_PERFORMANCE_METRICS_COUNTER_UNIT_GENERIC_META       = 0,
    XR_PERFORMANCE_METRICS_COUNTER_UNIT_PERCENTAGE_META    = 1,
    XR_PERFORMANCE_METRICS_COUNTER_UNIT_MILLISECONDS_META  = 2,
    XR_PERFORMANCE_METRICS_COUNTER_UNIT_BYTES_META         = 3,
    XR_PERFORMANCE_METRICS_COUNTER_UNIT_BYTES_PER_SECOND_META = 4,
    XR_PERFORMANCE_METRICS_COUNTER_UNIT_HERTZ_META         = 5,
    XR_PERFORMANCE_METRICS_COUNTER_UNIT_MAX_ENUM_META      = 0x7FFFFFFF
} XrPerformanceMetricsCounterUnitMETA;

typedef uint64_t XrPerformanceMetricsCounterFlagsMETA;

typedef struct XrPerformanceMetricsStateMETA {
    XrStructureType type;
    void *next;
    XrBool32 enabled;
} XrPerformanceMetricsStateMETA;

typedef struct XrPerformanceMetricsCounterMETA {
    XrStructureType type;
    void *next;
    XrPerformanceMetricsCounterFlagsMETA counterFlags;
    XrPerformanceMetricsCounterUnitMETA counterUnit;
    uint32_t uintValue;
    float floatValue;
} XrPerformanceMetricsCounterMETA;

typedef XrResult (XRAPI_PTR *PFN_xrEnumeratePerformanceMetricsCounterPathsMETA)(
    XrInstance instance,
    uint32_t counterPathCapacityInput,
    uint32_t *counterPathCountOutput,
    XrPath *counterPaths);

typedef XrResult (XRAPI_PTR *PFN_xrSetPerformanceMetricsStateMETA)(
    XrSession session,
    const XrPerformanceMetricsStateMETA *state);

typedef XrResult (XRAPI_PTR *PFN_xrGetPerformanceMetricsStateMETA)(
    XrSession session,
    XrPerformanceMetricsStateMETA *state);

typedef XrResult (XRAPI_PTR *PFN_xrQueryPerformanceMetricsCounterMETA)(
    XrSession session,
    XrPath counterPath,
    XrPerformanceMetricsCounterMETA *counter);

/* =========================================================================
 * XR_META_boundary_visibility
 * ========================================================================= */

typedef enum XrBoundaryVisibilityMETA {
    XR_BOUNDARY_VISIBILITY_NOT_SUPPRESSED_META = 1,
    XR_BOUNDARY_VISIBILITY_SUPPRESSED_META     = 2,
    XR_BOUNDARY_VISIBILITY_MAX_ENUM_META       = 0x7FFFFFFF
} XrBoundaryVisibilityMETA;

typedef struct XrSystemBoundaryVisibilityPropertiesMETA {
    XrStructureType type;
    const void *next;
    XrBool32 supportsBoundaryVisibility;
} XrSystemBoundaryVisibilityPropertiesMETA;

typedef XrResult (XRAPI_PTR *PFN_xrRequestBoundaryVisibilityMETA)(
    XrSession session,
    XrBoundaryVisibilityMETA boundaryVisibility);

/* =========================================================================
 * XR_FB_hand_tracking_mesh
 * ========================================================================= */

#define XR_HAND_TRACKING_MESH_JOINTS_FB 26

typedef struct XrHandTrackingMeshFB {
    XrStructureType type;
    void *next;
    uint32_t jointCapacityInput;
    uint32_t jointCountOutput;
    XrPosef *jointBindPoses;
    float *jointRadii;
    XrHandJointEXT *jointParents;
    uint32_t vertexCapacityInput;
    uint32_t vertexCountOutput;
    XrVector3f *vertexPositions;
    XrVector3f *vertexNormals;
    XrVector2f *vertexUVs;
    XrVector4sFB *vertexBlendIndices;
    XrVector4f *vertexBlendWeights;
    uint32_t indexCapacityInput;
    uint32_t indexCountOutput;
    int16_t *indices;
} XrHandTrackingMeshFB;

typedef struct XrHandTrackingScaleFB {
    XrStructureType type;
    void *next;
    float sensorOutput;
    float currentOutput;
    XrBool32 overrideHandScale;
    float overrideValueInput;
} XrHandTrackingScaleFB;

typedef XrResult (XRAPI_PTR *PFN_xrGetHandMeshFB)(
    XrHandTrackerEXT handTracker,
    XrHandTrackingMeshFB *mesh);

/* =========================================================================
 * XR_FB_face_tracking
 * ========================================================================= */

#define XR_FACE_EXPRESSION_COUNT_FB 63
#define XR_FACE_CONFIDENCE_COUNT_FB  2

typedef XrResult (XRAPI_PTR *PFN_xrCreateFaceTrackerFB)(
    XrSession session,
    const void *createInfo,
    XrFaceTrackerFB *faceTracker);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyFaceTrackerFB)(
    XrFaceTrackerFB faceTracker);

typedef XrResult (XRAPI_PTR *PFN_xrGetFaceExpressionWeightsFB)(
    XrFaceTrackerFB faceTracker,
    const void *expressionInfo,
    void *expressionWeights);

XR_DEFINE_HANDLE(XrFaceTrackerFB)

/* =========================================================================
 * XR_FB_body_tracking
 * ========================================================================= */

#define XR_BODY_JOINT_COUNT_FB 70

XR_DEFINE_HANDLE(XrBodyTrackerFB)

typedef XrResult (XRAPI_PTR *PFN_xrCreateBodyTrackerFB)(
    XrSession session,
    const void *createInfo,
    XrBodyTrackerFB *bodyTracker);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyBodyTrackerFB)(
    XrBodyTrackerFB bodyTracker);

typedef XrResult (XRAPI_PTR *PFN_xrLocateBodyJointsFB)(
    XrBodyTrackerFB bodyTracker,
    const void *locateInfo,
    void *locations);

typedef XrResult (XRAPI_PTR *PFN_xrGetBodySkeletonFB)(
    XrBodyTrackerFB bodyTracker,
    void *skeleton);

/* =========================================================================
 * XR_META_virtual_keyboard
 * ========================================================================= */

XR_DEFINE_HANDLE(XrVirtualKeyboardMETA)

typedef XrResult (XRAPI_PTR *PFN_xrCreateVirtualKeyboardMETA)(
    XrSession session,
    const XrVirtualKeyboardCreateInfoMETA *createInfo,
    XrVirtualKeyboardMETA *keyboard);

typedef XrResult (XRAPI_PTR *PFN_xrDestroyVirtualKeyboardMETA)(
    XrVirtualKeyboardMETA keyboard);

typedef struct XrVirtualKeyboardCreateInfoMETA {
    XrStructureType type;
    const void *next;
} XrVirtualKeyboardCreateInfoMETA;

typedef XrResult (XRAPI_PTR *PFN_xrSuggestVirtualKeyboardLocationMETA)(
    XrVirtualKeyboardMETA keyboard,
    const void *locationInfo);

typedef XrResult (XRAPI_PTR *PFN_xrGetVirtualKeyboardScaleMETA)(
    XrVirtualKeyboardMETA keyboard,
    float *scale);

typedef XrResult (XRAPI_PTR *PFN_xrSetVirtualKeyboardModelVisibilityMETA)(
    XrVirtualKeyboardMETA keyboard,
    const void *modelVisibility);

typedef XrResult (XRAPI_PTR *PFN_xrGetVirtualKeyboardModelAnimationStatesMETA)(
    XrVirtualKeyboardMETA keyboard,
    void *animationStates);

typedef XrResult (XRAPI_PTR *PFN_xrGetVirtualKeyboardDirtyTexturesMETA)(
    XrVirtualKeyboardMETA keyboard,
    uint32_t textureIdCapacityInput,
    uint32_t *textureIdCountOutput,
    uint64_t *textureIds);

typedef XrResult (XRAPI_PTR *PFN_xrGetVirtualKeyboardTextureDataMETA)(
    XrVirtualKeyboardMETA keyboard,
    uint64_t textureId,
    void *textureData);

typedef XrResult (XRAPI_PTR *PFN_xrSendVirtualKeyboardInputMETA)(
    XrVirtualKeyboardMETA keyboard,
    const void *info,
    XrPosef *interactorRootPose);

typedef XrResult (XRAPI_PTR *PFN_xrChangeVirtualKeyboardTextContextMETA)(
    XrVirtualKeyboardMETA keyboard,
    const void *changeInfo);

/* =========================================================================
 * Extension dispatch table (all FB/Meta extensions in one place)
 * ========================================================================= */

typedef struct XrMetaExtensionDispatchTable {
    /* XR_FB_display_refresh_rate */
    PFN_xrEnumerateDisplayRefreshRatesFB    EnumerateDisplayRefreshRatesFB;
    PFN_xrGetDisplayRefreshRateFB           GetDisplayRefreshRateFB;
    PFN_xrRequestDisplayRefreshRateFB       RequestDisplayRefreshRateFB;

    /* XR_FB_color_space */
    PFN_xrEnumerateColorSpacesFB            EnumerateColorSpacesFB;
    PFN_xrSetColorSpaceFB                   SetColorSpaceFB;

    /* XR_FB_foveation */
    PFN_xrCreateFoveationProfileFB          CreateFoveationProfileFB;
    PFN_xrDestroyFoveationProfileFB         DestroyFoveationProfileFB;

    /* XR_FB_passthrough */
    PFN_xrCreatePassthroughFB               CreatePassthroughFB;
    PFN_xrDestroyPassthroughFB              DestroyPassthroughFB;
    PFN_xrPassthroughStartFB                PassthroughStartFB;
    PFN_xrPassthroughPauseFB                PassthroughPauseFB;
    PFN_xrCreatePassthroughLayerFB          CreatePassthroughLayerFB;
    PFN_xrDestroyPassthroughLayerFB         DestroyPassthroughLayerFB;
    PFN_xrPassthroughLayerPauseFB           PassthroughLayerPauseFB;
    PFN_xrPassthroughLayerResumeFB          PassthroughLayerResumeFB;
    PFN_xrPassthroughLayerSetStyleFB        PassthroughLayerSetStyleFB;

    /* XR_FB_spatial_entity */
    PFN_xrCreateSpatialAnchorFB             CreateSpatialAnchorFB;
    PFN_xrGetSpaceUuidFB                    GetSpaceUuidFB;
    PFN_xrEnumerateSpaceSupportedComponentsFB EnumerateSpaceSupportedComponentsFB;
    PFN_xrSetSpaceComponentStatusFB         SetSpaceComponentStatusFB;
    PFN_xrGetSpaceComponentStatusFB         GetSpaceComponentStatusFB;

    /* XR_FB_hand_tracking_mesh */
    PFN_xrGetHandMeshFB                     GetHandMeshFB;

    /* XR_FB_face_tracking */
    PFN_xrCreateFaceTrackerFB               CreateFaceTrackerFB;
    PFN_xrDestroyFaceTrackerFB              DestroyFaceTrackerFB;
    PFN_xrGetFaceExpressionWeightsFB        GetFaceExpressionWeightsFB;

    /* XR_FB_body_tracking */
    PFN_xrCreateBodyTrackerFB               CreateBodyTrackerFB;
    PFN_xrDestroyBodyTrackerFB              DestroyBodyTrackerFB;
    PFN_xrLocateBodyJointsFB                LocateBodyJointsFB;
    PFN_xrGetBodySkeletonFB                 GetBodySkeletonFB;

    /* XR_META_performance_metrics */
    PFN_xrEnumeratePerformanceMetricsCounterPathsMETA EnumeratePerformanceMetricsCounterPathsMETA;
    PFN_xrSetPerformanceMetricsStateMETA    SetPerformanceMetricsStateMETA;
    PFN_xrGetPerformanceMetricsStateMETA    GetPerformanceMetricsStateMETA;
    PFN_xrQueryPerformanceMetricsCounterMETA QueryPerformanceMetricsCounterMETA;

    /* XR_META_boundary_visibility */
    PFN_xrRequestBoundaryVisibilityMETA     RequestBoundaryVisibilityMETA;

    /* XR_META_virtual_keyboard */
    PFN_xrCreateVirtualKeyboardMETA         CreateVirtualKeyboardMETA;
    PFN_xrDestroyVirtualKeyboardMETA        DestroyVirtualKeyboardMETA;
    PFN_xrSuggestVirtualKeyboardLocationMETA SuggestVirtualKeyboardLocationMETA;
    PFN_xrGetVirtualKeyboardScaleMETA       GetVirtualKeyboardScaleMETA;
    PFN_xrSetVirtualKeyboardModelVisibilityMETA SetVirtualKeyboardModelVisibilityMETA;
    PFN_xrGetVirtualKeyboardModelAnimationStatesMETA GetVirtualKeyboardModelAnimationStatesMETA;
    PFN_xrGetVirtualKeyboardDirtyTexturesMETA GetVirtualKeyboardDirtyTexturesMETA;
    PFN_xrGetVirtualKeyboardTextureDataMETA GetVirtualKeyboardTextureDataMETA;
    PFN_xrSendVirtualKeyboardInputMETA      SendVirtualKeyboardInputMETA;
    PFN_xrChangeVirtualKeyboardTextContextMETA ChangeVirtualKeyboardTextContextMETA;
} XrMetaExtensionDispatchTable;

/* Populate dispatch table by querying xrGetInstanceProcAddr */
XrResult xrMetaExtLoadDispatchTable(
    XrInstance instance,
    XrMetaExtensionDispatchTable *table);

#ifdef __cplusplus
}
#endif
