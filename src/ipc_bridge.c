/*
 * ipc_bridge.c — Waydroid XR IPC Bridge Implementation
 *
 * License: Apache 2.0
 */

#include "ipc_bridge.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <android/log.h>

#define LOG_TAG "WaydroidXR-IPC"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* =========================================================================
 * State
 * ========================================================================= */

static int          s_sockfd    = -1;
static bool         s_connected = false;
static uint32_t     s_seq       = 0;
static pthread_mutex_t s_lock   = PTHREAD_MUTEX_INITIALIZER;

static WaydroidXrCapsResponse s_caps = {0};

/* =========================================================================
 * Low-level send / recv helpers
 * ========================================================================= */

static bool send_all(int fd, const void *buf, size_t len) {
    const uint8_t *ptr = (const uint8_t *)buf;
    while (len > 0) {
        ssize_t n = send(fd, ptr, len, MSG_NOSIGNAL);
        if (n <= 0) {
            LOGE("send_all: %s", strerror(errno));
            return false;
        }
        ptr += n;
        len -= (size_t)n;
    }
    return true;
}

static bool recv_all(int fd, void *buf, size_t len) {
    uint8_t *ptr = (uint8_t *)buf;
    while (len > 0) {
        ssize_t n = recv(fd, ptr, len, MSG_WAITALL);
        if (n <= 0) {
            LOGE("recv_all: %s", strerror(errno));
            return false;
        }
        ptr += n;
        len -= (size_t)n;
    }
    return true;
}

static bool send_msg(WaydroidXrMsgType type,
                     const void *payload, uint32_t payload_len,
                     uint32_t *out_seq)
{
    WaydroidXrMsgHeader hdr = {
        .magic  = WXRMSG_MAGIC,
        .type   = (uint32_t)type,
        .length = payload_len,
        .seq    = __sync_fetch_and_add(&s_seq, 1),
    };
    if (out_seq) *out_seq = hdr.seq;

    if (!send_all(s_sockfd, &hdr, sizeof(hdr))) return false;
    if (payload_len > 0 && payload) {
        if (!send_all(s_sockfd, payload, payload_len)) return false;
    }
    return true;
}

static bool recv_msg(WaydroidXrMsgHeader *out_hdr,
                     void *payload_buf, uint32_t buf_size)
{
    if (!recv_all(s_sockfd, out_hdr, sizeof(*out_hdr))) return false;
    if (out_hdr->magic != WXRMSG_MAGIC) {
        LOGE("recv_msg: bad magic 0x%08x", out_hdr->magic);
        return false;
    }
    if (out_hdr->length > 0) {
        uint32_t read_len = out_hdr->length < buf_size
                          ? out_hdr->length : buf_size;
        if (!recv_all(s_sockfd, payload_buf, read_len)) return false;
        /* Drain any excess */
        if (out_hdr->length > buf_size) {
            uint8_t drain[64];
            uint32_t remaining = out_hdr->length - buf_size;
            while (remaining > 0) {
                uint32_t chunk = remaining < sizeof(drain)
                               ? remaining : sizeof(drain);
                if (!recv_all(s_sockfd, drain, chunk)) return false;
                remaining -= chunk;
            }
        }
    }
    return true;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

bool ipc_bridge_init(void) {
    pthread_mutex_lock(&s_lock);

    if (s_connected) {
        pthread_mutex_unlock(&s_lock);
        return true;
    }

    s_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s_sockfd < 0) {
        LOGE("socket(): %s", strerror(errno));
        pthread_mutex_unlock(&s_lock);
        return false;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_BRIDGE_SOCKET_PATH,
            sizeof(addr.sun_path) - 1);

    if (connect(s_sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOGW("ipc_bridge_init: no host server at %s (%s) — using stubs",
             IPC_BRIDGE_SOCKET_PATH, strerror(errno));
        close(s_sockfd);
        s_sockfd = -1;
        pthread_mutex_unlock(&s_lock);
        return false;
    }

    /* Send PING */
    uint32_t seq;
    if (!send_msg(WXRMSG_PING, NULL, 0, &seq)) {
        LOGE("ipc_bridge_init: ping failed");
        goto fail;
    }

    /* Receive PONG */
    WaydroidXrMsgHeader hdr;
    uint8_t dummy[4];
    if (!recv_msg(&hdr, dummy, sizeof(dummy)) ||
        hdr.type != WXRMSG_PONG) {
        LOGE("ipc_bridge_init: expected PONG, got type %u", hdr.type);
        goto fail;
    }

    /* Query capabilities */
    if (!send_msg(WXRMSG_QUERY_CAPS, NULL, 0, &seq)) goto fail;
    if (!recv_msg(&hdr, &s_caps, sizeof(s_caps)) ||
        hdr.type != WXRMSG_CAPS_RESPONSE) {
        LOGE("ipc_bridge_init: caps query failed");
        goto fail;
    }

    LOGI("ipc_bridge_init: connected to host (v%u, hand_mesh=%d, "
         "eye=%d, passthrough=%d, max_hz=%.1f)",
         s_caps.version,
         s_caps.supports_hand_mesh,
         s_caps.supports_eye_tracking,
         s_caps.supports_passthrough,
         s_caps.max_refresh_rate);

    s_connected = true;
    pthread_mutex_unlock(&s_lock);
    return true;

fail:
    close(s_sockfd);
    s_sockfd = -1;
    pthread_mutex_unlock(&s_lock);
    return false;
}

void ipc_bridge_shutdown(void) {
    pthread_mutex_lock(&s_lock);
    if (s_sockfd >= 0) {
        close(s_sockfd);
        s_sockfd = -1;
    }
    s_connected = false;
    pthread_mutex_unlock(&s_lock);
}

bool ipc_bridge_is_connected(void) {
    return s_connected;
}

bool ipc_bridge_supports_hand_mesh(void) {
    return s_connected && s_caps.supports_hand_mesh;
}

/* =========================================================================
 * Forwarded calls
 * ========================================================================= */

XrResult ipc_bridge_EnumerateDisplayRefreshRates(
    XrSession session,
    uint32_t capacityInput,
    uint32_t *countOutput,
    float *refreshRates)
{
    pthread_mutex_lock(&s_lock);
    if (!s_connected) {
        pthread_mutex_unlock(&s_lock);
        return XR_ERROR_RUNTIME_FAILURE;
    }

    if (!send_msg(WXRMSG_ENUMERATE_REFRESH_RATES, NULL, 0, NULL)) {
        pthread_mutex_unlock(&s_lock);
        return XR_ERROR_RUNTIME_FAILURE;
    }

    WaydroidXrMsgHeader hdr;
    WaydroidXrRefreshRateResp resp;
    if (!recv_msg(&hdr, &resp, sizeof(resp)) ||
        hdr.type != WXRMSG_ENUMERATE_REFRESH_RATES_RESP) {
        pthread_mutex_unlock(&s_lock);
        return XR_ERROR_RUNTIME_FAILURE;
    }
    pthread_mutex_unlock(&s_lock);

    *countOutput = resp.count;
    if (refreshRates && capacityInput >= resp.count) {
        memcpy(refreshRates, resp.rates, resp.count * sizeof(float));
    }
    return XR_SUCCESS;
}

XrResult ipc_bridge_GetDisplayRefreshRate(
    XrSession session,
    float *refreshRate)
{
    pthread_mutex_lock(&s_lock);
    if (!s_connected) {
        pthread_mutex_unlock(&s_lock);
        return XR_ERROR_RUNTIME_FAILURE;
    }
    if (!send_msg(WXRMSG_GET_REFRESH_RATE, NULL, 0, NULL)) {
        pthread_mutex_unlock(&s_lock);
        return XR_ERROR_RUNTIME_FAILURE;
    }

    WaydroidXrMsgHeader hdr;
    float rate;
    if (!recv_msg(&hdr, &rate, sizeof(rate)) ||
        hdr.type != WXRMSG_GET_REFRESH_RATE_RESP) {
        pthread_mutex_unlock(&s_lock);
        return XR_ERROR_RUNTIME_FAILURE;
    }
    pthread_mutex_unlock(&s_lock);

    *refreshRate = rate;
    return XR_SUCCESS;
}

XrResult ipc_bridge_RequestDisplayRefreshRate(
    XrSession session,
    float refreshRate)
{
    pthread_mutex_lock(&s_lock);
    if (!s_connected) {
        pthread_mutex_unlock(&s_lock);
        return XR_SUCCESS; /* silently ignore */
    }
    send_msg(WXRMSG_REQUEST_REFRESH_RATE, &refreshRate, sizeof(refreshRate), NULL);

    WaydroidXrMsgHeader hdr;
    uint8_t dummy[4];
    recv_msg(&hdr, dummy, sizeof(dummy));
    pthread_mutex_unlock(&s_lock);
    return XR_SUCCESS;
}

XrResult ipc_bridge_GetHandMesh(
    XrHandTrackerEXT handTracker,
    void *mesh)
{
    pthread_mutex_lock(&s_lock);
    if (!s_connected || !s_caps.supports_hand_mesh) {
        pthread_mutex_unlock(&s_lock);
        return XR_ERROR_FEATURE_UNSUPPORTED;
    }
    /* TODO: serialize XrHandTrackingMeshFB request over socket */
    /* For now return unsupported until full frame protocol is implemented */
    pthread_mutex_unlock(&s_lock);
    return XR_ERROR_FEATURE_UNSUPPORTED;
}
