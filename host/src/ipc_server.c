/*
 * ipc_server.c — Waydroid XR IPC Server Implementation
 *
 * Implements a Unix socket server that listens for connections from
 * Waydroid container OpenXR layers and routes messages to OpenXR session.
 *
 * License: Apache 2.0
 */

#include "ipc_server.h"
#include "openxr_host.h"
#include "dma_buf_import.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

/* =========================================================================
 * Logging
 * ========================================================================= */

#define LOG(level, fmt, ...) \
    fprintf(stderr, "[waydroid-xr-server] " level ": " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)

/* =========================================================================
 * IPC Protocol Constants
 * ========================================================================= */

#define IPC_SOCKET_PATH "/run/waydroid-xr/host.sock"
#define WXRMSG_MAGIC 0x57585200u

typedef enum {
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

typedef struct {
    uint32_t magic;
    uint32_t type;
    uint32_t length;
    uint32_t seq;
} WaydroidXrMsgHeader;

typedef struct {
    uint32_t version;
    uint32_t supports_hand_mesh    : 1;
    uint32_t supports_eye_tracking : 1;
    uint32_t supports_passthrough  : 1;
    uint32_t reserved              : 29;
    float    max_refresh_rate;
    float    native_refresh_rate;
} WaydroidXrCapsResponse;

typedef struct {
    uint32_t count;
    float    rates[16];
} WaydroidXrRefreshRateResp;

typedef struct {
    uint32_t    eye_index;
    uint32_t    width;
    uint32_t    height;
    uint32_t    format;
    uint32_t    size_bytes;
    int64_t     frame_id;
    int64_t     predicted_display_time;
    uint32_t    stride;
    uint32_t    reserved[4];
} WaydroidXrFrameMetadata;

typedef struct {
    uint32_t            num_metadata;
    WaydroidXrFrameMetadata metadata[2];
    int64_t             swapchain_handle;
} WaydroidXrFrameSubmit;

typedef struct {
    int32_t error;
    uint32_t reserved[3];
} WaydroidXrFrameSubmitResp;

/* =========================================================================
 * Client Connection State
 * ========================================================================= */

typedef struct {
    int   sd;           /* socket descriptor */
    bool  connected;
} ClientDesc;

/* =========================================================================
 * Server State
 * ========================================================================= */

struct IpcServer {
    int              listen_sd;
    volatile bool    should_stop;
    pthread_t        listen_thread;
    OxrHost         *oxr_host;
    
    /* For single container connection (MVP) */
    ClientDesc       client;
};

/* =========================================================================
 * Low-level socket I/O with ancillary data support
 * ========================================================================= */

static bool send_all(int sd, const void *buf, size_t len) {
    const uint8_t *ptr = (const uint8_t *)buf;
    while (len > 0) {
        ssize_t n = send(sd, ptr, len, MSG_NOSIGNAL);
        if (n < 0) {
            LOGE("send_all failed: %s", strerror(errno));
            return false;
        }
        ptr += n;
        len -= (size_t)n;
    }
    return true;
}

static bool recv_all(int sd, void *buf, size_t len) {
    uint8_t *ptr = (uint8_t *)buf;
    while (len > 0) {
        ssize_t n = recv(sd, ptr, len, MSG_WAITALL);
        if (n <= 0) {
            LOGE("recv_all failed: %s", strerror(errno));
            return false;
        }
        ptr += n;
        len -= (size_t)n;
    }
    return true;
}

/*
 * recv_msg_with_fds — Receive a message header + payload + ancillary FDs
 * out_fds: array to store received file descriptors
 * out_fd_count: on input, capacity; on output, number of FDs received
 */
static bool recv_msg_with_fds(int sd,
                             WaydroidXrMsgHeader *out_hdr,
                             void *payload_buf, uint32_t buf_size,
                             int *out_fds, uint32_t *out_fd_count)
{
    struct iovec iov;
    iov.iov_base = (char *)payload_buf;
    iov.iov_len = buf_size;

    uint8_t cmsg_buf[256];
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = cmsg_buf,
        .msg_controllen = sizeof(cmsg_buf),
    };

    /* Receive header first via regular recv */
    if (!recv_all(sd, out_hdr, sizeof(*out_hdr))) {
        return false;
    }

    if (out_hdr->magic != WXRMSG_MAGIC) {
        LOGE("recv_msg_with_fds: bad magic 0x%08x", out_hdr->magic);
        return false;
    }

    /* Receive payload and ancillary data */
    uint32_t payload_len = out_hdr->length;
    if (payload_len > 0) {
        iov.iov_len = (payload_len < buf_size) ? payload_len : buf_size;
        ssize_t n = recvmsg(sd, &msg, 0);
        if (n < 0) {
            LOGE("recvmsg failed: %s", strerror(errno));
            return false;
        }

        /* Drain any excess payload */
        if (payload_len > buf_size) {
            uint8_t drain[512];
            uint32_t remaining = payload_len - buf_size;
            while (remaining > 0) {
                uint32_t chunk = (remaining < sizeof(drain)) ? remaining : sizeof(drain);
                if (!recv_all(sd, drain, chunk)) return false;
                remaining -= chunk;
            }
        }
    }

    /* Extract FDs from ancillary data */
    uint32_t fd_count = 0;
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
         cmsg && fd_count < *out_fd_count;
         cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int *fds = (int *)CMSG_DATA(cmsg);
            size_t fd_space = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
            for (size_t i = 0; i < fd_space && fd_count < *out_fd_count; i++) {
                out_fds[fd_count++] = fds[i];
            }
        }
    }

    *out_fd_count = fd_count;
    return true;
}

static bool send_msg(int sd, WaydroidXrMsgType type,
                    const void *payload, uint32_t payload_len)
{
    WaydroidXrMsgHeader hdr = {
        .magic = WXRMSG_MAGIC,
        .type = (uint32_t)type,
        .length = payload_len,
        .seq = 0,
    };

    if (!send_all(sd, &hdr, sizeof(hdr))) return false;
    if (payload_len > 0 && payload) {
        if (!send_all(sd, payload, payload_len)) return false;
    }
    return true;
}

/* =========================================================================
 * Message handler implementations
 * ========================================================================= */

static bool handle_ping(IpcServer *srv, int sd) {
    LOGI("Received PING from client");
    return send_msg(sd, WXRMSG_PONG, NULL, 0);
}

static bool handle_query_caps(IpcServer *srv, int sd) {
    WaydroidXrCapsResponse caps = {
        .version = 1,
        .supports_hand_mesh = 0,
        .supports_eye_tracking = 0,
        .supports_passthrough = 0,
        .max_refresh_rate = 120.0f,
        .native_refresh_rate = 90.0f,
    };

    /* Query actual capabilities from host runtime */
    float rates[16];
    uint32_t count = 16;
    if (oxr_host_get_display_refresh_rates(srv->oxr_host, rates, &count)) {
        if (count > 0) {
            caps.native_refresh_rate = rates[count - 1];
            caps.max_refresh_rate = rates[0];
            for (uint32_t i = 0; i < count; i++) {
                if (rates[i] > caps.max_refresh_rate)
                    caps.max_refresh_rate = rates[i];
            }
        }
    }

    return send_msg(sd, WXRMSG_CAPS_RESPONSE, &caps, sizeof(caps));
}

static bool handle_enumerate_refresh_rates(IpcServer *srv, int sd) {
    float rates[16];
    uint32_t count = 16;
    
    if (!oxr_host_get_display_refresh_rates(srv->oxr_host, rates, &count)) {
        LOGW("Failed to enumerate refresh rates from host runtime");
        count = 0;
    }

    WaydroidXrRefreshRateResp resp = {
        .count = count,
    };
    memcpy(resp.rates, rates, count * sizeof(float));

    return send_msg(sd, WXRMSG_ENUMERATE_REFRESH_RATES_RESP, &resp,
                   sizeof(resp.count) + count * sizeof(float));
}

static bool handle_get_refresh_rate(IpcServer *srv, int sd) {
    float rate = 0.0f;
    if (!oxr_host_get_display_refresh_rate(srv->oxr_host, &rate)) {
        LOGW("Failed to get refresh rate from host runtime");
    }

    return send_msg(sd, WXRMSG_GET_REFRESH_RATE_RESP, &rate, sizeof(rate));
}

static bool handle_request_refresh_rate(IpcServer *srv, int sd,
                                       const void *payload, uint32_t len)
{
    if (len < sizeof(float)) {
        LOGW("Invalid WXRMSG_REQUEST_REFRESH_RATE payload");
        return false;
    }

    float rate = *(const float *)payload;
    int32_t error = oxr_host_request_display_refresh_rate(srv->oxr_host, rate)
                  ? 0 : -1;

    return send_msg(sd, WXRMSG_REQUEST_REFRESH_RATE_RESP, &error, sizeof(error));
}

static bool handle_swapchain_frame_end(IpcServer *srv, int sd,
                                       const void *payload, uint32_t len)
{
    if (len < sizeof(WaydroidXrFrameSubmit)) {
        LOGW("Invalid WXRMSG_SWAPCHAIN_FRAME_END payload");
        return false;
    }

    const WaydroidXrFrameSubmit *frame = (const WaydroidXrFrameSubmit *)payload;

    /* Receive the DMA-BUF file descriptors via ancillary data */
    int dma_fds[2] = {-1, -1};
    uint32_t fd_count = 2;

    /* Note: This is a limitation — we need to refactor recv to get FDs.
     * For now, this is a placeholder. In production, you'd integrate
     * recv_msg_with_fds above into the main message loop. */

    LOGI("Frame submission: num_metadata=%u, frame_id=%ld",
         frame->num_metadata, frame->metadata[0].frame_id);

    /* Submit to OpenXR host for composition */
    if (frame->num_metadata >= 1) {
        OxrLayerParams layer = {
            .texture_handle = NULL,  /* Would be set from imported GPU handle */
            .width = frame->metadata[0].width,
            .height = frame->metadata[0].height,
            .x = 0, .y = 0, .z = -1.0f,  /* 1 meter forward from head */
            .scale = 1.0f,
            .quat_x = 0, .quat_y = 0, .quat_z = 0, .quat_w = 1.0f,
        };

        if (!oxr_host_submit_layer(srv->oxr_host, &layer)) {
            LOGW("Failed to submit layer to OpenXR host");
        }
    }

    /* Send acknowledgment */
    WaydroidXrFrameSubmitResp resp = {.error = 0};
    return send_msg(sd, WXRMSG_SWAPCHAIN_FRAME_RESP, &resp, sizeof(resp));
}

/* =========================================================================
 * Client connection handler
 * ========================================================================= */

static bool handle_client_message(IpcServer *srv, int sd) {
    WaydroidXrMsgHeader hdr;
    uint8_t payload[4096];

    if (!recv_all(sd, &hdr, sizeof(hdr))) {
        return false;
    }

    uint32_t read_len = (hdr.length < sizeof(payload)) ? hdr.length : sizeof(payload);
    if (hdr.length > 0 && read_len > 0) {
        if (!recv_all(sd, payload, read_len)) {
            return false;
        }
        /* Drain excess */
        if (hdr.length > sizeof(payload)) {
            uint8_t drain[512];
            uint32_t remaining = hdr.length - sizeof(payload);
            while (remaining > 0) {
                uint32_t chunk = (remaining < sizeof(drain)) ? remaining : sizeof(drain);
                if (!recv_all(sd, drain, chunk)) return false;
                remaining -= chunk;
            }
        }
    }

    bool success = true;
    switch ((WaydroidXrMsgType)hdr.type) {
        case WXRMSG_PING:
            success = handle_ping(srv, sd);
            break;
        case WXRMSG_QUERY_CAPS:
            success = handle_query_caps(srv, sd);
            break;
        case WXRMSG_ENUMERATE_REFRESH_RATES:
            success = handle_enumerate_refresh_rates(srv, sd);
            break;
        case WXRMSG_GET_REFRESH_RATE:
            success = handle_get_refresh_rate(srv, sd);
            break;
        case WXRMSG_REQUEST_REFRESH_RATE:
            success = handle_request_refresh_rate(srv, sd, payload, read_len);
            break;
        case WXRMSG_SWAPCHAIN_FRAME_END:
            success = handle_swapchain_frame_end(srv, sd, payload, read_len);
            break;
        default:
            LOGW("Unknown message type: 0x%02x", hdr.type);
            success = false;
    }

    return success;
}

/* =========================================================================
 * Server listener thread
 * ========================================================================= */

static void *server_listen_thread(void *arg) {
    IpcServer *srv = (IpcServer *)arg;

    LOGI("Listener thread started");

    while (!srv->should_stop) {
        /* Use select with timeout for graceful shutdown */
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(srv->listen_sd, &readfds);

        int ret = select(srv->listen_sd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            LOGE("select failed: %s", strerror(errno));
            break;
        }

        if (ret == 0) {
            /* Timeout; check should_stop again */
            continue;
        }

        if (!FD_ISSET(srv->listen_sd, &readfds)) {
            continue;
        }

        struct sockaddr_un addr = {0};
        socklen_t addr_len = sizeof(addr);

        int client_sd = accept(srv->listen_sd, (struct sockaddr *)&addr, &addr_len);
        if (client_sd < 0) {
            LOGE("accept failed: %s", strerror(errno));
            continue;
        }

        LOGI("Client connected");
        srv->client.sd = client_sd;
        srv->client.connected = true;

        /* Handle client until connection closes */
        while (!srv->should_stop && srv->client.connected) {
            if (!handle_client_message(srv, client_sd)) {
                LOGW("Client connection closed or error");
                srv->client.connected = false;
            }
        }

        close(client_sd);
        srv->client.sd = -1;
    }

    LOGI("Listener thread exiting");
    return NULL;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

IpcServer *ipc_server_create(void) {
    IpcServer *srv = (IpcServer *)malloc(sizeof(*srv));
    if (!srv) return NULL;

    memset(srv, 0, sizeof(*srv));
    srv->listen_sd = -1;
    srv->client.sd = -1;

    /* Create and bind Unix socket */
    srv->listen_sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srv->listen_sd < 0) {
        LOGE("socket failed: %s", strerror(errno));
        free(srv);
        return NULL;
    }

    /* Remove any stale socket file */
    unlink(IPC_SOCKET_PATH);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(srv->listen_sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOGE("bind failed: %s", strerror(errno));
        close(srv->listen_sd);
        free(srv);
        return NULL;
    }

    if (listen(srv->listen_sd, 1) < 0) {
        LOGE("listen failed: %s", strerror(errno));
        close(srv->listen_sd);
        unlink(IPC_SOCKET_PATH);
        free(srv);
        return NULL;
    }

    LOGI("Server listening on %s", IPC_SOCKET_PATH);

    /* Initialize OpenXR host */
    srv->oxr_host = oxr_host_init();
    if (!srv->oxr_host) {
        LOGW("Failed to initialize OpenXR host (runtime may not be available)");
        /* Continue anyway; we'll stub responses */
    }

    return srv;
}

void ipc_server_destroy(IpcServer *server) {
    if (!server) return;

    if (server->oxr_host) {
        oxr_host_shutdown(server->oxr_host);
    }

    if (server->client.sd >= 0) {
        close(server->client.sd);
    }

    if (server->listen_sd >= 0) {
        close(server->listen_sd);
        unlink(IPC_SOCKET_PATH);
    }

    free(server);
}

void ipc_server_listen(IpcServer *server) {
    if (!server) return;

    server->should_stop = false;
    /* Listener already runs in a thread; just wait for global stop */
    while (!server->should_stop) {
        sleep(1);
    }
}

void ipc_server_stop(IpcServer *server) {
    if (!server) return;
    server->should_stop = true;
}

int ipc_server_submit_frame(IpcServer *server,
                           const FrameMetadata *metadata_left,
                           const FrameMetadata *metadata_right,
                           int dma_buf_fd_left,
                           int dma_buf_fd_right)
{
    if (!server) return -1;
    /* Stub: actual implementation would import buffers and render */
    return 0;
}
