/*
 * waydroid_xr_server.c — Waydroid XR Host Server Entry Point
 *
 * Main executable for the host-side OpenXR server.
 * Listens for connections from Waydroid container, receives frame buffers,
 * and composes them to the local PCVR runtime (Monado, SteamVR, etc.).
 *
 * Typical usage:
 *   $ waydroid-xr-server &
 *   # Inside Waydroid container, launch a Quest APK...
 *
 * License: Apache 2.0
 */

#include "ipc_server.h"
#include "openxr_host.h"
#include "dma_buf_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

static volatile int g_should_stop = 0;

static void signal_handler(int sig) {
    fprintf(stderr, "\nReceived signal %d; shutting down...\n", sig);
    g_should_stop = 1;
}

int main(int argc, char *argv[]) {
    fprintf(stderr, "waydroid-xr-server v1.0.0\n");
    fprintf(stderr, "================================\n\n");

    /* Set up signal handlers for graceful shutdown */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize DMA-BUF support */
    if (!dmabuf_init()) {
        fprintf(stderr, "WARNING: DMA-BUF support failed; GPU import will not work\n");
    }

    /* Create and start IPC server */
    IpcServer *server = ipc_server_create();
    if (!server) {
        fprintf(stderr, "ERROR: Failed to create IPC server\n");
        dmabuf_shutdown();
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Server initialized; listening for connections...\n");
    fprintf(stderr, "Socket path: /run/waydroid-xr/host.sock\n");
    fprintf(stderr, "Press Ctrl+C to stop.\n\n");

    /* Start listener thread */
    pthread_t listener_tid;
    if (pthread_create(&listener_tid, NULL, 
                      (void *(*)(void *))ipc_server_listen, 
                      server) != 0) {
        fprintf(stderr, "ERROR: Failed to create listener thread\n");
        ipc_server_destroy(server);
        dmabuf_shutdown();
        return EXIT_FAILURE;
    }

    /* Main loop: wait for shutdown signal */
    while (!g_should_stop) {
        sleep(1);
    }

    fprintf(stderr, "\nShutting down...\n");

    /* Gracefully stop server */
    ipc_server_stop(server);
    pthread_join(listener_tid, NULL);

    /* Clean up */
    ipc_server_destroy(server);
    dmabuf_shutdown();

    fprintf(stderr, "Shutdown complete.\n");
    return EXIT_SUCCESS;
}
