#include "content.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>

V2Content::V2Content() {}
V2Content::~V2Content() {
    freeAllCachedMaps();
    freeWalk();
}

void V2Content::scanFiles() {
    mapFiles.clear();
    walkFiles.clear();
    romFiles.clear();

    // Scan a folder for files with a given extension, descending one level into
    // subfolders. This lets you organise maps as:
    //   /V2NES/maps/Zelda/Zelda-Overworld.v2m
    //   /V2NES/maps/Mario/Mario-1-1.v2m
    // The filename (not the folder name) is stored, so prefix-matching,
    // auto-switching, and BACK/FOWD navigation all keep working unchanged.
    auto scanFolder = [](const char* path, const char* ext, std::vector<V2File>& list) {
        DIR* dir = opendir(path);
        if (!dir) return;
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            std::string name = ent->d_name;
            if (name == "." || name == "..") continue;
            std::string fullPath = std::string(path) + "/" + name;
            if (name.find(ext) != std::string::npos) {
                // File in the root folder — no group
                list.push_back({name, fullPath, ""});
            } else {
                // ent->d_type is unreliable on 3DS FAT — use stat() to check
                struct stat st;
                bool isDir = (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
                if (!isDir) continue;
                // Subfolder — scan one level deep, tag files with the folder name
                std::string groupName = name;
                DIR* sub = opendir(fullPath.c_str());
                if (sub) {
                    struct dirent* sent;
                    while ((sent = readdir(sub)) != NULL) {
                        std::string sname = sent->d_name;
                        if (sname.find(ext) != std::string::npos) {
                            std::string sPath = fullPath + "/" + sname;
                            list.push_back({sname, sPath, groupName});
                        }
                    }
                    closedir(sub);
                }
            }
        }
        closedir(dir);
    };

    // 1. Scan Maps from both standard and root directories
    scanFolder("sdmc:/V2NES/maps", ".v2m", mapFiles);
    scanFolder("sdmc:/3ds/V2NES/maps", ".v2m", mapFiles);

    // 2. Scan Walkthroughs from both standard and root directories
    scanFolder("sdmc:/V2NES/walkthroughs", ".txt", walkFiles);
    scanFolder("sdmc:/3ds/V2NES/walkthroughs", ".txt", walkFiles);

    // 3. Scan ROMs from various V2NES folders
    scanFolder("sdmc:/V2NES/roms", ".nes", romFiles);
    scanFolder("sdmc:/3ds/V2NES/roms", ".nes", romFiles);
    scanFolder("sdmc:/V2NES", ".nes", romFiles);
    scanFolder("sdmc:/3ds/V2NES", ".nes", romFiles);

    // 4. Fallback to Legacy root folder if either list is empty
    if (mapFiles.empty()) {
        scanFolder("sdmc:/3ds/V2NES", ".v2m", mapFiles);
    }
    if (walkFiles.empty()) {
        scanFolder("sdmc:/3ds/V2NES", ".txt", walkFiles);
    }
}

// Returns unique group names from a file list in encounter order.
// Files with no group (root level) are collected under "Other".
std::vector<std::string> V2Content::getGroups(const std::vector<V2File>& files) {
    std::vector<std::string> groups;
    for (const auto& f : files) {
        std::string g = f.group.empty() ? "Other" : f.group;
        bool found = false;
        for (const auto& existing : groups) {
            if (existing == g) { found = true; break; }
        }
        if (!found) groups.push_back(g);
    }
    return groups;
}

// Append a single line to sdmc:/V2NES/debug.log. Used during map-load
// diagnosis to capture per-tile success/failure that printf can't reach
// from inside Citra or hardware.
static void dbgLog(const char* fmt, ...) {
    FILE* lf = fopen("sdmc:/V2NES/debug.log", "a");
    if (!lf) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(lf, fmt, ap);
    va_end(ap);
    fputc('\n', lf);
    fclose(lf);
}

bool V2Content::loadMap(const char* path) {
    std::string p = path;
    // Cache hit: instant swap, no SD read.
    auto cacheIt = mapCache.find(p);
    if (cacheIt != mapCache.end()) {
        saveMapView();
        mapTiles.clear();
        activateCachedMap(p);
        if (!loadMapView()) resetView(209.0f);
        return mapLoaded;
    }
    // Cache miss: load from disk. If load fails (VRAM exhausted), free every
    // other cached map and retry — we'd rather have the currently-requested
    // map than stale cached ones we may not need. Sacrifices the fast-switch
    // optimization in tight-memory situations.
    if (!cacheMapFromDisk(p)) {
        dbgLog("loadMap: first attempt failed, evicting cache and retrying");
        // Active tiles may belong to a map we're about to free.
        mapTiles.clear();
        mapLoaded = false;
        for (auto& kv : mapCache) {
            for (auto t : kv.second.tiles) {
                if (t) { C3D_TexDelete(&t->tex); delete t; }
            }
            kv.second.tiles.clear();
        }
        mapCache.clear();
        if (!cacheMapFromDisk(p)) {
            dbgLog("loadMap: retry also failed for %s", p.c_str());
            return false;
        }
        dbgLog("loadMap: retry succeeded for %s", p.c_str());
    }
    activateCachedMap(p);
    if (!loadMapView()) resetView(209.0f);
    return mapLoaded;
}

bool V2Content::cacheMapFromDisk(const std::string& path) {
    if (mapCache.find(path) != mapCache.end()) return true;
    freeMap();
    printf("\n[MAP] Loading: %s", path.c_str());
    dbgLog("---- cacheMapFromDisk: %s ----", path.c_str());
    currentMapPath = path;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        printf("\n[MAP] ERROR: Cannot open file!");
        dbgLog("ERROR: fopen failed");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char magic[4] = {0};
    if (fread(magic, 1, 4, f) != 4) {
        fclose(f);
        return false;
    }

    if (memcmp(magic, "VM\x02\0", 4) == 0) {
        fread(&originalWidth, 4, 1, f);
        fread(&originalHeight, 4, 1, f);
        fread(&gridCols, 4, 1, f);
        fread(&gridRows, 4, 1, f);
        u32 format;
        fread(&format, 4, 1, f);

        mapAspect = (float)originalWidth / (float)originalHeight;
        printf("\n[MAP] Tiled: grid=%lux%lu size=%lux%lu aspect=%.3f", (unsigned long)gridCols, (unsigned long)gridRows, (unsigned long)originalWidth, (unsigned long)originalHeight, mapAspect);
        dbgLog("Header: magic=VM\\x02 origW=%lu origH=%lu gridCols=%lu gridRows=%lu format=%lu fileSize=%zu",
               (unsigned long)originalWidth, (unsigned long)originalHeight, (unsigned long)gridCols, (unsigned long)gridRows, (unsigned long)format, fileSize);

        u32 tileSize = 1024 * 1024 * 2; // RGB565 (2 bytes/pixel)
        // format 2 = pre-swizzled by the converter; no tempLinear needed.
        // format 1 = legacy linear data; we swizzle at runtime (backward compat).
        bool preTiled = (format == 2);
        u8* tempLinear = preTiled ? nullptr : (u8*)malloc(tileSize);
        u8* gpuTiled   = (u8*)linearAlloc(tileSize);

        if ((!preTiled && !tempLinear) || !gpuTiled) {
            printf("\n[MAP] ERROR: Out of memory!");
            dbgLog("ERROR: malloc tempLinear=%p linearAlloc gpuTiled=%p", tempLinear, gpuTiled);
            if (tempLinear) free(tempLinear);
            if (gpuTiled) linearFree(gpuTiled);
            fclose(f);
            return false;
        }

        for (u32 r = 0; r < gridRows; r++) {
            for (u32 c = 0; c < gridCols; c++) {
                u32 tileW, tileH;
                fread(&tileW, 4, 1, f);
                fread(&tileH, 4, 1, f);

                if (preTiled) {
                    // Format 2: data is already Morton-swizzled — read straight into
                    // GPU linear memory, no CPU swizzle loop needed.
                    size_t read = fread(gpuTiled, 1, tileSize, f);
                    if (read != tileSize) {
                        printf("\n[MAP] ERROR: Read %zu/%lu bytes on tile %lu,%lu", read, (unsigned long)tileSize, (unsigned long)r, (unsigned long)c);
                        dbgLog("Tile r=%lu c=%lu tileW=%lu tileH=%lu FREAD FAILED: %zu/%lu", (unsigned long)r, (unsigned long)c, (unsigned long)tileW, (unsigned long)tileH, read, (unsigned long)tileSize);
                        linearFree(gpuTiled);
                        fclose(f);
                        return false;
                    }
                } else {
                    // Format 1 (legacy): linear data — swizzle at runtime.
                    size_t read = fread(tempLinear, 1, tileSize, f);
                    if (read != tileSize) {
                        printf("\n[MAP] ERROR: Read %zu/%lu bytes on tile %lu,%lu", read, (unsigned long)tileSize, (unsigned long)r, (unsigned long)c);
                        dbgLog("Tile r=%lu c=%lu tileW=%lu tileH=%lu FREAD FAILED: %zu/%lu", (unsigned long)r, (unsigned long)c, (unsigned long)tileW, (unsigned long)tileH, read, (unsigned long)tileSize);
                        free(tempLinear);
                        linearFree(gpuTiled);
                        fclose(f);
                        return false;
                    }

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

                            u32 blockIndex  = blockY * (1024 / 8) + blockX;
                            u32 tiledOffset = (blockIndex * 64 + morton) * 2;
                            u32 linearOffset = (y * 1024 + x) * 2;

                            gpuTiled[tiledOffset + 0] = tempLinear[linearOffset + 0];
                            gpuTiled[tiledOffset + 1] = tempLinear[linearOffset + 1];
                        }
                    }
                }
                dbgLog("Tile r=%lu c=%lu tileW=%lu tileH=%lu fread=OK preTiled=%d", (unsigned long)r, (unsigned long)c, (unsigned long)tileW, (unsigned long)tileH, (int)preTiled);

                V2Tile* tile = new V2Tile();
                tile->width = tileW;
                tile->height = tileH;
                tile->isRGBA8 = false;

                bool texOK = C3D_TexInit(&tile->tex, 1024, 1024, GPU_RGB565);
                dbgLog("  TexInit r=%lu c=%lu -> %s", (unsigned long)r, (unsigned long)c, texOK ? "OK" : "FAILED");
                if (!texOK) {
                    // VRAM/linear-heap exhausted. tile->tex is uninitialized —
                    // do NOT call C3D_TexDelete on it. Cleanup successful tiles
                    // from THIS map only (those have valid textures), then bail
                    // so the caller can free other cached maps and retry.
                    delete tile;
                    free(tempLinear);
                    linearFree(gpuTiled);
                    fclose(f);
                    for (auto t : mapTiles) {
                        if (t) { C3D_TexDelete(&t->tex); delete t; }
                    }
                    mapTiles.clear();
                    return false;
                }
                C3D_TexSetFilter(&tile->tex, GPU_NEAREST, GPU_NEAREST);
                C3D_TexUpload(&tile->tex, gpuTiled);
                C3D_TexFlush(&tile->tex);

                tile->subTex.width = tileW;
                tile->subTex.height = tileH;
                tile->subTex.left = 0.0f;
                // PICA200 Morton tiling stores y=0 (image top) at V=0 in GPU memory.
                // citro2d maps subTex.top → GPU V = 1 - top, so top=1.0 → GPU V=0 → row 0.
                // bottom = (1024-tileH)/1024 → GPU V = tileH/1024 → content boundary.
                // This correctly clips to the valid tileH×tileW content region.
                tile->subTex.top = 1.0f;
                tile->subTex.right = (float)tileW / 1024.0f;
                tile->subTex.bottom = (float)(1024 - tileH) / 1024.0f;

                tile->image.tex = &tile->tex;
                tile->image.subtex = &tile->subTex;

                mapTiles.push_back(tile);
            }
        }

        if (tempLinear) free(tempLinear);
        linearFree(gpuTiled);
        fclose(f);
        // Move the loaded tiles + metadata into the cache. The caller (loadMap)
        // will activate this entry and reset the view.
        CachedMap cm;
        cm.tiles = mapTiles;
        cm.gridCols = gridCols;
        cm.gridRows = gridRows;
        cm.originalWidth = originalWidth;
        cm.originalHeight = originalHeight;
        cm.aspect = mapAspect;
        mapCache[path] = cm;
        mapTiles.clear();
        mapLoaded = false;
        printf("\n[MAP] SUCCESS: Loaded %u tiles!", (unsigned int)cm.tiles.size());
        dbgLog("DONE: %u tiles cached for %s", (unsigned int)cm.tiles.size(), path.c_str());
        return true;
    } else {
        // Legacy RGBA8 single-tile map or Header16/Header20 V1 format
        fseek(f, 0, SEEK_SET);
        u32 dataSize = 1024 * 1024 * 4;
        u8* tempLinear = (u8*)malloc(dataSize);
        if (!tempLinear) {
            fclose(f);
            return false;
        }

        u32 w = 1024, h = 1024;
        u32 tW = 1024, tH = 1024;

        char tempMagic[4];
        if (fread(tempMagic, 1, 4, f) == 4) {
            if (memcmp(tempMagic, "VM\x01\0", 4) == 0) {
                if (fileSize >= dataSize + 20) {
                    fread(&w, 4, 1, f);
                    fread(&h, 4, 1, f);
                    fread(&tW, 4, 1, f);
                    fread(&tH, 4, 1, f);
                } else {
                    fread(&w, 4, 1, f);
                    fread(&h, 4, 1, f);
                    u32 reserved;
                    fread(&reserved, 4, 1, f);
                }
            } else {
                fseek(f, 0, SEEK_SET);
            }
        }

        size_t read = fread(tempLinear, 1, dataSize, f);
        fclose(f);

        if (read != dataSize) {
            free(tempLinear);
            return false;
        }

        u8* gpuTiled = (u8*)linearAlloc(dataSize);
        if (!gpuTiled) {
            free(tempLinear);
            return false;
        }

        // Swizzle RGBA8
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
                u32 tiledOffset = (blockIndex * 64 + morton) * 4;
                u32 linearOffset = (y * 1024 + x) * 4;

                gpuTiled[tiledOffset + 0] = tempLinear[linearOffset + 3];
                gpuTiled[tiledOffset + 1] = tempLinear[linearOffset + 2];
                gpuTiled[tiledOffset + 2] = tempLinear[linearOffset + 1];
                gpuTiled[tiledOffset + 3] = tempLinear[linearOffset + 0];
            }
        }

        free(tempLinear);

        originalWidth = w;
        originalHeight = h;
        gridCols = 1;
        gridRows = 1;
        mapAspect = (float)w / (float)h;

        V2Tile* tile = new V2Tile();
        tile->width = tW;
        tile->height = tH;
        tile->isRGBA8 = true;

        C3D_TexInit(&tile->tex, 1024, 1024, GPU_RGBA8);
        C3D_TexSetFilter(&tile->tex, GPU_NEAREST, GPU_NEAREST);
        C3D_TexUpload(&tile->tex, gpuTiled);
        C3D_TexFlush(&tile->tex);
        linearFree(gpuTiled);

        tile->subTex.width = tW;
        tile->subTex.height = tH;
        tile->subTex.left = 0.0f;
        tile->subTex.top = (float)tH / 1024.0f;
        tile->subTex.right = (float)tW / 1024.0f;
        tile->subTex.bottom = 0.0f;

        tile->image.tex = &tile->tex;
        tile->image.subtex = &tile->subTex;

        mapTiles.push_back(tile);
        // Move loaded tile + metadata into the cache. Caller will activate.
        CachedMap cm;
        cm.tiles = mapTiles;
        cm.gridCols = gridCols;
        cm.gridRows = gridRows;
        cm.originalWidth = originalWidth;
        cm.originalHeight = originalHeight;
        cm.aspect = mapAspect;
        mapCache[path] = cm;
        mapTiles.clear();
        mapLoaded = false;
        printf("\n[MAP] SUCCESS: Loaded legacy single tile!");
        return true;
    }
}

void V2Content::freeMap() {
    saveMapView();
    // Tile pointers are owned by the cache (mapCache). We only release our
    // active-map references — the cache holds the actual textures alive so
    // re-loading the same map is instant. Use freeAllCachedMaps() to actually
    // delete the textures (e.g. when switching to a different ROM).
    mapTiles.clear();
    mapLoaded = false;
}

void V2Content::freeMapFromCache(const std::string& path) {
    auto it = mapCache.find(path);
    if (it == mapCache.end()) return;
    // Don't free the currently active map
    if (path == currentMapPath) return;
    for (auto tile : it->second.tiles) {
        if (tile) {
            C3D_TexDelete(&tile->tex);
            delete tile;
        }
    }
    it->second.tiles.clear();
    mapCache.erase(it);
}

void V2Content::freeAllCachedMaps() {
    saveMapView();
    mapTiles.clear();
    mapLoaded = false;
    for (auto& kv : mapCache) {
        for (auto tile : kv.second.tiles) {
            if (tile) {
                C3D_TexDelete(&tile->tex);
                delete tile;
            }
        }
        kv.second.tiles.clear();
    }
    mapCache.clear();
}

void V2Content::freeAllCachedMapsExcept(const std::string& keepPath) {
    for (auto it = mapCache.begin(); it != mapCache.end(); ) {
        if (it->first == keepPath) { ++it; continue; }
        for (auto tile : it->second.tiles) {
            if (tile) {
                C3D_TexDelete(&tile->tex);
                delete tile;
            }
        }
        it->second.tiles.clear();
        it = mapCache.erase(it);
    }
}

void V2Content::freeAllCachedMapsExcept(const std::string& keep1, const std::string& keep2) {
    // Keep overworld (keep1) + current dungeon (keep2); evict everything else.
    for (auto it = mapCache.begin(); it != mapCache.end(); ) {
        if (it->first == keep1 || it->first == keep2) { ++it; continue; }
        for (auto tile : it->second.tiles) {
            if (tile) {
                C3D_TexDelete(&tile->tex);
                delete tile;
            }
        }
        it->second.tiles.clear();
        it = mapCache.erase(it);
    }
}

void V2Content::activateCachedMap(const std::string& path) {
    auto it = mapCache.find(path);
    if (it == mapCache.end()) return;
    const CachedMap& cm = it->second;
    mapTiles = cm.tiles;
    gridCols = cm.gridCols;
    gridRows = cm.gridRows;
    originalWidth = cm.originalWidth;
    originalHeight = cm.originalHeight;
    mapAspect = cm.aspect;
    currentMapPath = path;
    mapLoaded = !mapTiles.empty();
    // Automatically load (or clear) the .pos sidecar whenever the active map changes.
    loadPosCalib(path);
}

void V2Content::loadPosCalib(const std::string& mapPath) {
    posCalib = PosCalibration();   // reset to invalid
    if (mapPath.empty()) return;

    // Derive sidecar path: same directory and stem, .pos extension.
    std::string posPath = mapPath;
    size_t dot = posPath.rfind('.');
    if (dot != std::string::npos) posPath = posPath.substr(0, dot);
    posPath += ".pos";

    FILE* f = fopen(posPath.c_str(), "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz <= 0 || sz > 2048) { fclose(f); return; }
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);

    PosCalibration pc;
    // Parse newline-separated "key=value" pairs.
    char* line = strtok(buf, "\n\r");
    while (line) {
        char key[64]  = {0};
        char val[256] = {0};
        if (sscanf(line, "%63[^=]=%255s", key, val) == 2) {
            if      (strcmp(key, "ram_addr")   == 0) pc.ramAddr   = atoi(val);
            else if (strcmp(key, "world_cols") == 0) pc.worldCols = atoi(val);
            else if (strcmp(key, "world_rows") == 0) pc.worldRows = atoi(val);
            else if (strcmp(key, "map_left")   == 0) pc.mapLeft   = (float)atof(val);
            else if (strcmp(key, "map_top")    == 0) pc.mapTop    = (float)atof(val);
            else if (strcmp(key, "map_right")  == 0) pc.mapRight  = (float)atof(val);
            else if (strcmp(key, "map_bottom") == 0) pc.mapBottom = (float)atof(val);
        }
        line = strtok(NULL, "\n\r");
    }
    free(buf);

    if (pc.worldCols > 0 && pc.worldRows > 0) {
        pc.valid = true;
        posCalib = pc;
        dbgLog("loadPosCalib: OK ramAddr=%d grid=%dx%d left=%.4f top=%.4f right=%.4f bottom=%.4f",
               pc.ramAddr, pc.worldCols, pc.worldRows,
               pc.mapLeft, pc.mapTop, pc.mapRight, pc.mapBottom);
    }
}

void V2Content::preloadMatchingMaps(const std::string& prefix) {
    for (size_t i = 0; i < mapFiles.size(); i++) {
        const std::string& name = mapFiles[i].name;
        if (name.compare(0, prefix.size(), prefix) != 0) continue;
        if (mapCache.find(mapFiles[i].path) != mapCache.end()) continue;
        cacheMapFromDisk(mapFiles[i].path);
    }
}

bool V2Content::loadNextMap(const std::string& prefix, size_t& cursor) {
    for (size_t i = cursor; i < mapFiles.size(); i++) {
        cursor = i + 1;
        // Only load maps that belong to the current game prefix
        if (!prefix.empty() && mapFiles[i].name.compare(0, prefix.size(), prefix) != 0) continue;
        if (mapCache.find(mapFiles[i].path) != mapCache.end()) continue;
        // Cap at 8 maps per game. Prefix filter above already ensures we only
        // cache maps for the currently running game, so this is safe.
        if ((int)mapCache.size() >= 8) return false;
        cacheMapFromDisk(mapFiles[i].path);
        return true;
    }
    return false;
}

void V2Content::drawMap(float viewportH, int playerRamVal) {
    if (mapLoaded) {
        float drawScaleX = (320.0f / originalWidth) * zoom;
        float drawScaleY = ((320.0f / mapAspect) / originalHeight) * zoom;
        float tileScreenW = 1024.0f * drawScaleX;
        float tileScreenH = 1024.0f * drawScaleY;

        static std::string lastLoggedPath;
        if (lastLoggedPath != currentMapPath) {
            lastLoggedPath = currentMapPath;
            dbgLog("drawMap vH=%.0f zoom=%.4f panX=%.2f panY=%.2f dSX=%.4f dSY=%.4f origW=%lu origH=%lu",
                   viewportH, zoom, panX, panY, drawScaleX, drawScaleY,
                   (unsigned long)originalWidth, (unsigned long)originalHeight);
        }

        for (u32 r = 0; r < gridRows; r++) {
            for (u32 c = 0; c < gridCols; c++) {
                float tileScreenX = panX + (c * 1024.0f) * drawScaleX;
                float tileScreenY = panY + (r * 1024.0f) * drawScaleY;

                // Frustum cull: skip tiles entirely off-screen
                if (tileScreenX + tileScreenW < 0 || tileScreenX > 320.0f) continue;
                if (tileScreenY + tileScreenH < 0 || tileScreenY > viewportH) continue;

                size_t tileIdx = r * gridCols + c;
                if (tileIdx >= mapTiles.size()) continue;
                V2Tile* tile = mapTiles[tileIdx];
                if (!tile) continue;

                C2D_DrawImageAt(tile->image, tileScreenX, tileScreenY, 0.5f, NULL, drawScaleX, drawScaleY);
            }
        }

        // Player position marker — only shown when calibration is loaded and a
        // valid RAM value is provided. The .pos sidecar maps a NES screen-index
        // to a cell in the overworld grid; we draw a dot at that cell's centre.
        if (posCalib.valid && playerRamVal >= 0) {
            int totalScreens = posCalib.worldCols * posCalib.worldRows;
            if (playerRamVal < totalScreens) {
                int col = playerRamVal % posCalib.worldCols;
                int row = playerRamVal / posCalib.worldCols;

                // Normalized centre of the cell in image space [0,1]
                float cellW = (posCalib.mapRight - posCalib.mapLeft) / (float)posCalib.worldCols;
                float cellH = (posCalib.mapBottom - posCalib.mapTop)  / (float)posCalib.worldRows;
                float normX = posCalib.mapLeft + (col + 0.5f) * cellW;
                float normY = posCalib.mapTop  + (row + 0.5f) * cellH;

                // Convert to bottom-screen pixel coordinates
                float markerX = panX + normX * (float)originalWidth  * drawScaleX;
                float markerY = panY + normY * (float)originalHeight * drawScaleY;

                // Only draw if the marker is actually visible in the viewport
                if (markerX >= 0.0f && markerX <= 320.0f &&
                    markerY >= 0.0f && markerY <= viewportH) {
                    // White halo for contrast, then a red dot on top
                    C2D_DrawCircleSolid(markerX, markerY, 0.61f, 6.0f, C2D_Color32(255, 255, 255, 200));
                    C2D_DrawCircleSolid(markerX, markerY, 0.62f, 4.0f, C2D_Color32(220, 50,  50,  240));
                }
            }
        }
    }
}

void V2Content::zoomIn(float factor, float viewportH) {
    float oldZoom = zoom;
    zoom *= factor;
    if (zoom > 16.0f) zoom = 16.0f;
    
    if (zoom != oldZoom) {
        float scaleFactor = zoom / oldZoom;
        panX = 160.0f - (160.0f - panX) * scaleFactor;
        panY = (viewportH / 2.0f) - ((viewportH / 2.0f) - panY) * scaleFactor;
    }
}

void V2Content::zoomOut(float factor, float viewportH) {
    float oldZoom = zoom;
    zoom /= factor;
    
    float mapHeightAtZoom1 = 320.0f / mapAspect;
    float minZoom = 1.0f;
    if (mapHeightAtZoom1 > viewportH) {
        minZoom = viewportH / mapHeightAtZoom1;
    }
    if (zoom < minZoom) zoom = minZoom;
    
    if (zoom != oldZoom) {
        float scaleFactor = zoom / oldZoom;
        panX = 160.0f - (160.0f - panX) * scaleFactor;
        panY = (viewportH / 2.0f) - ((viewportH / 2.0f) - panY) * scaleFactor;
    }
}

void V2Content::pan(float dx, float dy, float viewportH) {
    panX += dx;
    panY += dy;

    // Constrain panX and panY to keep the map on screen
    float mapWidth = 320.0f * zoom;
    float mapHeight = (320.0f / mapAspect) * zoom;
    
    float minX = -mapWidth + 40.0f;
    float maxX = 320.0f - 40.0f;
    float minY = -mapHeight + 30.0f;
    float maxY = viewportH - 30.0f;

    if (panX < minX) panX = minX;
    if (panX > maxX) panX = maxX;
    if (panY < minY) panY = minY;
    if (panY > maxY) panY = maxY;
}

void V2Content::resetView(float viewportH) {
    // Fit the whole map inside the viewport.
    // At zoom=1 the map always fills the viewport width (320px) exactly.
    // If the map is taller than the viewport at zoom=1, shrink zoom further
    // so the full height also fits.
    float mapHeightAtOne = 320.0f / mapAspect;
    if (mapHeightAtOne <= viewportH) {
        zoom = 1.0f;          // width-fit; map shorter than viewport
    } else {
        zoom = viewportH / mapHeightAtOne;  // height-fit for tall maps
    }
    float mapWidth  = 320.0f * zoom;
    float mapHeight = mapHeightAtOne * zoom;
    panX = (320.0f - mapWidth)  / 2.0f;
    panY = (viewportH - mapHeight) / 2.0f;
    dbgLog("resetView vH=%.0f aspect=%.4f mapH1=%.1f zoom=%.4f mapW=%.1f mapH=%.1f panX=%.2f panY=%.2f",
           viewportH, mapAspect, mapHeightAtOne, zoom, mapWidth, mapHeight, panX, panY);
}

bool V2Content::loadWalk(const char* path) {
    this->currentWalkPath = path;
    freeWalk();
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size > 30000) size = 30000; // Safe upper cap
    fseek(f, 0, SEEK_SET);

    char* raw = (char*)malloc(size + 1);
    if (!raw) {
        fclose(f);
        return false;
    }
    fread(raw, 1, size, f);
    raw[size] = '\0';
    fclose(f);

    // Helper lambda to wrap a single line of text at word boundaries and break long words
    auto wrapLine = [](const std::string& str, std::vector<std::string>& list, size_t maxChars) {
        if (str.empty()) {
            list.push_back("");
            return;
        }
        std::string line = "";
        std::string word = "";

        auto addWord = [&](const std::string& w) {
            std::string tempWord = w;
            while (tempWord.length() > maxChars) {
                if (!line.empty()) {
                    list.push_back(line);
                    line = "";
                }
                list.push_back(tempWord.substr(0, maxChars));
                tempWord = tempWord.substr(maxChars);
            }
            if (line.length() + tempWord.length() + (line.empty() ? 0 : 1) > maxChars) {
                if (!line.empty()) list.push_back(line);
                line = tempWord;
            } else {
                if (!line.empty()) line += " ";
                line += tempWord;
            }
        };

        for (char c : str) {
            if (c == ' ') {
                if (!word.empty()) {
                    addWord(word);
                    word = "";
                }
            } else {
                word += c;
            }
        }
        if (!word.empty()) {
            addWord(word);
        }
        if (!line.empty()) {
            list.push_back(line);
        }
    };

    // Temp buffer for measuring line heights on-the-fly
    C2D_TextBuf measureBuf = C2D_TextBufNew(4096);
    walkTotalHeight = 0;
    walkLineYOffsets.clear();

    std::string currentLine = "";
    for (int i = 0; i < size; i++) {
        char c = raw[i];
        if (c == '\r') {
            continue; // Skip carriage returns
        }
        if (c == '\n') {
            // Wrap to a safe 40 character limit (scale 0.52f fits 100% of the screen width!)
            std::vector<std::string> wrapped;
            wrapLine(currentLine, wrapped, 40);
            
            for (const auto& wLine : wrapped) {
                walkLines.push_back(wLine);
                walkLineYOffsets.push_back(walkTotalHeight);
                
                // Pre-measure height of this wrapped line
                if (measureBuf) {
                    C2D_TextBufClear(measureBuf);
                    C2D_Text text;
                    C2D_TextParse(&text, measureBuf, wLine.c_str());
                    C2D_TextOptimize(&text);
                    float w, h;
                    C2D_TextGetDimensions(&text, 0.52f, 0.52f, &w, &h);
                    walkTotalHeight += (int)h + 4; // Add height and 4px spacing
                } else {
                    walkTotalHeight += 20; // Safe fallback
                }
            }

            currentLine = "";
        } else if (c == '\t') {
            currentLine += "    "; // Translate tabs to spaces
        } else if ((unsigned char)c >= 32 && (unsigned char)c <= 126) {
            currentLine += c; // Safe ASCII only
        }
    }

    if (!currentLine.empty()) {
        std::vector<std::string> wrapped;
        wrapLine(currentLine, wrapped, 40);
        for (const auto& wLine : wrapped) {
            walkLines.push_back(wLine);
            walkLineYOffsets.push_back(walkTotalHeight);
            if (measureBuf) {
                C2D_TextBufClear(measureBuf);
                C2D_Text text;
                C2D_TextParse(&text, measureBuf, wLine.c_str());
                C2D_TextOptimize(&text);
                float w, h;
                C2D_TextGetDimensions(&text, 0.52f, 0.52f, &w, &h);
                walkTotalHeight += (int)h + 4;
            } else {
                walkTotalHeight += 20;
            }
        }
    }

    if (measureBuf) {
        C2D_TextBufDelete(measureBuf);
    }
    free(raw);
    
    walkLoaded = true;
    saveBookmark();
    return true;
}

void V2Content::freeWalk() {
    if (walkLoaded) {
        walkLines.clear();
        walkLineYOffsets.clear();
        walkTotalHeight = 0;
        walkLoaded = false;
    }
}

bool V2Content::preloadMap(const std::string& path) {
    return cacheMapFromDisk(path);
}

void V2Content::drawWalk(int scrollY) {
    if (!walkLoaded || walkLines.empty()) return;

    // Use a small, highly efficient dynamic buffer to draw visible lines only
    static C2D_TextBuf drawBuf = C2D_TextBufNew(8192);
    if (!drawBuf) return;

    C2D_TextBufClear(drawBuf);

    // 1. Find the first visible line index using a simple, blazing-fast loop
    size_t startLine = 0;
    for (size_t i = 0; i < walkLines.size(); i++) {
        // We estimate the bottom of the line is yOffset + 22. 
        // If the bottom is >= scrollY, then this line is the first one visible (or partially visible) in the viewport!
        if (walkLineYOffsets[i] + 22 >= scrollY) {
            startLine = i;
            break;
        }
    }

    // 2. Draw lines starting exactly from the first visible index until we go below the viewport!
    for (size_t i = startLine; i < walkLines.size(); i++) {
        float y = 10.0f + (float)walkLineYOffsets[i] - (float)scrollY;

        // If the line starts below the bottom screen boundaries (209px for HUD), stop immediately!
        if (y > 207.0f) {
            break;
        }

        // Parse and render the line on-the-fly
        C2D_Text text;
        C2D_TextParse(&text, drawBuf, walkLines[i].c_str());
        C2D_TextOptimize(&text);

        C2D_DrawText(&text, C2D_WithColor, 10, y, 0.5f, 0.52f, 0.52f, C2D_Color32(230, 230, 230, 255));
    }
}

int V2Content::getWalkHeight() {
    return walkTotalHeight;
}

// Walk scroll-position sidecar uses the .wpos extension (4-byte binary int).
// Previously .pos, which name-clashed with the new map-calibration sidecar from
// the companion app. They lived in separate directories so never overlapped on
// disk, but the shared extension was a footgun — renamed so the two file types
// are unambiguous from the filename alone.
void V2Content::saveWalkPosition(int currentScroll) {
    if (currentWalkPath.empty()) return;
    std::string wposPath = currentWalkPath;
    size_t dot = wposPath.find_last_of('.');
    if (dot != std::string::npos) wposPath = wposPath.substr(0, dot);
    wposPath += ".wpos";
    FILE* f = fopen(wposPath.c_str(), "wb");
    if (f) {
        fwrite(&currentScroll, sizeof(int), 1, f);
        fclose(f);
    }
}

int V2Content::loadWalkPosition() {
    if (currentWalkPath.empty()) return 0;
    std::string wposPath = currentWalkPath;
    size_t dot = wposPath.find_last_of('.');
    if (dot != std::string::npos) wposPath = wposPath.substr(0, dot);
    wposPath += ".wpos";
    int scroll = 0;
    FILE* f = fopen(wposPath.c_str(), "rb");
    if (f) {
        fread(&scroll, sizeof(int), 1, f);
        fclose(f);
    }
    return scroll;
}

void V2Content::saveBookmark() {
    if (currentWalkPath.empty()) return;
    mkdir("sdmc:/V2NES", 0777);
    FILE* f = fopen("sdmc:/V2NES/bookmark.txt", "wb");
    if (f) {
        fwrite(currentWalkPath.c_str(), 1, currentWalkPath.length(), f);
        fclose(f);
    }
}

std::string V2Content::loadBookmark() {
    FILE* f = fopen("sdmc:/V2NES/bookmark.txt", "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size <= 0 || size > 512) { fclose(f); return ""; }
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return ""; }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    std::string path(buf);
    free(buf);
    return path;
}

void V2Content::saveMapView() {
    if (currentMapPath.empty()) return;
    std::string posPath = currentMapPath;
    size_t dot = posPath.find_last_of('.');
    if (dot != std::string::npos) posPath = posPath.substr(0, dot);
    posPath += ".mapv";
    FILE* f = fopen(posPath.c_str(), "wb");
    if (f) {
        float data[3] = { zoom, panX, panY };
        fwrite(data, sizeof(float), 3, f);
        fclose(f);
    }
}

bool V2Content::loadMapView() {
    if (currentMapPath.empty()) return false;
    std::string posPath = currentMapPath;
    size_t dot = posPath.find_last_of('.');
    if (dot != std::string::npos) posPath = posPath.substr(0, dot);
    posPath += ".mapv";
    FILE* f = fopen(posPath.c_str(), "rb");
    if (f) {
        float data[3];
        bool ok = false;
        if (fread(data, sizeof(float), 3, f) == 3) {
            zoom = data[0];
            panX = data[1];
            panY = data[2];
            ok = true;
        }
        fclose(f);
        return ok;
    }
    return false;
}

void V2Content::saveMapBookmark() {
    if (currentMapPath.empty()) return;
    mkdir("sdmc:/V2NES", 0777);
    FILE* f = fopen("sdmc:/V2NES/bookmark_map.txt", "wb");
    if (f) {
        fwrite(currentMapPath.c_str(), 1, currentMapPath.length(), f);
        fclose(f);
    }
}

std::string V2Content::loadMapBookmark() {
    FILE* f = fopen("sdmc:/V2NES/bookmark_map.txt", "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size <= 0 || size > 512) { fclose(f); return ""; }
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return ""; }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    std::string path(buf);
    free(buf);
    return path;
}
