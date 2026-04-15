# waydroid-xr-stubs

**Meta/FB/Oculus OpenXR extension compatibility layer for Waydroid**

Enables Meta Quest Android APKs to run inside a [Waydroid](https://waydro.id/) container on Linux, bridging proprietary Meta OpenXR extensions to a host PCVR runtime (Monado, SteamVR).

> **Status:** Foundation / proof-of-concept. The layer loads, extensions enumerate correctly, and stub functions prevent crashes. The IPC bridge to the host runtime is scaffolded. **Phase 2 (frame transport) is in progress:** Host server receives frames via DMA-BUF IPC; GPU import and composition are stubbed pending full implementation.

---

## Architecture

```
┌────────────────────────────────────────────────────────────┐
│  Waydroid Android Container (guest ARM64 environment)      │
│                                                            │
│   Quest APK                                                │
│     │ calls xrCreateInstance / xrEndFrame / etc.          │
│     ▼                                                      │
│   OpenXR Loader (Khronos, ARM64)                           │
│     │                                                      │
│     ▼  implicit API layer (this project)                   │
│   libopenxr_waydroid_meta_compat.so                        │
│     │  • Injects XR_FB_* / XR_META_* into extension list  │
│     │  • Routes Meta function pointers to stubs            │
│     │  • Connects to host via Unix socket IPC              │
│     │                                                      │
│     ▼  (for core OpenXR: xrEndFrame, xrLocateViews, etc.) │
│   Host OpenXR runtime (via socket bridge)                  │
└──────────────────────────────────┬─────────────────────────┘
                                   │
                    Unix socket: /run/waydroid-xr/host.sock
                                   │
┌──────────────────────────────────▼─────────────────────────┐
│  Host Linux                                                 │
│                                                            │
│   waydroid-xr-server  (in host/ subdirectory)              │
│     │  • Listens for container IPC connections             │
│     │  • Imports DMA-BUF frame buffers                      │
│     │  • Submits quad layers to OpenXR runtime             │
│     ▼                                                      │
│   Monado OpenXR runtime  (or SteamVR via OpenComposite)    │
│     │                                                      │
│     ▼                                                      │
│   Your PCVR headset (Index, Vive, Reverb G2, etc.)         │
└────────────────────────────────────────────────────────────┘
```

---

## Extension Coverage

### Forwarded to Host Runtime (when IPC bridge is active)
| Extension | Status | Notes |
|---|---|---|
| `XR_FB_display_refresh_rate` | ✅ Forwarded | Host reports real headset refresh rates |
| `XR_FB_hand_tracking_mesh` | 🟡 Partial | Forwarded if host supports it |

### Stubbed with Safe Fallbacks
| Extension | Behavior |
|---|---|
| `XR_FB_color_space` | Returns REC709/REC2020, ignores set requests |
| `XR_FB_foveation` + `_configuration` | Creates dummy profile handles; apps render full-res |
| `XR_META_performance_metrics` | Returns zero counters; apps can profile with PC tools instead |
| `XR_META_boundary_visibility` | Always reports boundary suppressed |

### Stubbed as Not Supported (graceful degradation)
| Extension | Reason |
|---|---|
| `XR_FB_passthrough` | No camera on PCVR headset |
| `XR_FB_spatial_entity` + `_query` + `_storage` | No room mapping on PC |
| `XR_FB_scene` + `_capture` | Requires Quest spatial scan |
| `XR_FB_face_tracking` | No facial camera |
| `XR_FB_eye_tracking_social` | No IR eye tracker on most PCVR HMDs |
| `XR_FB_body_tracking` | No full-body sensor suite |
| `XR_META_virtual_keyboard` | Use PC keyboard; returns `XR_ERROR_FEATURE_UNSUPPORTED` |
| `XR_META_environment_depth` | No depth sensor |
| `XR_OCULUS_android_session_state_enable` | Deprecated; no-op |

---

## Building

### Prerequisites
- Android NDK r25+
- CMake 3.22+
- Ninja

### Build (ARM64, targets Waydroid guest)
```bash
NDK_PATH=/path/to/ndk ./scripts/build_ndk.sh
```

Output: `dist/android-arm64/vendor/lib64/openxr/libopenxr_waydroid_meta_compat.so`

### Install into Waydroid System Image
```bash
# See full instructions at bottom of scripts/build_ndk.sh
waydroid session stop
sudo mount -o rw,remount /var/lib/waydroid/images/system.img /mnt/waydroid
sudo cp dist/android-arm64/vendor/lib64/openxr/*.so /mnt/waydroid/vendor/lib64/openxr/
sudo cp dist/android-arm64/vendor/etc/openxr/1/api_layers/implicit.d/*.json \
    /mnt/waydroid/vendor/etc/openxr/1/api_layers/implicit.d/
sudo umount /mnt/waydroid
waydroid session start
```

### Build (Host Server, Linux)

The host-side server receives frame buffers from the container and composites
them to your PCVR runtime. It requires CMake, the Khronos OpenXR SDK, and a
local OpenXR runtime (Monado, SteamVR, etc.).

```bash
# Build host server standalone
./scripts/build_host_server.sh

# Or build everything (container layer + host):
mkdir build && cd build
cmake -DCMAKE_BUILD_HOST_SERVER=ON ..
cmake --build . -j$(nproc)
```

Output: `build-host/waydroid_xr_server`

See [host/README.md](host/README.md) for full documentation on running and
debugging the host server.

---

## IPC Bridge Protocol

The bridge uses length-prefixed binary messages over a Unix domain socket.

**Message header (16 bytes):**
```
uint32  magic    = 0x57585200  "WXR\0"
uint32  type     (WaydroidXrMsgType enum)
uint32  length   (payload bytes after header)
uint32  seq      (monotonic, for request/response matching)
```

The host server (`waydroid-xr-server`) needs to be running for forwarded
extensions to work. Without it, stubs activate automatically — apps load
but hardware-specific features are unavailable.

---

## Roadmap

### Phase 1 — This repo (foundation)
- [x] Meta extension enumeration injection
- [x] Stub implementations for all known XR_FB_* / XR_META_* extensions
- [x] IPC bridge connection scaffolding
- [x] Forwarded: display refresh rate
- [ ] Forwarded: full frame submission (xrEndFrame → host compositor)

### Phase 2 — Core frame transport (waydroid-xr-server)
- [ ] Host server receiving GPU frames from container via DMA-BUF / shared memory
- [ ] Submitting frames to Monado/SteamVR
- [ ] Controller input forwarding (host → container)
- [ ] Head pose forwarding (host → container)

### Phase 3 — Extended features
- [ ] Hand tracking mesh from host runtime
- [ ] Haptics forwarding (container → host → controller)
- [ ] Eye tracking from supported PCVR HMDs

---

## Related Projects
- [Waydroid](https://github.com/waydroid/waydroid) — Android container for Linux
- [Monado](https://monado.freedesktop.org/) — Open-source OpenXR runtime
- [WiVRn](https://github.com/WiVRn/WiVRn) — OpenXR streaming (Quest → PC)
- [Valve Lepton](https://www.gamingonlinux.com/2025/12/valves-version-of-android-on-linux-based-on-waydroid-is-now-called-lepton/) — Valve's Waydroid fork for Steam Frame
- [OpenComposite](https://gitlab.com/znixian/OpenOVR) — OpenVR → OpenXR translation

---

## License
Apache 2.0. All extension names, struct layouts, and enumerant values are
derived from the public [Khronos OpenXR Registry](https://github.com/KhronosGroup/OpenXR-Registry)
and Meta's public [Meta-OpenXR-SDK](https://github.com/meta-quest/Meta-OpenXR-SDK).
No proprietary Meta source code is used or reproduced.
