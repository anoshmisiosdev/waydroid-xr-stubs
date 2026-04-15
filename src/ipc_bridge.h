/*
 * ipc_bridge.h — Waydroid XR IPC Bridge
 *
 * Thin socket-based bridge from inside the Waydroid Android container
 * to the host Linux OpenXR runtime (Monado / SteamVR).
 *
 * Architecture:
 *
 *   ┌─────────────────────────────┐
 *   │  Android container (guest)  │
 *   │   Quest APK                 │
 *   │   → meta_ext_stubs.so       │
 *   │   → ipc_bridge.so    ───────┼──── Unix socket (/run/waydroid-xr.sock)
 *   └─────────────────────────────┘     │
 *                                       ▼
 *   ┌─────────────────────────────┐
 *   │   Host Linux                │
 *   │   waydroid-xr-server        │
 *   │   → Monado OpenXR runtime   │
 *   │   → SteamVR (via OpenVR)    │
 *   └─────────────────────────────┘
 *
 * The socket path is visible from inside Waydroid because Waydroid
 * shares the host network namespace and can bind-mount /run.
 *
 * Protocol: length-prefixed msgpack messages (or simple C structs for MVP)
 *
 * License: Apache 2.0
 */

#pragma once

#include <openxr/openxr.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Path inside the container where the host socket is visible */
#define IPC_BRIDGE_SOCKET_PATH "/run/waydroid-xr/host.sock"

/* =========================================================================
 * Connection lifecycle
 * ========================================================================= */

/*
 * ipc_bridge_init — Connect to the host waydroid-xr-server.
 * Should be called from the OpenXR layer's xrCreateInstance hook.
 * Returns true if connected, false if no host server is running.
 * If false, all forwarding calls return stub values.
 */
bool ipc_bridge_init(void);

/*
 * ipc_bridge_shutdown — Disconnect cleanly.
 * Called from xrDestroyInstance hook.
 */
void ipc_bridge_shutdown(void);

/*
 * ipc_bridge_is_connected — Query connection state.
 */
bool ipc_bridge_is_connected(void);

/*
 * ipc_bridge_supports_hand_mesh — Query host capability flag.
 */
bool ipc_bridge_supports_hand_mesh(void);

/* =========================================================================
 * Forwarded OpenXR calls (display / rendering path)
 * ========================================================================= */

XrResult ipc_bridge_EnumerateDisplayRefreshRates(
    XrSession session,
    uint32_t capacityInput,
    uint32_t *countOutput,
    float *refreshRates);

XrResult ipc_bridge_GetDisplayRefreshRate(
    XrSession session,
    float *refreshRate);

XrResult ipc_bridge_RequestDisplayRefreshRate(
    XrSession session,
    float refreshRate);

/* =========================================================================
 * Forwarded OpenXR calls (hand tracking path)
 * ========================================================================= */

XrResult ipc_bridge_GetHandMesh(
    XrHandTrackerEXT handTracker,
    void *mesh); /* XrHandTrackingMeshFB* */

/* =========================================================================
 * IPC message types (internal protocol)
 * ========================================================================= */

typedef enum WaydroidXrMsgType {
    WXRMSG_PING                          = 0x01,
    WXRMSG_PONG                          = 0x02,
    WXRMSG_QUERY_CAPS                    = 0x10,
    WXRMSG_CAPS_RESPONSE                 = 0x11,
    WXRMSG_ENUMERATE_REFRESH_RATES       = 0x20,
    WXRMSG_ENUMERATE_REFRESH_RATES_RESP  = 0x21,
    WXRMSG_GET_REFRESH_RATE              = 0x22,
    WXRMSG_GET_REFRESH_RATE_RESP         = 0x23,
    WXRMSG_REQUEST_REFRESH_RATE          = 0x24,
    WXRMSG_REQUEST_REFRESH_RATE_RESP     = 0x25,
    WXRMSG_GET_HAND_MESH                 = 0x30,
    WXRMSG_GET_HAND_MESH_RESP            = 0x31,
    WXRMSG_SWAPCHAIN_FRAME_BEGIN         = 0x40,
    WXRMSG_SWAPCHAIN_FRAME_END           = 0x41,
    WXRMSG_SWAPCHAIN_FRAME_RESP          = 0x42,
} WaydroidXrMsgType;

typedef struct WaydroidXrMsgHeader {
    uint32_t magic;    /* 0x57585200 "WXR\0" */
    uint32_t type;     /* WaydroidXrMsgType */
    uint32_t length;   /* payload bytes following this header */
    uint32_t seq;      /* monotonic sequence number for matching responses */
} WaydroidXrMsgHeader;

#define WXRMSG_MAGIC 0x57585200u

typedef struct WaydroidXrCapsResponse {
    uint32_t version;
    uint32_t supports_hand_mesh    : 1;
    uint32_t supports_eye_tracking : 1;
    uint32_t supports_passthrough  : 1;
    uint32_t reserved              : 29;
    float    max_refresh_rate;
    float    native_refresh_rate;
} WaydroidXrCapsResponse;

typedef struct WaydroidXrRefreshRateResp {
    uint32_t count;
    float    rates[16]; /* up to 16 supported rates */
} WaydroidXrRefreshRateResp;

/* =========================================================================
 * Frame buffer protocol — DMA-BUF based eye buffer transport
 * ========================================================================= */

/*
 * WaydroidXrFrameMetadata sent by container in SWAPCHAIN_FRAME_END
 * This describes one submitted eye buffer. The actual image data is in
 * a DMA-BUF with FD passed separately via Unix socket ancillary data (cmsg).
 */
typedef struct WaydroidXrFrameMetadata {
    uint32_t    eye_index;      /* 0=left, 1=right */
    uint32_t    width;
    uint32_t    height;
    uint32_t    format;         /* VkFormat or XrSwapchainFormat enum */
    uint32_t    size_bytes;     /* total image size in bytes */
    int64_t     frame_id;       /* monotonic frame counter */
    int64_t     predicted_display_time;  /* in nanoseconds */
    uint32_t    stride;         /* bytes per row (may differ from width * bpp) */
    uint32_t    reserved[4];
} WaydroidXrFrameMetadata;

/*
 * WaydroidXrFrameSubmit wraps metadata for a single eye buffer submission.
 * The DMA-BUF file descriptor is passed as a Unix socket SCM_RIGHTS ancillary
 * message immediately after this struct.
 *
 * Serialized format in socket message:
 *   [WaydroidXrMsgHeader: type=WXRMSG_SWAPCHAIN_FRAME_END]
 *   [WaydroidXrFrameSubmit]
 *   [... possibly repeated for second eye ...]
 *   [ancillary data: SCM_RIGHTS with DMA-BUF FDs]
 */
typedef struct WaydroidXrFrameSubmit {
    uint32_t            num_metadata;   /* typically 2 (left, right) */
    WaydroidXrFrameMetadata metadata[2];
    int64_t             swapchain_handle; /* opaque handle from container */
} WaydroidXrFrameSubmit;

/*
 * Host-side response to frame submission.
 * Signals readiness for the next frame and any error status.
 */
typedef struct WaydroidXrFrameSubmitResp {
    int32_t error;  /* 0 = success, else OpenXR XrResult code */
    uint32_t reserved[3];
} WaydroidXrFrameSubmitResp;

#ifdef __cplusplus
}
#endif
