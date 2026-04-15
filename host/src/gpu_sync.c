/*
 * gpu_sync.c — GPU Synchronization Implementation
 *
 * Implements CPU-GPU synchronization using platform-specific APIs
 * (GL sync, VkFence, conditional waits).
 *
 * License: Apache 2.0
 */

#include "gpu_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

#define LOG(level, fmt, ...) \
    fprintf(stderr, "[waydroid-xr-server] " level ": " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)

/* =========================================================================
 * GPU Fences (OpenGL GL_ARB_sync backed)
 * ========================================================================= */

struct GpuFence {
    void *gl_sync;  /* GLsync object if OpenGL */
    bool is_signaled;
    pthread_mutex_t lock;
};

GpuFence *gpu_fence_create(void) {
    GpuFence *fence = (GpuFence *)malloc(sizeof(*fence));
    if (!fence) return NULL;

    fence->gl_sync = NULL;
    fence->is_signaled = false;
    pthread_mutex_init(&fence->lock, NULL);

    LOGI("Created GPU fence");
    return fence;
}

void gpu_fence_destroy(GpuFence *fence) {
    if (!fence) return;

    pthread_mutex_destroy(&fence->lock);
    free(fence);
}

bool gpu_fence_wait(GpuFence *fence, uint64_t timeout_ms) {
    if (!fence) return false;

    pthread_mutex_lock(&fence->lock);

    if (fence->is_signaled) {
        pthread_mutex_unlock(&fence->lock);
        return true;
    }

    /* For GL-backed fence, we'd call glClientWaitSync here.
     * For now, use simple condition variable approach. */

    pthread_mutex_unlock(&fence->lock);

    /* Busy-wait with timeout (naive; production would use EGL sync objects) */
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    end_time.tv_sec += timeout_ms / 1000;
    end_time.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (end_time.tv_nsec >= 1000000000) {
        end_time.tv_sec++;
        end_time.tv_nsec -= 1000000000;
    }

    while (true) {
        pthread_mutex_lock(&fence->lock);
        if (fence->is_signaled) {
            pthread_mutex_unlock(&fence->lock);
            return true;
        }
        pthread_mutex_unlock(&fence->lock);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec > end_time.tv_sec ||
            (now.tv_sec == end_time.tv_sec && now.tv_nsec >= end_time.tv_nsec)) {
            return false;  /* Timeout */
        }

        /* Sleep briefly to avoid busy-waiting */
        usleep(100);  /* 100 microseconds */
    }
}

bool gpu_fence_is_signaled(GpuFence *fence) {
    if (!fence) return false;

    pthread_mutex_lock(&fence->lock);
    bool signaled = fence->is_signaled;
    pthread_mutex_unlock(&fence->lock);

    return signaled;
}

void gpu_fence_reset(GpuFence *fence) {
    if (!fence) return;

    pthread_mutex_lock(&fence->lock);
    fence->is_signaled = false;
    pthread_mutex_unlock(&fence->lock);
}

/* =========================================================================
 * GPU Semaphores (Placeholder)
 * ========================================================================= */

struct GpuSemaphore {
    void *vk_semaphore;  /* VkSemaphore if Vulkan */
    bool is_binary;
};

GpuSemaphore *gpu_semaphore_create(void) {
    GpuSemaphore *sem = (GpuSemaphore *)malloc(sizeof(*sem));
    if (!sem) return NULL;

    sem->vk_semaphore = NULL;
    sem->is_binary = true;

    LOGI("Created GPU semaphore");
    return sem;
}

void gpu_semaphore_destroy(GpuSemaphore *sem) {
    if (!sem) return;
    free(sem);
}

void gpu_semaphore_signal(GpuSemaphore *sem) {
    if (!sem) return;
    /* In Vulkan: vkCmdSignalEvent, vkSetEvent, etc. */
}

void gpu_semaphore_wait(GpuSemaphore *sem) {
    if (!sem) return;
    /* In Vulkan: vkCmdWaitEvents, vkWaitForFences, etc. */
}

/* =========================================================================
 * Frame Pacing
 * ========================================================================= */

static struct {
    float target_rate_hz;
    int64_t frame_interval_ns;
    int64_t last_frame_time;
    int64_t frame_count;
    uint32_t dropped_frames;
} g_frame_pace = {
    .target_rate_hz = 90.0f,
    .frame_interval_ns = 11111111,  /* ~90 Hz */
    .last_frame_time = 0,
    .frame_count = 0,
    .dropped_frames = 0,
};

void gpu_frame_pacing_init(float target_rate_hz) {
    g_frame_pace.target_rate_hz = target_rate_hz;
    g_frame_pace.frame_interval_ns = (int64_t)(1e9 / target_rate_hz);
    g_frame_pace.last_frame_time = 0;
    g_frame_pace.frame_count = 0;
    g_frame_pace.dropped_frames = 0;
    LOGI("Frame pacing initialized: %.1f Hz (interval: %" PRId64 " ns)",
         target_rate_hz, g_frame_pace.frame_interval_ns);
}

int64_t gpu_frame_pacing_update(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int64_t current_time = now.tv_sec * 1000000000LL + now.tv_nsec;

    if (g_frame_pace.last_frame_time == 0) {
        g_frame_pace.last_frame_time = current_time;
        return 0;  /* First frame; no wait */
    }

    int64_t elapsed = current_time - g_frame_pace.last_frame_time;
    int64_t wait_ns = g_frame_pace.frame_interval_ns - elapsed;

    if (wait_ns < 0) {
        /* Running late; dropped a frame */
        g_frame_pace.dropped_frames++;
        if (g_frame_pace.dropped_frames % 30 == 1) {
            LOGW("Running late on frame %" PRId64 " (dropped %u frames so far)",
                 g_frame_pace.frame_count, g_frame_pace.dropped_frames);
        }
        g_frame_pace.last_frame_time = current_time;
        return 0;
    }

    g_frame_pace.last_frame_time = current_time + wait_ns;
    g_frame_pace.frame_count++;

    return wait_ns;
}

void gpu_frame_pacing_get_stats(GpuFrameStats *out_stats) {
    if (!out_stats) return;

    memset(out_stats, 0, sizeof(*out_stats));
    out_stats->frame_id = g_frame_pace.frame_count;
    out_stats->dropped_frames = g_frame_pace.dropped_frames;
}
