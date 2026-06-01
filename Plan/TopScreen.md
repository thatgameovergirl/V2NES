# V2NES — Top Screen NES Emulation Roadmap

## Source

All emulator source comes from bubble2k16/emus3ds:

- **Repository:** https://github.com/bubble2k16/emus3ds (branch: `master`)
- **NES core:** `src/cores/virtuanes/` — CPU, PPU, APU, mappers
- **3DS platform:** `src/3ds/` — video, audio, input, config, font, menu, file I/O, cheat engine, save states
- **Build reference:** `virtuanes-make` (shows the compiler flags and directory layout)

The V2NES backup at `C:\V2NES\Backups\v2nes00.zip` may also contain a snapshot of these files.

---

## Phase 0 — Get the VirtuaNES source

**Action:**
1. Clone or download https://github.com/bubble2k16/emus3ds into `C:\V2NES\emus3ds_src\`
2. List every file from `src/cores/virtuanes/` and `src/3ds/`
3. Try building the original VirtuaNES standalone to confirm the toolchain works

**Test:** `build.bat` compiles the original VirtuaNES and runs in Citra.

---

## Phase 1 — Copy the core, create the bridge

**Action:**
1. Copy `src/cores/virtuanes/` into `C:\V2NES\3ds_source\cores\virtuanes\`
2. Read existing `src/3ds/3dsemu.h` to identify the public NES core API
3. Create a thin wrapper header `C:\V2NES\3ds_source\nes_core.h` that exposes:
   - `NES_Init()` / `NES_Deinit()`
   - `NES_LoadROM(const char* path)`
   - `NES_Reset()`
   - `NES_EmulateFrame()` — runs one frame of emulation
   - `NES_GetFrameBuffer()` — returns a `u16*` to the rendered NES framebuffer
   - `NES_GetAudioBuffer()` / `NES_GetAudioSize()`
   - `NES_SetInput(u8 nesButtonMask)` — set controller state
   - `NES_GetRAM() u8*` — for map auto-matching
4. Add the new `cores/` directory to the Makefile `SOURCES`

**Test:** Core files compile as part of the V2NES Makefile (no emulation running yet).

---

## Phase 2 — Integrate the 3DS platform layer (video, audio, input)

**Action:**
1. Copy these key 3DS platform files into `C:\V2NES\3ds_source\platform\`:
   - `3dsvideo.cpp` / `.h` — top-screen NES framebuffer display
   - `3dssound.cpp` / `.h` — audio output via DSP
   - `3dsinput.cpp` / `.h` — button mapping
   - `3dsgpu.cpp` / `.h` — GPU texture management
   - `3dsasync.cpp` / `.h` — threading helpers
   - `3dsconfig.cpp` / `.h` — configuration management
2. Adapt `3dsvideo.cpp`:
   - Remove bottom-screen rendering (replace with V2NES)
   - Keep top-screen rendering: upload NES 16-bit framebuffer as a GPU texture each frame
3. Add `platform/` to the Makefile `SOURCES`

**Test:** A stub main loop that initializes the platform layer and renders a black NES frame to the top screen.

---

## Phase 3 — ROM loading + basic emulation loop

**Action:**
1. V2NES's `scanFiles()` already scans `.nes` files in `sdmc:/V2NES/roms/` — confirmed that works
2. When user taps **PLAY** or selects a ROM from **GAME LIST**:
   - Call `NES_LoadROM(path)`, `NES_Reset()`
   - Set `state = STATE_EMULATING` (new GameState entry)
3. In `STATE_EMULATING`, the main loop:
   - Calls `NES_EmulateFrame()` once per frame
   - Gets the framebuffer via `NES_GetFrameBuffer()`
   - Uploads it as a C3D texture for the top screen
   - V2NES bottom screen continues to render normally (map/guide/settings)
4. Add `STATE_EMULATING` to the `GameState` enum in `ui.h`
5. Add the top-screen rendering pipeline to `main.cpp`

**Test:** Select a ROM, see the NES title screen on the top screen (correct video, no input/sound yet).

---

## Phase 4 — Input mapping

**Action:**
1. Map 3DS physical buttons to NES controller bits using the layout settings (Classic/Modern/One-Handed)
2. Read button state each frame via `hidKeysHeld()` / `hidKeysDown()`
3. Build a `u8 nesButtons` bitmask and pass it to `NES_SetInput(nesButtons)` before `NES_EmulateFrame()`
4. Handle Start/Select via 3DS Start/Select (or a configurable combo)

**Test:** Game responds to button presses — can start a game, move around, jump.

---

## Phase 5 — Audio

**Action:**
1. Initialize DSP service (`dspInit()`) and NDSP (`ndspInit()`) alongside existing services in `main()`
2. After each `NES_EmulateFrame()`, read the audio buffer via `NES_GetAudioBuffer()` / `NES_GetAudioSize()`
3. Feed samples into NDSP for playback
4. Handle frame-skip audio sync (VirtuaNES already throttled to ~60 FPS)

**Test:** Audio plays (title screen music, sound effects, no crackling).

---

## Phase 6 — Bottom screen integration

**Action:**
1. During `STATE_EMULATING`, the bottom screen shows:
   - Map overlay if `gameplayMode == GAMEPLAY_MAP`
   - Guide overlay if `gameplayMode == GAMEPLAY_GUIDE`
   - HUD bar with SHOW MAP / SHOW GUIDE / prev / next buttons
2. Wire the existing `nesRAM[0x800]` array to `NES_GetRAM()` so auto-map-switching works with real emulated memory
3. HUD "SHOW MAP" → switches bottom mode to map
4. HUD "SHOW GUIDE" → switches bottom mode to guide
5. D-pad during gameplay: map pan/zoom (MAP mode) or guide scroll (GUIDE mode)

**Test:** Play a game while maps auto-switch and walkthroughs are visible on the bottom screen.

---

## Phase 7 — Save states, cheats, polish

**Action:**
1. Wire L/R buttons to actual save/load state using `3dsasync.cpp` (currently placeholders)
2. Copy `3dscheat.cpp` / `.h` for Game Genie cheat support (`.CHX` files)
3. Copy `3dsconfig.cpp` / `.h` for per-game and global settings (already similar to existing `settings.cpp` — may need merging)
4. Performance tuning:
   - Frame-skip auto-adjustment for old 3DS
   - Screen sizing options (cropped 4:3, cropped fullscreen, etc.)
5. Update version to **v1.6.0**

**Test:** Save/load states work, cheats work, stable 60 FPS on old 3DS for most games.

---

## Files to watch for conflicts

When merging `src/3ds/` files, these have V2NES equivalents that will need careful merging:

| Original `3ds/` file | V2NES equivalent | Conflict |
|----------------------|-----------------|----------|
| `3dsmenu.cpp/h` | `ui.cpp/h`, `main.cpp` | Both define the bottom-screen UI — use V2NES |
| `3dsconfig.cpp/h` | `settings.cpp/h` | Both handle settings — merge or pick one |
| `3dsmain.cpp/h` | `main.cpp` | Both have `main()` — use V2NES's main loop, call into core |
| `3dsinput.cpp/h` | `main.cpp` (input section) | Input reading exists in V2NES for menu — need to add NES button mapping |
| `3dsexit.cpp/h` | — | Service cleanup — add to existing cleanup |

## Build configuration

The Makefile needs these additions:
- `SOURCES` += `cores/virtuanes` and `platform`
- `LIBS` += `-ldsp -lndsp -lctu` (add DSP/NDSP for audio)
