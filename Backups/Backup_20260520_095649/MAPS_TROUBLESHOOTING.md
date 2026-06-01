# V2NES Map System Documentation & Troubleshooting Guide

This guide details the technical implementation of the **V2NES Multi-Tile pixel-perfect map system** to serve as a reference in case maps break or if you need to modify the rendering pipeline in the future.

---

## 1. Technical Overview

To display massive retro maps (like the Zelda Overworld at `4096x1408` pixels) on the Nintendo 3DS without losing sharpness:
1. **No Downscaling / Lossless Slicing**: The C# Companion App slices the large map into a grid of `1024x1024` tiles at its original resolution instead of downscaling.
2. **RGB565 Hardware Format**: Rather than using heavy `RGBA8` (4 bytes per pixel), pixel data is stored in native `RGB565` format (2 bytes per pixel). This reduces VRAM and memory footprint by **50%** (exactly `2,097,152` bytes per tile).
3. **Sub-texture Mapping**: Empty canvas padding at the right and bottom edges of the tiles is excluded dynamically via texture coordinates (`Tex3DS_SubTexture`).
4. **Unlocked Zoom**: Zoom levels are unlocked up to `16.0f` to allow players to zoom in to a crisp, 1:1 pixel-perfect scale.

---

## 2. V2M Binary File Format Specification (`Version 2`)

Each `.v2m` file is a custom binary payload structured as follows:

### Main Header (24 bytes)

| Offset | Type | Field Name | Description |
|---|---|---|---|
| `0x00` | `char[4]` | `magic` | Identifier bytes: `V`, `M`, `0x02`, `0x00` |
| `0x04` | `uint32_t` | `originalWidth` | Width of the cropped active map area |
| `0x08` | `uint32_t` | `originalHeight` | Height of the cropped active map area |
| `0x0C` | `uint32_t` | `gridCols` | Number of columns in the tile grid |
| `0x10` | `uint32_t` | `gridRows` | Number of rows in the tile grid |
| `0x14` | `uint32_t` | `format` | Color format flag: `1` = `RGB565` |

### Tile Payloads (Repeated for `gridRows * gridCols`)

Tiles are stored sequentially from left-to-right, top-to-bottom. Each tile has:

| Relative Offset | Type | Field Name | Description |
|---|---|---|---|
| `0x00` | `uint32_t` | `tileW` | Width of the active image content in this tile |
| `0x04` | `uint32_t` | `tileH` | Height of the active image content in this tile |
| `0x08` | `uint8_t[2097152]` | `pixelData` | Raw `1024x1024` `RGB565` bytes (`2,097,152` bytes) |

---

## 3. Emulator Rendering Pipeline (`content.cpp`)

### A. Memory swizzling for 3DS PICA200 GPU
The 3DS GPU requires textures to be swizzled in **Morton Z-order** 8x8 blocks. The linear file bytes are rearranged using the following swizzling formula:

```cpp
for (u16 y = 0; y < 1024; y++) {
    for (u16 x = 0; x < 1024; x++) {
        u16 blockX = x / 8;
        u16 blockY = y / 8;
        u8 pixelX = x % 8;
        u8 pixelY = y % 8;

        u32 morton = 0;
        morton |= (pixelX & 1) << 0;
        morton |= (pixelY & 1) << 1;
        morton |= (pixelX & 2) << 1;
        morton |= (pixelY & 2) << 2;
        morton |= (pixelX & 4) << 2;
        morton |= (pixelY & 4) << 3;

        u32 blockIndex = blockY * (1024 / 8) + blockX;
        u32 tiledOffset = (blockIndex * 64 + morton) * 2; // RGB565 (2 bytes/pixel)
        u32 linearOffset = (y * 1024 + x) * 2;

        gpuTiled[tiledOffset + 0] = tempLinear[linearOffset + 0];
        gpuTiled[tiledOffset + 1] = tempLinear[linearOffset + 1];
    }
}
```

### B. Clipping Unused Padding
To draw only the active region (`tileW` x `tileH`) of each `1024x1024` hardware texture, the subtexture UV coordinates are adjusted:
```cpp
tile->subTex.width = tileW;
tile->subTex.height = tileH;
tile->subTex.left = 0.0f;
tile->subTex.top = 1.0f;
tile->subTex.right = (float)tileW / 1024.0f;
tile->subTex.bottom = 1.0f - (float)tileH / 1024.0f;
```

---

## 4. Map View Persistence (Save/Load State)

To prevent the map view coordinates and zoom from resetting when navigating menus, games, or shutting down the 3DS, V2NES implements view state serialization:

### A. Saved File Path
* The coordinates are saved on the SD card next to the active `.v2m` file, using the `.mapv` extension.
* For example: `sdmc:/V2NES/maps/zelda.v2m` saves its state to `sdmc:/V2NES/maps/zelda.mapv`.

### B. Serialization Data Structure
The state is written as a binary array of 3 single-precision floating-point numbers (12 bytes total):
1. `data[0]` (float): `zoom` level
2. `data[1]` (float): `panX` offset
3. `data[2]` (float): `panY` offset

### C. Flow Control & Event Triggers
1. **Writing/Saving**:
   * Triggered inside `V2Content::freeMap()`, ensuring that whenever a map is closed, switched, or when the app is shut down, the active view state is saved to the SD card.
   * Triggered in `main.cpp` when pressing the `B` button to leave the standalone map view (`STATE_MAP_VIEW`), saving the state instantly.
2. **Reading/Loading**:
   * Triggered after calling `loadMap()` in `main.cpp`.
   * If `loadMapView()` returns `true` (indicating a saved `.mapv` was found and successfully read), the code **skips** the default `resetView()` call, preserving the position.
   * If it returns `false`, `resetView()` is called to center the map.

---

## 5. Troubleshooting Steps

If maps fail to load, look distorted, or don't align properly:

1. **Verify Map file size**:
   * A single-tile map or legacy map will be exactly `4,194,320` bytes (header + `RGBA8` data).
   * A multi-tiled `Version 2` map will be `24 + N * (8 + 2097152)` bytes. For the Zelda Overworld (8 tiles), this is exactly `16,777,280` bytes.

2. **Verify Header Output**:
   * When loading a map, check the console output. It should say:
     `[MAP] Tiled: grid=4x2 size=4096x1408 aspect=2.909`
   * If it says `Loaded legacy single tile!`, the companion app was not compiled properly, or the old conversion script was run.

3. **Compilation Command Check**:
   * If compiling the desktop companion app fails, execute this manually in Windows CMD/PowerShell from the `c:\V2NES\companion_app\` folder:
     `C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe /target:winexe /out:v2m_compiler.exe v2m_compiler.cs`
