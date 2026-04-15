/*
 * ipc_server.h — Waydroid XR IPC Server
 *
 * Host-side Unix socket server that receives frame data and OpenXR commands
 * from the Waydroid container (guest) and forwards them to the host OpenXR runtime.
 *
 * The server listens on /run/waydroid-xr/host.sock and handles:
 *   • Capability negotiation (QUERY_CAPS)
 *   • Display refresh rate queries/requests (enumeration, get, set)
 *   • Hand tracking mesh requests
 *   • Eye buffer frame submissions with DMA-BUF file descriptors
 *
 * License: Apache 2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IpcServer IpcServer;
typedef struct IpcClientConnection IpcClientConnection;

/* =========================================================================
 * Server lifecycle
 * ========================================================================= */

/*
 * ipc_server_create — Initialize and bind the IPC server socket.
 * Returns NULL on error (binding failed, etc.).
 * Socket is not listening yet; call ipc_server_listen() to start accepting.
 */
IpcServer *ipc_server_create(void);

/*
 * ipc_server_destroy — Clean up and unbind the socket.
 */
void ipc_server_destroy(IpcServer *server);

/*
 * ipc_server_listen — Start accepting connections.
 * Blocks the current thread; typically called in its own thread.
 * Returns on unrecoverable error or when signaled to stop.
 */
void ipc_server_listen(IpcServer *server);

/*
 * ipc_server_stop — Request the listener thread to exit and close all connections.
 */
void ipc_server_stop(IpcServer *server);

/* =========================================================================
 * Frame buffer handling (called from message handlers)
 * ========================================================================= */

/*
 * ipc_server_submit_frame — Accept an eye buffer frame with DMA-BUF FD.
 *   metadata_left:  Left eye buffer metadata (may be NULL)
 *   metadata_right: Right eye buffer metadata (may be NULL)
 *   dma_buf_fd_left:  DMA-BUF file descriptor for left eye (or -1)
 *   dma_buf_fd_right: DMA-BUF file descriptor for right eye (or -1)
 *
 * This function transfers ownership of the file descriptors.
 * Returns 0 on success, negative on error.
 */
typedef struct {
    uint32_t    eye_index;
    uint32_t    width;
    uint32_t    height;
    uint32_t    format;
    uint32_t    size_bytes;
    int64_t     frame_id;
    int64_t     predicted_display_time;
    uint32_t    stride;
} FrameMetadata;

int ipc_server_submit_frame(IpcServer *server,
                           const FrameMetadata *metadata_left,
                           const FrameMetadata *metadata_right,
                           int dma_buf_fd_left,
                           int dma_buf_fd_right);

#ifdef __cplusplus
}
#endif
