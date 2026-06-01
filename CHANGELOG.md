# V2NES Project Changelog

All notable changes to the **V2NES 3DS Emulator** and its **Desktop Asset Companion** are documented here.

---

## [1.8.6] - 2026-06-01
### Added
* **Overworld Player Position Marker**:
  * V2NES can now draw a live player-position dot on the overworld map by reading a single byte of NES RAM each frame and looking up the corresponding cell in a calibrated grid. Dungeons and games without calibration are unaffected — the dot only appears when a sidecar exists for the active map.
  * **Sidecar format** — a small text `.pos` file placed next to the `.v2m` map (same folder, same stem). Keys: `ram_addr`, `world_cols`, `world_rows`, `map_left`, `map_top`, `map_right`, `map_bottom` (normalized 0..1 image coords). `game`, `img_width`, `img_height` are written for human reference and ignored by the loader.
  * **3DS side** — `PosCalibration` struct + `loadPosCalib()` in `content.cpp` parse the sidecar on every map switch (silently invalid if no file). `drawMap()` now takes a `playerRamVal` argument and renders a white-haloed red dot (radii 6/4) at the cell centre, with viewport visibility culling and correct scaling under zoom/pan. `main.cpp` reads `nesReadRAM(posCalib.ramAddr)` each frame while `STATE_PLAYING`/`STATE_EMULATING` and feeds it via `ui.playerRamVal`.
  * **Companion app — Map Calibration tool** — new `MapCalibrationDialog` reached from a "CALIBRATE PLAYER MARKER" button on the main form. Workflow: select game preset (Zelda RAM=`$EB` 16×8, Dragon Warrior 120×120, Final Fantasy 256×256, Custom) → load overworld PNG → click top-left then bottom-right corners of the grid → preview shows the grid overlay → Export writes the `.pos` next to the source image. Preset auto-detects from the ROM filename, falling back to the loaded image filename when no ROM was dropped first.
* **Per-Button Aquamarine Glow Overlays**:
  * The four gameplay HUD buttons (BACK, 4WRD, MAPS, WALK) and all five main-menu buttons (PLAY, MAPS, GUIDES, SETTINGS, GAME LIST) now render an aquamarine glow border from pre-built PNG overlays, matching the screen-edge glow theme.
  * Glows are generated as inset thin rounded-rectangle rings (3px margin inside the button bounds, radius 8) so each button fully contains its glow. Colour matches the theme accent (0, 220, 200).
  * New embedded assets: `aquamarine_glow_{back,4wrd,maps,walk}.png` (HUD, sized per button) and `aquamarine_glow_play.png` (240×60) + `aquamarine_glow_menu.png` (115×40, shared by the four equal-sized menu buttons).
  * Added `initSmallGlowTex()` in `ui.cpp` — decodes each PNG, Morton-swizzles into a power-of-2-padded `GPU_RGBA8` texture, and uploads it once at init.

### Changed
* **Glow Buttons Drop the Procedural Border**: `drawButton` gained a `noBorder` flag; HUD and menu buttons skip the old 1px accent outline and white glass-edge highlight, letting the glow overlay be the sole edge treatment.
* **Button Labels Re-Centered**: glow-button labels now use `C2D_AlignCenter` about the button mid-point for reliable horizontal centering.
* **Walkthrough Bookmark Now Restored on Game Launch**: when a ROM starts, the engine checks `bookmark.txt` (already written by `loadWalk` on every walk open) and reopens the last walkthrough the user had for *that specific game*, falling back to the first prefix-matching walk when the bookmark doesn't belong to the launching ROM. Previously `bookmark.txt` was written but never read — the engine always defaulted to the alphabetically first walk for the game.
* **Walk Scroll-Position File Renamed `.pos` → `.wpos`**: the walkthrough scroll-offset sidecar now uses `.wpos` so it can never be confused with the new map-calibration `.pos` sidecar from the companion app. The two file types lived in separate directories so they didn't overlap on disk before, but sharing an extension was a footgun (e.g. a misplaced file would silently overwrite the other). Old `.pos` scroll files become orphaned; they're tiny and can be safely deleted.

### Fixed
* **Glow Texture UV Convention**: `initSmallGlowTex` initially reused the full-screen glow's `top=h/texH, bottom=0` mapping, which shifts a small padded sprite upward by `(texH−h)` px (≈5px on a 27px glow in a 32px texture) — the glow rendered too high with a transparent gap at the bottom. Corrected to the convention used by the working `hudIconSubTexs` (`top=1.0, bottom=(texH−h)/texH`), so glows sit centered and contained. (`main.cpp`'s `initGlowTexture` for the full-screen glow still uses the old convention, where the shift is unnoticeable.)

### Build
* **Makefile Toolchain Paths Made Relocatable**: corrected the hardcoded `/c/V2NES/devkitPro` paths (DEVKITPRO override, `tex3ds.exe` invocation) and `build.bat` to the actual project location, so the project builds from `C:\Projects\V2NES`.
* New glow PNGs added to `DATA_OBJS` with `bin2s` embed rules.

### Companion App v1.5.0 (bumped from v1.4.0)
* **Renamed Calibration Action**: the main-form button "CALIBRATE PLAYER MARKER" is now "CREATE OVERWORLD MAP NAVIGATION MARKER", and the dialog title became "Create Overworld Map Navigation Marker". Wording change only — same workflow, same `.pos` output — clarifies that the sidecar exists purely for overworld map navigation, not for arbitrary in-game markers.
* **Output Folder Field**: the main form now has an "OUTPUT FOLDER" text box (with browse button) just below the staged files list. When set, all `.v2m` map conversions and `.txt` guide copies are written there instead of into the ROM's source directory. The folder is also passed to the Overworld Marker dialog as the default output for the `.pos` file. Leaving the field blank keeps the legacy behaviour (write next to the dropped ROM). `GetOutputDir()` helper centralises the choice so future export sites pick it up automatically.
* **Output Scale Restricted to 75% / 50% / 33%**: the 100% option was removed because real-world overworld images always need downscaling to fit 3DS VRAM — keeping it as a choice routinely produced broken builds. The combo now defaults to 75% and the case mapping in `GetSelectedScale()` was renumbered (0=75, 1=50, 2=33).
* **RESET Button**: a new "RESET (clear staged files)" button below the marker action clears the staged-file list, the detected ROM name/path, and the output folder so a fresh batch can be queued without quitting and relaunching the app. The log is preserved.
* **Form Resized**: window grew from 740 → 790 px tall to accommodate the new output-folder row and reset button.

---

## [1.8.5] - 2026-05-23
### Added
* **Two-Level Folder Navigation for Maps and Guides**:
  * Maps and walkthroughs can now be organised into subfolders named after each game (e.g. `/V2NES/maps/Zelda/`, `/V2NES/walkthroughs/Mario/`).
  * Tapping MAPS or WALK now shows a "Select Game" folder list first, then drops into that game's files. If only one folder exists the folder step is skipped automatically.
  * D-pad + A navigates both levels; B goes one level back. Auto-switching, BACK/FOWD cycling, and prefix matching all continue to work unchanged since they operate on filenames, not folder names.
  * Fixed: `ent->d_type` is unreliable on the 3DS FAT filesystem (always returns `DT_UNKNOWN`). Replaced with `stat()` for correct directory detection.
* **Touch-to-Pause**:
  * Tapping anywhere on the bottom screen while a game is running now pauses emulation and opens the map/guide UI — identical to the L+R hotkey. HUD buttons still respond normally on the same tap.

### Changed
* **Glow Border — Hinge-Aware Split**:
  * Both glow overlay PNGs rebuilt as 320×240 with the hinge-facing edge removed: the top screen asset has no bottom glow line; the bottom screen asset has no top glow line. The left and right borders connect across the physical hinge, giving the illusion of one continuous glowing rectangle.
  * Glow is now hidden during active emulation (`STATE_EMULATING`) — it only appears on menus, lists, and the paused map/guide view.
* **Save State Flash Replaced with Glow Overlay**:
  * The 4px blue line border that flashed on save/load has been removed. The bottom screen glow now pulses at full z-depth (in front of all UI) for the same duration, giving a cleaner save confirmation.
* **HUD Forward Button Renamed**: `FOWD` → `4WRD`.
* **D-Pad Map Panning Inverted**:
  * D-pad direction now moves the map in the natural direction — press left and the map scrolls left. Previously the map moved in the opposite direction of the input.

### Fixed
* **V2NES Logo Centering**:
  * Left crop edge shifted 5px inward (x=22 → x=27) so the artwork is visually centred on the top screen. Draw width updated to match.

---

## [1.8.4] - 2026-05-23
### Fixed
* **Emulation Speed Restored to Full 60 fps**:
  * Reverted the background emulation thread (core 1) introduced in 1.8.3 — Old 3DS core 1 is too slow for VirtuaNES and caused a consistent frame-rate drop with audibly slow music.
  * Restored the `lastEmuTime` / `framesDue` timing catch-up loop so the NES always runs at real-time speed even when individual render iterations exceed 16 ms (e.g. complex map on the bottom screen). Audio is submitted after each catch-up frame to prevent sample loss.
  * Texture upload moved back inside the render block (before `DrawImageAt`) — eliminates the 1-frame pipeline lag introduced by the threading rework.
* **FOWD Button Text Now Visible**:
  * The HUD forward button label was set to `"FWD"` in init but `drawButton` checked for `"FOWD"` — the strcmp never matched so no text was drawn. Fixed the label and widened the button from 46 px to 60 px (scale 0.4f → 0.45f).
  * FOWD text moved from per-frame dynamic parsing to a one-time static parse at init, matching the other HUD labels.
* **B Button No Longer Closes the Game from the Bottom Screen**:
  * Pressing B while in `STATE_PLAYING` (bottom-screen map/guide view during a game) now returns to `STATE_EMULATING` (top screen) instead of exiting the game entirely.
  * Map zoom and pan position are preserved on this transition — `resetView` is now guarded so it only fires when actually returning to the main menu.

### Changed
* **HUD Buttons Renamed and Resized for Finger Use**:
  * `<` → **BACK**, `>` → **FOWD**, `M` → **MAPS**, `G` → **WALK**.
  * All four buttons now fill the full HUD bar height (27 px tall) and are significantly wider: BACK 54 px, FOWD 60 px, MAPS 52 px, WALK 60 px. Previously all four were 24×25 px icon-only tiles that were too small to reliably tap.
  * Sprite-sheet icon rendering removed; labels are text-only, pre-parsed into the static text buffer at init.
* **Battery Centered, Clock Removed from HUD**:
  * During gameplay the battery percentage is now centered in the HUD bar. The clock has been removed to reduce clutter.
* **Single Theme — Aquamarin Wave**:
  * Removed all theme and accent colour options from the Settings screen. The app now ships with a single hardcoded dark theme (Aquamarin Wave) — no user-facing toggle needed.
* **Aquamarine Glow Border Overlay**:
  * Both screens now display a translucent aquamarine glow border rendered from a pre-built PNG overlay. The same 320×240 image is used on both screens — drawn at x=40 on the 400px top screen so the left/right edges align perfectly with the 320px bottom screen, forming one continuous glowing rectangle across the hinge. Bottom-screen glow is drawn behind all HUD buttons so it never covers touch targets.

### Improved
* **Dirty-Frame Flag Skips Redundant Texture Uploads**:
  * The NES framebuffer is only Morton-swizzled and uploaded to the GPU when `nesFrameChanged()` reports a new frame. During menus, lists, and any state where the NES isn't actively rendering, this saves ~3 ms per iteration.
* **Audio Crackle Reduction**:
  * Fractional sample accumulator (367/368 alternation) eliminates the 30-sample/sec deficit that caused periodic pops at 22 050 Hz / 60 fps.
  * Three silent buffers are pre-filled into the NDSP queue at game start, providing ~50 ms of cushion against scheduling jitter.
* **`framesDue` Capped to 2**:
  * Prevents the emulation loop from running more than two catch-up frames per iteration, avoiding multi-frame stutter spikes after heavy bottom-screen work.

---

## [1.8.3] - 2026-05-23
### Added
* **Live RAM Debug Overlay**:
  * Top-right corner of the top screen now shows free application RAM at all times (green, `osGetMemRegionFree`).
  * When a ROM is running a three-line variant also shows NES register values (`$10`, `$EB`) and the frame counter / success flag (yellow), providing a real-time readout of emulator health and memory pressure.
* **Background Emulation Thread (Core 1)** *(reverted in 1.8.4)*:
  * Moved `nesEmulateFrame()` to a background thread on CPU core 1. Reverted due to Old 3DS performance regression — see 1.8.4 fix notes.

---

## [1.8.2] - 2026-05-23
### Added
* **D-pad Navigation in List Screens**:
  * Map List, Guide List, and Game List now respond to `↑`/`↓` for cursor movement, `A` to confirm, and `B` to go back.
  * The cursor position is shown with a brighter accent highlight, distinct from the dimmer "currently loaded" indicator.
  * Lists initialise with the cursor on the first item (or on the active item when re-entering from gameplay).

### Changed
* **Logo Zoomed to Near Full Screen Height**:
  * Scale updated from `320/469` to `390/469`. The logo now renders at ~212px tall on the 240px top screen (5px side margins) in all states where it appears (startup splash, main menu, loading screen).
* **Theme Button Cycles Colours Directly — No Sub-Screen**:
  * Pressing `[ THEME ]` in Settings now cycles through all 8 accent colours instantly without opening a sub-page. The button background is tinted with the active accent so the current selection is always visible.
  * The colour name appears on the top-screen notification bar for 2 seconds after each change.
* **M and G HUD Icons 1pt Larger**:
  * Mode icons (M = map, G = guide) increased from 18px to 19px, with centering recalculated from button bounds.

### Removed
* **Layout Setting Removed**:
  * `[LAYOUT]` button, `STATE_SETTINGS_LAYOUT` sub-screen, and all three layout variant buttons (Classic / Modern / One-Handed) removed from Settings.

---

## [1.8.1] - 2026-05-20
### Added
* **Branded Loading Screen with Embedded Logo**:
  * New `STATE_LOADING_GAME` shown the moment the user taps Play or selects a game.
  * Embedded 512×256 Castlevania-themed V2NES logo (PNG baked into `.3dsx` via `bin2s`, decoded at startup with lodepng and Morton-swizzled into a `GPU_RGBA8` texture).
  * "Loading Maps & Guides..." status line on the top screen below the logo; "Loading Maps & Guides..." mirrored on the bottom screen.
  * One-frame render gate ensures the loading screen is *visible* during the blocking SD reads — without it the touch handler would fire the load on the same iteration and the user would never see the screen.
* **All Maps Preloaded Per Game**:
  * `V2Content::preloadMatchingMaps(prefix)` walks every map file matching the active ROM's prefix and caches each tile set into VRAM-friendly textures once.
  * `loadMap` becomes O(1) pointer-swap for any cached path — auto-switching between overworld and dungeons is now instant with no SD read, no Morton swizzle, no audio dropout.
  * Cache is keyed on the full file path and freed via `freeAllCachedMaps()` when the user exits to the menu (so memory is reclaimed between games).
* **NES Warmup After Load**:
  * After the heavy preload completes, the emulator silently emulates 30 NES frames before the loading screen is dismissed, so the title screen is already drawn in the framebuffer when the user first sees the top screen. Eliminates the brief black/boot frame.
* **Gameplay-Active Gate**:
  * New `V2UI::nesGameplayActive` flag — latches `true` once `$00EB` (Zelda 1 screen index) becomes non-zero, indicating Link is actually in the world.
  * Bottom screen hides the map and shows "Waiting for game to start..." until the flag latches, so the overworld isn't visible during the NES title sequence / file select.
* **Auto-Switch Debounce**:
  * `updateRAMMapSwitching` now requires the watched RAM byte to hold a new value for 60 host frames (~1 second) before triggering a switch. Filters out transient flickers during scene transitions.

### Changed
* **NDSP-Driven Emulation Loop**:
  * Emulation pacing now follows the NDSP audio ring buffer state rather than a wall-clock catch-up counter. Each loop iteration fills whatever NDSP slots have just been consumed (capped at 3 per iteration).
  * NDSP plays at a fixed 22050 Hz, so the NES is now locked to that clock — eliminates the "audio speeds up and slows down" artifact caused by bursty wall-clock submissions filling the ring buffer, getting dropped, then producing time-compressed catch-up samples.
* **Maps Open Zoomed-In by Default**:
  * `V2Content::resetView` now starts at `zoom = 3.0` instead of `1.0`. Loading the map fully zoomed out was a GPU draw spike that pulled host frame rate below the audio buffer fill rate. Frustum culling at zoom 3 keeps off-screen tiles out of the draw list.
  * Every successful `loadMap` (cache hit or disk read) now calls `resetView` automatically.

### Fixed
* **Zelda 1 RAM Address Reverted to `$0010`**:
  * `$0606` was incorrect — it varies during overworld play and triggered phantom dungeon switches. The on-screen debug RAM viewer confirmed `$10` correctly tracks dungeon ID (0 = overworld, 1–9 = dungeons), matching the original convention.
* **Map Switch No Longer Freezes the Game**:
  * Combined with the preload + cache changes, mid-game auto-switches no longer block on SD reads. The 0.5–2 s freeze on entering a dungeon is gone.

---

## [1.8.0] - 2026-05-23
### Fixed
* **Maps Remember Zoom and Position**:
  * `loadMap()` now calls `loadMapView()` after activating a cached map; only falls back to `resetView` if no `.mapv` file exists. Applies to both cache-hit and cache-miss paths. Pan and zoom are preserved when switching between maps and returning to a previously viewed map.
* **Map Zoom Step Reduced**:
  * Per-press zoom factor lowered — Slow: `1.10→1.07`, Medium: `1.25→1.15`, Fast: `1.40→1.25`. Each tap zooms in/out less aggressively.
* **Guide Scrolling in Standalone Walk View**:
  * Added a maximum scroll cap (`getWalkHeight() - 200`) to `STATE_WALK_VIEW`. Previously the guide could be scrolled past all content, leaving a blank screen that appeared as broken scrolling.
* **Guide Remembers Reading Position**:
  * `saveWalkPosition()` is now called when pressing B to exit walk view and when quitting to the main menu from gameplay.
  * `loadWalkPosition()` is now called whenever a guide is loaded — manual selection from the list, auto-matched on game start, and fallback. The guide opens at the last saved scroll position instead of always resetting to the top.
* **HUD Bar Taller — M and G Buttons Larger**:
  * HUD bar height increased from 26px to 31px (`y=209..240`). All four HUD buttons are now 25px tall (was 20px), giving more tap surface area to M, G, `<`, and `>`.
* **M and G Buttons Separated by 10px**:
  * M moved to `x=258`, G to `x=292`. The two mode buttons now have a clear 10px gap between them.
* **`<` and `>` Arrows Separated by 10px**:
  * `>` moved from `x=30` to `x=38`. The two nav arrows now have a clear 10px gap between them.
* **NES Output Shifted 3px Left on Top Screen**:
  * Draw position changed from `x=40` to `x=37`, bringing the left edge of the NES image 3px closer to the left bezel and better aligning the active area with the 320px bottom screen.
* **X and Y Buttons No Longer Control NES in Emulation Mode**:
  * `KEY_X → NES_BTN_A` and `KEY_Y → NES_BTN_B` mappings removed from the emulation input block. X and Y now only function as hotkeys (save/load state, map zoom) and do not interfere with NES games.
* **Loading Progress Bar Moved to Bottom Screen**:
  * The progress bar and status text ("Loading ROM… / Caching Maps… / Finalizing…") are now rendered on the bottom screen during `STATE_LOADING_GAME` using the active accent colour for the fill. The top screen is uncluttered, showing only the logo.

---

## [1.7.7] - 2026-05-23
### Added
* **"App loading..." on Bottom Screen During Startup**:
  * While the splash logo is displayed on the top screen, the bottom screen now shows a centered "App loading..." hint so the user knows the app is initialising rather than frozen.
* **Map-Caching Progress Bar**:
  * A progress bar appears below the V2NES logo on the top screen during `STATE_LOADING_GAME`.
  * Dark background track spans the full 320px content area (x=40–360); bright blue fill advances left-to-right as each map is cached (0% while the ROM loads, proportional during map caching, 100% at finalize).

### Changed
* **Logo Zoomed to Fill Content Area**:
  * The logo SubTexture is now cropped to the 469×255 content region, removing the 21px black margins that were part of the 512×256 atlas padding.
  * Scale updated from `320/512` to `320/469`, increasing rendered size from 292×160px to 320×174px — about 10% larger with no wasted black border.

---

## [1.7.6] - 2026-05-23
### Changed
* **Gameplay HUD Bar Redesigned**:
  * Navigation arrows (`<` / `>`) anchored to the left side of the bar.
  * Map (`M`) and Guide (`G`) mode buttons moved to the right side.
  * Battery percentage and current time (`HH:MM`) now fill the centre — battery retains its green/yellow/red colour coding; time uses the active theme text colour.
* **HUD Buttons Use Sprite Sheet Icons**:
  * All four gameplay bar buttons (prev, next, M, G) now render icon images instead of text labels.
  * Icons are stored in a single 128×128 RGBA PNG sprite sheet (`data/hud_icons.png`, four 32×32 cells).
  * The sheet is converted to a PICA200-compatible `.t3x` file by `tex3ds` at build time (Makefile rule added), then embedded via `bin2s`. At runtime `Tex3DS_TextureImport` uploads the pre-tiled data in one fast transfer — no CPU decode loop, no Morton swizzle, eliminating the frame stall that affected the previous image-button attempt.

---

## [1.7.5] - 2026-05-23
### Added
* **Startup Splash Screen**:
  * App now opens with a `STATE_STARTUP` screen showing the V2NES logo on the top screen for ~3 seconds before advancing to the main menu.
  * Any button press skips the splash after a 30-frame minimum (prevents instant-skip from keys held during boot).
* **Logo on Top Screen During Menu and Loading**:
  * The V2NES logo is displayed on the top screen in all non-gameplay states: startup splash, main menu (while the bottom screen shows the PLAY/MAPS/GUIDES buttons), and game loading.
  * Loading state still shows a status line ("Loading ROM… / Caching Maps… / Finalizing…") below the logo.

### Changed
* **NES Output Constrained to 320×240 Content Area**:
  * Top screen rendering — NES game output, logo, and all overlays — is now drawn within a centred 320×240 region (x=40 to x=360), leaving 40px strips on each side reserved for bezels to be added in a future update.
  * Previously the NES frame was stretched to the full 400px top screen width.
* **Logo asset replaced with `logo_black.png`**:
  * Switched from the RGBA transparent logo to a pre-composited black-background version, eliminating any transparency rendering artefacts and removing the need for an explicit background fill before drawing.
  * Source image (2760×1504) is resized to fit within a 512×256 GPU texture (aspect-ratio preserved, padded with black) at build time via the existing `bin2s` pipeline.

---

## [1.7.4] - 2026-05-23
### Fixed
* **All Dungeon Maps Displaying Correctly (UV Coordinate Bug)**:
  * Dungeon maps with tile height < 1024 px were rendering with the top portion cut off and a black band at the bottom. Root cause: `subTex.top` was set to `tileH / 1024.0f` instead of `1.0f`.
  * The PICA200 GPU stores Morton-tiled texture data with y=0 (image top) at V=0. citro2d maps `subTex.top` → GPU V via `1 − top`, so `top=1.0` correctly reaches GPU V=0 (image row 0). With the old formula, `top=0.773` (for a 792-px-tall tile) mapped to GPU V=0.227, placing image row 232 at the screen top — silently discarding the first 232 rows.
  * Fixed: `subTex.top = 1.0f` and `subTex.bottom = (1024 − tileH) / 1024.0f` for every tile, correctly cropping to the valid content region regardless of tile height.
  * Overworld tiles (tileH = 1024) are mathematically unchanged: `top=1.0`, `bottom=0.0`.
* **Map Fit-to-Viewport Using Correct Viewport Height**:
  * `resetView` was called with the default 240 px (full bottom screen), but the gameplay viewport is 214 px (HUD bar occupies the bottom 26 px). A compensating `adjustedPanY` hack in `drawMap` over-corrected for tall maps, causing 13 px of the map to be clipped from both top and bottom.
  * Fixed: `resetView(214.0f)` called at all load sites; `adjustedPanY` compensation removed from `drawMap`. Maps now fill the 214 px gameplay viewport exactly with no clipping.

---

## [1.7.3] - 2026-05-20
### Fixed
* **Auto-Map Switching Reads Real NES RAM**:
  * `updateRAMMapSwitching` previously read from a standalone `nesRAM[]` array in `main.cpp` that was never connected to the running emulator — only the Select+D-pad debug shortcut wrote to it. The game's actual dungeon transitions were invisible to the auto-switcher.
  * Added `nesReadRAM(addr)` to nes_core that reads directly from VirtuaNES's CPU RAM (`RAM[addr & 0x07FF]`). Zelda dungeon detection (`$0010`) and SMB world detection (`$075C`) now respond to real in-game state changes.

---

## [1.7.2] - 2026-05-20
### Fixed
* **Audio Tempo & Pan Speed (NONBLOCK Side Effect)**:
  * Without V-sync, the main loop ran at ~200 Hz, causing music to play 3× too fast and the map pan to whip across the screen.
  * Added a manual 60 Hz wall-clock cap via `svcSleepThread` at the end of the loop — keeps NONBLOCK's advantage (no drop-to-30 V-sync penalty) while maintaining correct real-time speed for audio, input, and map navigation.

---

## [1.7.1] - 2026-05-20
### Performance
* **Bottom Screen Map Frustum Culling**:
  * `drawMap` now skips tiles whose screen rectangle falls entirely outside the 320×viewport viewport, eliminating wasted GPU draw calls for large multi-tile overworld maps.
  * Removed per-frame `printf` debug output from the map render hot path.
* **NES Frame Cache (Top Screen)**:
  * Morton swizzle and texture upload now happen only when `nesEmulateFrame` actually produces a new frame, not every host iteration. Skips redundant work when the game is paused (browsing maps/guides) or running below 60 FPS.
* **NONBLOCK Render Submission**:
  * Switched `C3D_FrameBegin` from `SYNCDRAW` to `NONBLOCK` with a guarded return-value check, preventing the V-sync drop-to-30 FPS lock when a frame goes slightly over 16.67 ms.
* **Audio Path Cleanup**:
  * `nesGetAudio` now writes APU samples directly into the NDSP ring buffer instead of going through an intermediate buffer + memcpy.
  * Removed the unused `GSPGPU_FlushDataCache` call on audio buffers (DSP cache flush is the only one needed).

---

## [1.7.0] - 2026-05-20
### Added
* **NES Audio Playback via NDSP**:
  * Full APU audio output routed through the 3DS NDSP sound service using a DSP firmware dump (extracted via Rosalina).
  * Mono PCM16 at 22050 Hz on NDSP channel 10 with an 8-slot ring buffer for smooth, gapless playback.
  * Wall-clock decoupled audio feed ensures correct music tempo independent of host frame rate.
* **Play Button Resumes Last Game**:
  * Saves the last played ROM path to `bookmark_rom.txt`; pressing Play loads it automatically with matching maps and guides.
  * Falls back to the first available ROM if the bookmark is missing or invalid.
* **Clean Game Names in Game List**:
  * ROM filenames are cleaned for display: strips `.nes` extension, region tags `(USA)`, `[!]`, and trailing `, The` suffixes (e.g. `Legend of Zelda, The (USA).nes` → `Legend of Zelda`).
* **Persistent Top Screen During Navigation**:
  * NES top screen keeps rendering the game frame while browsing maps, guides, or selection screens, instead of going dark.
* **Save States (3 Slots)**:
  * Functional VirtuaNES `SaveState`/`LoadState` integration — slots stored as `.st1` / `.st2` / `.st3` next to the ROM.
  * Slot selector with cycling: `L + Up` advances to the next slot (1→2→3→1), `L + Down` walks backwards.
* **Top Screen Notification Overlay**:
  * Semi-transparent banner centered vertically on the top screen for save/load events: "State N Saved!", "State N Loaded!", "Slot N Selected", "No State N Found!"
  * 2-second display, drawn above the NES frame at depth 0.9.
* **Revised Hotkey Scheme**:
  * `L + R` — Toggle between top screen (NES) and bottom screen (maps/guides).
  * `L + Up` / `L + Down` — Cycle save state slot (1–3).
  * `L + Y` — Save state to current slot.
  * `L + X` — Load state from current slot.
  * All combos require L held first, eliminating conflicts between save/load and screen toggle.

### Fixed
* **Audio Tempo Fix (Top Screen Emulation)**:
  * Decoupled NES emulation timing from host frame rate using wall-clock catch-up loop (up to 3 frames per iteration), ensuring music and sound effects play at correct tempo even when rendering drops below 60 FPS.
  * Added NDSP ring buffer status checks before audio submission — prevents overwriting buffers still being played, eliminating garbled/slow audio artifacts during frame dips.
  * Removed duplicate `AUDIO_SAMPLES` macro definition that shadowed the correct value.
* **Top Screen Vertical Crush Fix**:
  * Corrected subtexture UV coordinates to sample only 240 of the 256-row GPU texture, fixing the ~6% vertical compression that squished all NES sprites and text.
* **Input Conflict During Emulation**:
  * B button no longer exits to menu while NES is running — passes through as NES B button.
  * Start button no longer quits the application while NES is running — passes through as NES Start button.
* **Auto-Map Switching During Emulation**:
  * RAM-based map switching (overworld/dungeon detection) now runs in both `STATE_EMULATING` and `STATE_PLAYING`, so the correct map loads automatically when starting a game.

---

## [1.6.0] - 2026-05-19
### Added
* **Multi-Tile Lossless Slicing System (Companion App & 3DS Client)**:
  * Desktop companion app slices large map files into a lossless grid of `1024x1024` tiles at 100% native resolution instead of downscaling.
  * Encodes tiles in hardware-friendly 16-bit **`RGB565`** format (2MB per tile) to reduce VRAM footprint by 50% for maximum 3DS memory stability.
  * 3DS client parses Version 2 (`VM\x02\0`) files, swizzles `RGB565` data for the GPU, and maps them side-by-side seamlessly using sub-textures.
* **Unlocked 16x Pixel-Perfect Zoom**:
  * Unlocked map zoom range up to `16.0f` to allow full 1:1 pixel-perfect alignment.
  * Configured `GPU_NEAREST` filtering to keep retro pixel art clean and sharp without blurring.
* **Persistent Map Coordinate Fixes**:
  * Resolved a critical issue where the global menu `B` button handler unconditionally cleared map pan offsets and zoom.
  * Added automatic state saving to `.mapv` files when closing maps or pressing `B` to leave the map screen.
  * Fixed `loadMapView()` in `content.cpp` to correctly assign loaded `panX` and `panY` coordinate offsets instead of only reading the zoom level.
* **NES Top Screen Rendering Pipeline**:
  * Implemented full NES emulation on the 3DS top screen using the VirtuaNES core (CPU, PPU, APU, 180+ mappers).
  * Compact 256×256 buffer extraction from PPU's 512-pitch framebuffer.
  * Morton Z-order swizzling for PICA200 GPU compatibility (8×8 tile tiling).
  * Citro2D top screen render target with correct subtexture UV convention (V-flipped, map-style).
  * Top screen renders at 400×240 (NES 256×240 scaled to fill).
  * Controller input mapping (A/B/X/Y/D-Pad/Start/Select to NES buttons).
  * Bottom screen HUD (maps/guides) works during emulation.
  * Documented in `TOPSCREEN_TROUBLESHOOTING.md`.

---

## [1.5.2] - 2026-05-19
### Added
* **Map Bookmark**:
  * Saves the last opened map path to `sdmc:/V2NES/bookmark_map.txt` on every map load.
  * Tapping "MAPS" from the main menu auto-opens the bookmarked map directly into the viewer.
  * Falls back to the file list if the bookmarked map was removed.
* **Map View Persistence**:
  * Per-map zoom level saved to `.mapv` files; pan resets to center on open to avoid black bars.
  * View saved on B press in MAP_VIEW, B press in PLAYING, HUD "SHOW MAP" exit, and HUD prev/next map switch.
* **Guide Scroll Position Restore**:
  * When picking a guide from the file list, scroll position is now restored from the `.pos` file instead of resetting to 0.
* **Drag-to-Pan Stale Touch Fix**:
  * Resets the stylus drag state (`wasTouching`) on every new `KEY_TOUCH` event, preventing the map from jumping to a stale offset when entering MAP_VIEW from the menu.

## [1.5.1] - 2026-05-19
### Added
* **Guide Bookmark Persistence**:
  * Automatically saves the last opened guide path to `sdmc:/V2NES/bookmark.txt` on every guide load.
  * Tapping "GUIDES" from the main menu auto-opens the bookmarked guide directly into the viewer, with scroll position restored.
  * Survives device power-off, switching maps, and changing games.
  * Falls back to the file list if the bookmarked file was removed.

## [1.5.0] - 2026-05-18
### Added
* **Dynamic Theme & Accent Engine (3DS Client)**:
  * Implemented full interactive settings toggle interface for swapping between **Light Theme** (Warm Parchment style) and **Dark Theme** (Charcoal/Navy style).
  * Built custom color mapping representing all 8 original Nintendo 3DS hardware chassis colors (including Aquamarin Wave, Shimmer Red, Yellow lemon, Neox Green, Dusty Purple, Crazy Orange, Pink glamour, Old'e Responsible).
  * Programmed tactile selections that automatically highlight the active Control Layout and Active Theme with the chosen 3DS hardware accent colors in real-time.
* **Tactile Savestate Border Flash (3DS Client)**:
  * Programmed a responsive 4px screen outline that flashes the active accent color for exactly 5 frames upon Quick Save (L) or Quick Load (R), providing seamless gameplay confirmations.

## [1.4.0] - 2026-05-18
### Added
* **Interactive Modal Compilation Engine (Companion App)**:
  * Re-programmed the assignment dialog to remain open during compilation, housing a native progress-bar display.
  * Added visual progress indicators that scale and update dynamically as each map completes.
  * Integrated comprehensive error handling that gracefully aborts and alerts the user with structured popups and red/emerald terminal log reporting.
  * Preserved full manual pixel-perfect downscaling skip control for optimal graphic resolution.
* **Unified v1.4.0 Version Tags**:
  * Set version tags across 3DS footer rendering and Desktop Asset Companion GUI to **v1.4.0**.

## [1.3.0] - 2026-05-18
### Added
* **Aspect-Ratio Preserving Letterboxing/Pillarboxing (Companion App)**:
  * Computes proportional scaling vectors for wide or tall map images.
  * Centers and pads the map with black pixels inside the 3DS power-of-two `1024x1024` texture box, perfectly preserving the original aspect ratio and eliminating all horizontal/vertical squishing.
* **Alphanumeric Suffix & Auto-Overworld Mapping (Companion App)**:
  * Allows any alphanumeric level names (e.g. `Level1`, `Dungeon`, `Overworld`) as suffixes instead of restricting filenames to numbers.
  * Automatically maps `Overworld` suffixes (case-insensitive) to clean base `.v2m` game names, matching emulator startup defaults natively.
* **Calibration-Safe Delayed Touchscreen Engine (3DS Client)**:
  * Implemented a deferred stylus coordinate check that filters out initial calibration frame `0,0` coordinate bugs on real 3DS hardware.
  * Triggers buttons instantly and smoothly the microsecond valid coordinates are registered.
* **Matching ROM Auto-Guides Loader (3DS Client)**:
  * Automatically searches and loads the `.txt` guide file matching the game ROM prefix immediately upon clicking PLAY, removing the need to choose manually.
* **Crash-Safe Power Service Integration (3DS Client)**:
  * Replaced the protected, restricted `mcu::hwc` battery query service with the standard, public, and completely safe `ptm:u` (Power Management) service, resolving startup crash screen exceptions on all 3DS hardware.
* **Aligned Versioning**:
  * Set version tags across C++ client HUD and C# Companion GUI window title to **v1.3.0**.

## [1.2.0] - 2026-05-18
### Added
* **Premium C# Desktop Companion GUI (`v2m_compiler.exe`)**:
  * Brand-new standalone Windows Forms dark-themed native desktop utility.
  * Single drag-and-drop zone that accepts your ROM, Map images, and Guides together.
  * Intelligent ROM detection that locks the sanitized game title name-key automatically.
  * Proportional crop-scan boundaries and high-quality Bilinear Resizing to exactly 1024x1024 binary `.v2m` files.
  * Direct one-click folder deployment saving assets right next to your target game ROM.
* **Virtualized 60 FPS Guides Viewer (3DS Client)**:
  * Implemented an **O(1) dynamic rendering virtualization engine** that caches exact line y-offsets on load.
  * Blazing-fast viewport indexing that only ever parses and renders the 10–12 lines currently visible on the screen.
  * Completely eliminates CPU rendering bottlenecks, keeping guide scrolling buttery smooth at a constant 60 FPS.
* **Bulletproof Word Wrap & Tab Alignment**:
  * Implemented C++ margin-bound word-wrapping that automatically chops and fits long text or lines at a strict **40-character limit** (perfect for scale `0.52f`).
  * Resolves all vertical line overlapping issues natively.
  * Auto-sanitizes files on load (completely strips `\r` carriage returns and translates `\t` tab indentations into 4 spaces).
* **Renamed UI Rebrand**:
  * Rebranded all user-visible dashboard buttons, notifications, and menu titles from **"Walkthrough/Walkthru"** to **"Guides"** across both the 3DS and companion app interfaces.
  * Updated version tags displayed inside the emulator to **v1.2.0**.

---

## [1.1.0] - 2026-05-15
### Added
* **Interactive Multitasking HUD Overlay**:
  * Elegant semi-transparent glass panel HUD on the bottom screen during gameplay.
  * Quick-access touch controls: **`SHOW MAP`** and **`SHOW GUIDE`**.
  * Hot-swappable asset navigation list selectors accessible during gameplay.
* **Smart Level Suffix Grouping**:
  * Seamless multi-map selector utilizing filename suffixes (e.g. `Legend_of_Zelda_The_USA-1-1.v2m`) to dynamically link and cycle relevant level assets.
  * Smooth D-Pad scrolling and pan support (stylus drag).
* **Layout Presets**:
  * Integrated three configuration layout presets (**Classic**, **Modern**, and **One-Handed**) saved dynamically into the local JSON configuration structure.

---

## [1.0.0] - 2026-05-14
### Added
* **Initial Fork of V2NES**:
  * Preserved the original VirtuaNES emulation core while splitting execution pipelines to utilize the 3DS bottom screen for secondary asset layers.
* **PICA200 Tiling Engine**:
  * Hardware-accelerated software Z-order Morton curve pixel swizzling to convert linear `.v2m` binary textures directly into Citra GPU memory.
  * Dynamic memory management clearing caches between game load states.
