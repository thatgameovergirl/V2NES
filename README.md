# V2NES

**Dual-Screen NES Emulator for Nintendo 3DS**

Play NES games on the top screen while viewing game maps and walkthrough guides on the bottom screen — all with touch controls, auto-map switching, and player-position tracking.

> **Disclaimer:** V2NES does not support piracy. You must dump your own legally purchased NES cartridges to use this emulator. No copyrighted ROMs, maps, or game assets are included in this repository.

## Features

- **Full NES emulation** — 180+ mappers, NDSP audio, CPU/PPU/APU, savestates (3 slots)
- **Bottom-screen maps** — custom `.v2m` format supporting single and multi-tile images with zoom/pan
- **Walkthrough guides** — scrollable `.txt` guides with word-wrap and position bookmarking
- **Auto-map switching** — detects dungeon/level changes via real NES RAM values (Zelda, SMB)
- **Player-position marker** — red dot on overworld maps, calibrated via `.pos` sidecar files
- **Touch screen UI** — aquamarine glow theme with HUD controls and button overlays
- **Map caching** — preloads maps for instant switching, with VRAM-aware eviction
- **Folder-organized browser** — maps, walkthroughs, and ROMs organized by game prefix

## Platform Compatibility

| Platform | Status |
|----------|--------|
| Old 3DS / 3DS XL | Tested and working |
| New 3DS / New 3DS XL | Untested but should work |
| 2DS / New 2DS XL | Untested but should work |
| DS / DSi / DS Lite | Unsupported (requires 3DS hardware) |

## Installation

1. Copy `V2NES.3dsx` to `sdmc:/3ds/` on your SD card
2. Launch via the Homebrew Launcher

## SD Card Setup

Create these folders on your 3DS SD card:

```
sdmc:/3ds/V2NES/maps/         → .v2m map files
sdmc:/3ds/V2NES/walkthroughs/ → .txt guide files
sdmc:/3ds/V2NES/roms/         → .nes ROM files
```

You can also place files in `sdmc:/V2NES/maps/`, `sdmc:/V2NES/walkthroughs/`, and `sdmc:/V2NES/roms/`.

Maps in `.v2m` format are not included in this repository. Use the [companion app](#companion-app) to convert your own images, or download ready-to-use PNG maps from sites like [NESMaps.com](https://nesmaps.com/maps/Zelda/Zelda.html)

## Controls

| Button | Action |
|--------|--------|
| Y / X | Zoom map in / out |
| D-pad | Pan map / scroll guide |
| L + Y / L + X | Load / save state |
| Touch | Navigate menus (PLAY, MAPS, GUIDE, SETTINGS, GAME LIST) |
| B | Go back |

## Companion App

A Windows desktop tool (`companion_app/v2m_compiler.exe`) converts PNG, JPG, and BMP images to `.v2m` map format with:

- Auto-border cropping and nearest-neighbor resizing
- Output scale options (100% to 33%)
- Multi-tile VM\x02 export for large maps
- Map calibration dialog for player-position markers
- Drag-and-drop interface with batch processing

## Building

**Prerequisites:**
- [devkitARM](https://devkitpro.org/) with libctru 2.7.0, citro2d, and citro3d

**Build:**

```
build.bat
```

Output: `3ds_source/V2NES.3dsx`

## Credits

### Core Emulator
- **Norix** (`norix_v`) — original author of [VirtuaNES](http://virtuanes.s1.xrea.com:8080/), the Windows NES emulator this project's CPU, PPU, APU, and mapper cores are derived from
- **bubble2k16** — 3DS port of VirtuaNES ([emus3ds](https://github.com/bubble2k16/emus3ds)), the foundation for V2NES's NES core and platform layer

### Mapper Contributors
- **FCEUX team** — mapper 028 (Action 53), ROM database for auto-patching headers
- **CaH4e3** — mappers 036, 120; CoolBoy, FK23C, KS7030, OneBus, and Yoko board emulation (from fceumm)
- **dragon2snow** & **VirtualNES Plus authors** — mappers 28, 35, 36, 120, 156, 162, 170, 175, 206, 209, 211, 216, 220, 253, and fixes
- **VirtualNESEx authors** — mappers 4, 7, 16, 23, 34, 47, 49, 52, 65, 69, 71, 73, 74, 86, 91, 92, 121, 132, 141, 148, 150, 162, 163, 166-179, 189, 190, 194, 198, 199, 212, 235, 237, 240, 241, 252, and fixes
- **fanoble** — mapper 171; FDC code in mappers 169, 172
- **Xodnizel** — mapper 206

### Audio
- **Mitsutaka Okazaki** — [emu2413](https://github.com/digital-sound-antiques/emu2413) YM2413/OPLL emulator (VRC7 audio)
- **Tatsuyuki Satoh** (fmopl.c from MAME), **mamiya** (s_opl.c from NEZplug), **cisc** (fmgen.cpp), **NARUTO** (fmpac.ill) — references for OPLL emulation

### Libraries & Tools
- **Lode Vandevenne** — [LodePNG](https://github.com/lvandeve/lodepng), PNG decoder used for UI textures (zlib license)
- **[devkitPro](https://devkitpro.org/)** — devkitARM toolchain, libctru, citro2d, citro3d, and build tools
- **smealum** — original [ctrulib](https://github.com/smealum/ctrulib)

### Testing & Community
- **SG6000** — debugging a critical VirtuaNES crash bug
- **GBATemp forum members** — bug reports and testing feedback
- **[Citra Emulator](https://citra-emu.org/) team** — the 3DS emulator used for development testing

### V2NES
- **HristValkyrja** (thatgameovergirl@gmail.com) — author of V2NES, companion app, map system, walkthrough system, and UI

## License

V2NES is licensed under the [GNU General Public License v2.0](LICENSE) or (at your option) any later version. This license is required by the FCEUX-derived mapper code included in the emulator core.
