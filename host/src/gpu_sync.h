/*
 * gpu_sync.h — GPU Synchronization Primitives
 *
 * Handles GPU fences, semaphores, and frame pacing for proper
 * synchronization between CPU and GPU workloads.
 *
 * License: Apache 2.0
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GpuFence GpuFence;
typedef struct GpuSemaphore GpuSemaphore;

/* =========================================================================
 * GPU Fences — CPU waits for GPU work to complete
 * ========================================================================= */

/*
 * gpu_fence_create — Create a fence for GPU completion tracking.
 * Fence is created in the unsignaled state.
 */
GpuFence *gpu_fence_create(void);

/*
 * gpu_fence_destroy — Free fence resources.
 */
void gpu_fence_destroy(GpuFence *fence);

/*
 * gpu_fence_wait — Block until GPU completes signaled work.
 *   timeout_ms: Milliseconds to wait, or 0 for no timeout.
 * Returns true if signaled within timeout, false if timed out or error.
 */
bool gpu_fence_wait(GpuFence *fence, uint64_t timeout_ms);

/*
 * gpu_fence_is_signaled — Check if fence is signaled without blocking.
 */
bool gpu_fence_is_signaled(GpuFence *fence);

/*
 * gpu_fence_reset — Reset fence to unsignaled state.
 */
void gpu_fence_reset(GpuFence *fence);

/* =========================================================================
 * GPU Semaphores — Inter-queue synchronization (Vulkan)
 * ========================================================================= */

/*
 * gpu_semaphore_create — Create a semaphore for GPU-to-GPU sync.
 */
GpuSemaphore *gpu_semaphore_create(void);

/*
 * gpu_semaphore_destroy — Free semaphore.
 */
void gpu_semaphore_destroy(GpuSemaphore *sem);

/*
 * gpu_semaphore_signal — Submit signal operation to GPU.
 */
void gpu_semaphore_signal(GpuSemaphore *sem);

/*
 * gpu_semaphore_wait — Insert wait on GPU.
 */
void gpu_semaphore_wait(GpuSemaphore *sem);

/* =========================================================================
 * Frame Pacing and Timing
 * ========================================================================= */

typedef struct {
    int64_t frame_id;
    int64_t gpu_submit_time;      /* When CPU submitted to GPU (ns) */
    int64_t gpu_complete_time;    /* When GPU finished (ns) */
    int64_t gpu_latency_ns;       /* gpu_complete_time - gpu_submit_time */
    uint32_t dropped_frames;      /* Frames missed during pacing */
} GpuFrameStats;

/*
 * gpu_frame_pacing_init — Initialize frame pacing with target frame rate.
 *   target_rate_hz: Desired frame rate (e.g., 90.0 for 90 Hz)
 */
void gpu_frame_pacing_init(float target_rate_hz);

/*
 * gpu_frame_pacing_update — Call at the start of frame submission.
 * Returns time in nanoseconds until next frame deadline.
 */
int64_t gpu_frame_pacing_update(void);

/*
 * gpu_frame_pacing_get_stats — Retrieve timing statistics for monitoring.
 */
void gpu_frame_pacing_get_stats(GpuFrameStats *out_stats);

#ifdef __cplusplus
}
#endif
