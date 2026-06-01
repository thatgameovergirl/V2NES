# V2NES Companion App

A Windows desktop tool for converting map images and walkthrough guides into formats compatible with the V2NES 3DS emulator, and for creating overworld navigation markers.

---

## Requirements

- Windows 7 or later
- No installation needed — just run `v2m_compiler.exe`

---

## Getting Started

1. Run `v2m_compiler.exe`
2. Drag and drop your files into the drop zone (or click to browse):
   - **NES ROM** (`.nes` / `.fds`) — sets the game title for all exported files
   - **Map images** (`.png`, `.jpg`, `.bmp`) — overworld or level maps
   - **Walkthrough guides** (`.txt`) — plain text guides
3. Set your **Output Folder** (where converted files will be saved). Leave blank to save next to your ROM.
4. Choose an **Output Scale**:
   - **75%** — good quality, recommended for most maps
   - **50%** — half resolution, recommended for large overworld maps
   - **33%** — smallest file size, lower quality
5. Click **CONVERT & DEPLOY ASSETS**

Converted files are named automatically using the ROM title (e.g. `Legend_of_Zelda_The_USA.v2m`).

---

## Map Naming — Important

For V2NES to auto-switch maps when you enter a dungeon or level, your map files must follow this naming pattern:

```
GameTitle.v2m              → overworld map
GameTitle-Level1.v2m       → dungeon / level 1
GameTitle-Level2.v2m       → dungeon / level 2
```

When you drop multiple map images, the app will ask you to assign a suffix (Overworld, Level 1–9, or custom) to each one.

---

## Where to Put the Files on Your 3DS SD Card

```
sdmc:/3ds/V2NES/maps/GameTitle/          → .v2m map files
sdmc:/3ds/V2NES/walkthroughs/GameTitle/  → .txt guide files
sdmc:/3ds/V2NES/roms/                    → .nes ROM files
```

---

## Overworld Map Navigation Marker

This feature adds a live red dot to your overworld map in V2NES showing where your character is.

### Steps

1. Click **CREATE OVERWORLD MAP NAVIGATION MARKER**
2. Select your game preset (Zelda, Dragon Warrior, Final Fantasy, or Custom)
3. Click **LOAD MAP IMAGE** and open the same PNG you used to create the overworld `.v2m`
4. Click the **top-left corner** of the first screen cell on the map grid
5. Click the **bottom-right corner** of the last screen cell
6. Verify the cyan grid overlay lines up with the map grid
7. Click **EXPORT .POS FILE**

### Where to Put the .pos File

Place the `.pos` file in the **same folder** as the overworld `.v2m` file, with the **same filename stem**:

```
sdmc:/3ds/V2NES/maps/GameTitle/GameTitle.v2m   ← your map
sdmc:/3ds/V2NES/maps/GameTitle/GameTitle.pos   ← marker sidecar
```

The marker will only appear on maps that have a matching `.pos` file. Dungeon maps and games without a `.pos` file are unaffected.

---

## Reset

Click **RESET (clear staged files)** to clear all dropped files and start a fresh batch without restarting the app.

---

## Supported File Formats

| Type | Formats |
|------|---------|
| ROMs | `.nes`, `.fds` |
| Maps | `.png`, `.jpg`, `.bmp` |
| Guides | `.txt` |

ZIP, HTML, and WEBP files are not supported.
