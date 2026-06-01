# V2NES Project Changelog

All notable changes to the **V2NES 3DS Emulator** and its **Desktop Asset Companion** are documented here.

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
