# RetroAchievements Feasibility — Verdict: **YES, it's doable**

## RAM: No problem
The app uses **5.6 MB** of the Old 3DS's **64 MB** APPLICATION region. RetroAchievements would add ~**1.9 MB** (mostly the 1 MB SOC socket buffer). Over 50 MB of headroom remains — RAM is not a concern.

### Memory Budget

| Component | KB |
|-----------|----|
| rcheevos code (.text) | 300 |
| rc_client state + parsed JSON | 200 |
| SOC buffer (network socket layer) | 1024 |
| httpc response buffer | 256 |
| Background thread stack | 128 |
| Achievement icon texture (1 active) | 16 |
| MD5 hash + misc buffers | 16 |
| **ESTIMATED TOTAL** | **1940 (~1.9 MB)** |

---

## Phase 1: Environment & Dependency Setup

**Feasible: YES — Low risk**

- **rcheevos** is pure C with no dependencies — drop the source files into the project and compile directly. No `-lrcheevos` needed since it's not a prebuilt library for 3DS; just add the `.c` files to the Makefile's source list.
- **httpc** is already in libctru (`3ds/services/httpc.h` exists in `devkitPro/libctru/include`). It links via `-lctru` which the Makefile already includes — no separate `-lhttpc` library needed.

---

## Phase 2: The Read-Only Memory Bridge

**Feasible: YES — Trivial**

The NES Work RAM is a global array in VirtuaNES:

```c
// cores/virtuanes/NES/MMU.h / MMU.cpp
extern BYTE RAM[8*1024];   // 2KB mirrored to 8KB at $0000-$1FFF
```

The CPU already reads it with the same masking pattern (`Cpu.cpp` line 647):

```c
return RAM[addr & 0x07FF];
```

The `peek_ram()` wrapper for rcheevos would be a one-liner:

```c
static uint8_t peek_ram(uint32_t addr, void* userdata) {
    return RAM[addr & 0x07FF];
}
```

---

## Phase 3: The Background Thread

**Feasible: YES — but modify the approach**

**Do NOT use a dedicated thread.** We already tried threading the emulation loop on core 1 (v1.8.3) and reverted it in v1.8.4 because the Old 3DS core 1 is too slow for VirtuaNES. The specific history:

- v1.8.3: Moved `nesEmulateFrame()` to core 1 — massive frame-rate regression on Old 3DS.
- v1.8.4: Reverted to fully synchronous emulation on core 0 with `framesDue` catch-up.

**However**, `rc_client_do_frame()` is far lighter than `nesEmulateFrame()`. It only compares memory values against parsed conditions — no PPU rendering, no APU synthesis. It takes microseconds, not milliseconds.

**Recommendation:** Call `rc_client_do_frame()` inline after each `nesEmulateFrame()` on core 0. The frame budget is 16ms, emulation takes ~8-10ms, and achievement checking costs sub-0.1ms. There's plenty of room. No thread, no mutex, no sync complexity.

If performance ever becomes a concern (unlikely), it could be moved to core 1 later — unlike full emulation, achievement checking can tolerate being late by a frame or two.

---

## Phase 4: Asynchronous Network Initialization

**Feasible: YES — Medium risk (3DS WiFi is slow)**

- `httpcInit()` + `httpcOpenContext()` are available in the bundled libctru.
- MD5 hashing a NES ROM (typically 128KB-1MB) takes milliseconds — no issue.
- **Caveat**: The 3DS WiFi stack is slow. The initial API call to identify the game and download the achievement JSON will take 2-5 seconds.
- **Solution**: Run the network call in a thread on core 1 (this is lightweight I/O, perfectly fine for core 1 unlike CPU-bound emulation). Set a flag when done. The main loop starts checking achievements only after the flag is set. The game plays immediately; achievements appear a few seconds after load.

---

## Phase 5: UI Integration

**Feasible: YES — Low risk, natural fit**

The bottom screen already has a notification system:

- `V2UI::showNotification()` with a timer-based display.
- `renderBottom()` draws notifications each frame and decrements the timer.

Achievement popups would use the exact same pattern — set the text + icon and let `renderBottom()` draw it for N frames. For achievement icons, one 64x64 RGBA texture = 16 KB per badge.

---

## Summary

| Phase | Feasible? | Risk | Notes |
|-------|-----------|------|-------|
| 1 - Dependencies | YES | Low | Pure C library, httpc already in libctru |
| 2 - Memory Bridge | YES | Trivial | `RAM[addr & 0x07FF]` — one-liner |
| 3 - Achievement Logic | YES (modified) | Low | Call inline after emulation, skip threading |
| 4 - Network | YES | Medium | Async I/O thread on core 1 for HTTP only |
| 5 - UI | YES | Low | Reuse existing notification system |
| **RAM** | **YES** | **None** | ~1.9 MB additional out of 50+ MB free |

**Key change from original plan:** Kill the dedicated achievement-checking thread (Phase 3). Call `rc_client_do_frame()` synchronously on core 0 after each emulated frame. Reserve threading only for the Phase 4 network call where it actually matters.
