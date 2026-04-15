# Waydroid XR Host Server

The **host-side OpenXR server** receives frame buffers from inside the Waydroid container and composites them to your local PCVR runtime (Monado, SteamVR, etc.).

> **Status:** MVP implementation with stubbed GPU/DMA-BUF integration. The IPC protocol and architectural framework are complete; GPU texture import via DMA-BUF is marked for future implementation.

---

## Architecture

```
┌─────────────────────────────────────────────┐
│  Waydroid Android Container (ARM64)         │
│                                             │
│  Quest APK → OpenXR Layer                   │
│     │ (xrEndFrame with eye buffers)         │
│     │ Unix socket IPC bridge                │
│     ▼                                       │
└─────────────────────────────────────────────┘
            │
            │ DMA-BUF file descriptors
            │ over Unix socket (/run/waydroid-xr/host.sock)
            ▼
┌─────────────────────────────────────────────┐
│  Host Linux (waydroid-xr-server)            │
│                                             │
│  ┌─ IPC Server ─────────────────────────┐  │
│  │ • Listen for container connections   │  │
│  │ • Handle OpenXR control messages     │  │
│  │ • Receive frame DMA-BUF FDs          │  │
│  └──────────────────────────────────────┘  │
│                 │                           │
│  ┌─ DMA-BUF Import ──────────────────────┐ │
│  │ • Import GPU textures from FDs       │  │
│  │ • Map GPU memory for rendering       │  │
│  └──────────────────────────────────────┘  │
│                 │                           │
│  ┌─ OpenXR Host ────────────────────────┐  │
│  │ • Create session with local runtime  │  │
│  │ • Submit quad layers (one per eye)   │  │
│  │ • Handle frame timing & sync         │  │
│  └──────────────────────────────────────┘  │
│                 │                           │
│     ▼ (OpenXR API calls)                   │
│   Host OpenXR Runtime (Monado, SteamVR)    │
│     │                                      │
│     ▼                                      │
│   Your PCVR Headset                        │
└─────────────────────────────────────────────┘
```

---

## Building the Host Server

### Prerequisites

- **Linux** (tested on Ubuntu 22.04+)
- **CMake** 3.22+
- **OpenXR SDK** (Khronos) — fetched automatically during build
- **OpenGL** or **Vulkan** development libraries (for GPU buffer support)
- **pthreads** (typically included)

### Build Steps

```bash
cd waydroid-xr-stubs/

# Build only the host server
./scripts/build_host_server.sh

# Or build everything (container layer + host server)
mkdir build && cd build
cmake -DCMAKE_BUILD_HOST_SERVER=ON ..
cmake --build . -j$(nproc)
```

### Output

- **Executable**: `build-host/waydroid_xr_server`
- **Installable**: `cmake --install build-host --prefix /usr/local`

### Installation

```bash
# System-wide installation
sudo mkdir -p /usr/local/bin
sudo cp build-host/waydroid_xr_server /usr/local/bin/

# Or use CMake install
cd build-host
sudo cmake --install .

# Verify
waydroid-xr-server --help
```

---

## Running the Server

### Prerequisites

1. **OpenXR Runtime** must be available:
   - **Monado**: `sudo apt install monado monado-tools`
   - **SteamVR**: Install via Steam, enable "Enable OpenXR support"
   - **OpenComposite**: For OpenVR-only headsets

2. **Socket directory** must be writable:
   ```bash
   sudo mkdir -p /run/waydroid-xr
   sudo chmod 777 /run/waydroid-xr
   ```

### Start the Server

```bash
# In one terminal on the host:
waydroid-xr-server

# You should see:
# [waydroid-xr-server] INFO: Server listening on /run/waydroid-xr/host.sock
# [waydroid-xr-server] INFO: Press Ctrl+C to stop.
```

### From Inside Waydroid Container

```bash
# Inside the container (in a separate terminal):
waydroid shell

# The container should now be able to connect to the host server.
# Launch a Meta Quest APK, and frame data will flow to your PCVR headset.
```

---

## Implementation Details

### IPC Protocol

The server communicates with the container via **length-prefixed binary messages** on Unix socket `/run/waydroid-xr/host.sock`.

**Message Header** (16 bytes):
```c
struct WaydroidXrMsgHeader {
    uint32_t magic;    /* 0x57585200 "WXR\0" */
    uint32_t type;     /* WaydroidXrMsgType enum */
    uint32_t length;   /* Payload size in bytes */
    uint32_t seq;      /* Sequence number for matching responses */
};
```

**Supported Messages**:
- `WXRMSG_PING` / `WXRMSG_PONG` — Keep-alive
- `WXRMSG_QUERY_CAPS` → `WXRMSG_CAPS_RESPONSE` — Capabilities negotiation
- `WXRMSG_ENUMERATE_REFRESH_RATES` → Response — Display refresh rates
- `WXRMSG_GET_REFRESH_RATE` → Response — Current refresh rate
- `WXRMSG_REQUEST_REFRESH_RATE` → Response — Request rate change
- `WXRMSG_SWAPCHAIN_FRAME_END` + ancillary **DMA-BUF FDs** → `WXRMSG_SWAPCHAIN_FRAME_RESP` — Frame submission

### Frame Submission Flow

1. **Container (Guest)**:
   ```
   xrEndFrame() with left & right eye swapchain buffers
     ↓
   Layer marshals buffer metadata into WaydroidXrFrameSubmit
     ↓
   Sends via Unix socket with DMA-BUF FDs in ancillary data (SCM_RIGHTS)
   ```

2. **Host Server**:
   ```
   Receives message header + metadata + ancillary DMA-BUF FDs
     ↓
   Imports FDs as GPU textures (OpenGL/Vulkan)
     ↓
   Wraps each texture in an XrCompositionLayerQuad
     ↓
   Submits quad layers to host OpenXR runtime
     ↓
   Runtime composites to HMD display
   ```

### File Structure

```
host/
├── CMakeLists.txt              ← Build configuration
├── README.md                   ← This file
└── src/
    ├── waydroid_xr_server.c    ← Main executable entry point
    ├── ipc_server.c/h          ← Unix socket server implementation
    ├── openxr_host.c/h         ← Host OpenXR session management
    └── dma_buf_import.c/h      ← GPU buffer import (OpenGL/Vulkan)

../scripts/
├── build_host_server.sh        ← Build script
```

---

## Stub Components (MVP Roadmap)

The following are marked as stubs pending full implementation:

### 1. **DMA-BUF GPU Import** (`dma_buf_import.c`)

```c
// Currently stubbed:
bool dmabuf_import_gl(int fd, const DmaBufMeta *meta, uint32_t *out_handle);
DmaBufBuffer *dmabuf_import_vulkan(int fd, const DmaBufMeta *meta);
```

**Full implementation requires**:
- **OpenGL path**: `EGL_EXT_image_dma_buf_import` to create `EGLImage` from FD, then wrap in GL texture
- **Vulkan path**: `VK_EXT_external_memory_dma_buf` to import FD as `VkDeviceMemory`, then bind to `VkImage`

### 2. **Graphics Binding for OpenXR** (`openxr_host.c`)

```c
// Currently stubbed:
XrSessionCreateInfo info = {
    .next = NULL,  /* Would set graphics binding here */
};
```

**Full implementation requires**:
- Detect available graphics APIs (OpenGL via EGL, Vulkan, D3D12 on Windows)
- Set up `XrGraphicsBindingEGLKHR` or `XrGraphicsBindingVulkanKHR`
- Create proper swapchain with GPU resources

### 3. **Frame Timing & Sync** (`ipc_server.c` + `openxr_host.c`)

Currently uses basic `xrWaitFrame` / `xrBeginFrame` / `xrEndFrame` without advanced synchronization.

**Future enhancements**:
- Per-eye buffer timing
- GPU pipeline flush and fence synchronization
- DMA-BUF release notification back to container

---

## Testing & Debugging

### Verify IPC Connection

```bash
# Terminal 1: Start server
waydroid-xr-server

# Terminal 2: Check socket was created
ls -la /run/waydroid-xr/host.sock

# Terminal 3: Tail logs
dmesg | grep -i waydroid
```

### Check OpenXR Runtime

```bash
# List detected runtimes
ls /etc/xdg/openxr/*/active_runtime.json
cat /etc/xdg/openxr/*/active_runtime.json

# Test Monado directly
monado-test
```

### Strace Container Communications

```bash
# Monitor IPC traffic inside container
waydroid shell
strace -e socket,connect,sendto,recvfrom -p $(pgrep myapp)
```

### Logs

The server logs to stderr with `[waydroid-xr-server]` prefix:

```
[waydroid-xr-server] INFO: Server listening on /run/waydroid-xr/host.sock
[waydroid-xr-server] INFO: Client connected
[waydroid-xr-server] INFO: Frame submission: num_metadata=2, frame_id=142
```

---

## Known Limitations

1. **Single Container Connection** — Currently supports only one simultaneous Waydroid container. Multi-container support would require connection pooling.

2. **No Graphics Binding** — GPU rendering pipeline is not fully connected. Quad layers are created but textures are not properly mapped.

3. **No Frame Synchronization** — Frames are submitted without synchronization primitives (fences, semaphores). Real-time sync requires careful GPU pipeline management.

4. **Stub GPU Import** — DMA-BUF import functions are not implemented; they return false. See roadmap above.

5. **No Eye Tracking** — Container side doesn't yet fetch gaze data or send it back to container.

---

## Next Steps

### Phase 2b: Complete DMA-BUF Integration

- [ ] Implement `dmabuf_import_gl()` using `EGL_EXT_image_dma_buf_import`
- [ ] Implement `dmabuf_import_vulkan()` using `VK_EXT_external_memory_dma_buf`
- [ ] Add GPU driver detection (NVIDIA, AMD, Intel)
- [ ] Test with real container frame submission

### Phase 2c: Graphics Binding

- [ ] Select and configure appropriate OpenXR graphics binding
- [ ] Create real GPU swapchain (not dummy)
- [ ] Render imported textures to swapchain

### Phase 3: Synchronization & Performance

- [ ] Per-frame GPU synchronization (fences, semaphores)
- [ ] DMA-BUF release callbacks to container
- [ ] Latency profiling and optimization
- [ ] Support for multiple refresh rates

---

## References

- [OpenXR Specification](https://www.khronos.org/registry/OpenXR/)
- [Linux DMA-BUF](https://www.kernel.org/doc/html/latest/userspace-api/dma-buf-alloc-exchange.html)
- [EGL_EXT_image_dma_buf_import](https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_image_dma_buf_import.txt)
- [Vulkan VK_EXT_external_memory_dma_buf](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_external_memory_dma_buf.html)
- [Monado OpenXR Runtime](https://monado.freedesktop.org/)

---

## License

Apache License 2.0 — See [LICENSE](../LICENSE) in the project root.
