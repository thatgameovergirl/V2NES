#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <string>
#include <vector>
#include <map>

struct V2File {
    std::string name;
    std::string path;
};

struct V2Tile {
    C3D_Tex tex;
    C2D_Image image;
    Tex3DS_SubTexture subTex;
    u32 width;
    u32 height;
    bool isRGBA8;
};

class V2Content {
public:
    V2Content();
    ~V2Content();

    void scanFiles();
    
    std::vector<V2File> mapFiles;
    std::vector<V2File> walkFiles;
    std::vector<V2File> romFiles;

    // Map Viewing
    bool loadMap(const char* path);
    void freeMap();
    // Pre-load every map file whose name starts with `prefix` into the cache.
    // Subsequent loadMap calls for those paths are instant (no SD read, no swizzle).
    void preloadMatchingMaps(const std::string& prefix);
    bool loadNextMap(const std::string& prefix, size_t& cursor);
    // Release every cached map and free their GPU textures. Call when switching games.
    void freeAllCachedMaps();
    // Free all cached maps except the one at the given path (keeps active map alive).
    void freeAllCachedMapsExcept(const std::string& keepPath);
    // Free a single map from the cache (GPU texture + memory). Used by auto-switcher
    // to reclaim VRAM when moving between levels that won't be revisited soon.
    void freeMapFromCache(const std::string& path);
    void drawMap(float viewportH = 240.0f);
    void zoomIn(float factor = 1.25f, float viewportH = 240.0f);
    void zoomOut(float factor = 1.25f, float viewportH = 240.0f);
    void pan(float dx, float dy, float viewportH = 240.0f);
    void resetView(float viewportH = 240.0f);
    std::string currentMapPath;
    void saveMapView();
    bool loadMapView();
    void saveMapBookmark();
    std::string loadMapBookmark();

    // Walkthrough Viewing
    bool loadWalk(const char* path);
    std::string currentWalkPath;
    void saveWalkPosition(int currentScroll);
    int loadWalkPosition();
    void freeWalk();
    void drawWalk(int scrollY);
    int getWalkHeight();

    // Preload a map by path (caches it without activating). Used to prioritize
    // the active map's GPU memory allocation before other maps.
    bool preloadMap(const std::string& path);

    // Bookmark
    void saveBookmark();
    std::string loadBookmark();



private:
    // Texture for the map
    C3D_Tex mapTex;
    C2D_Image mapImage;
    Tex3DS_SubTexture mapSubTex;
    bool mapLoaded = false;
    float mapAspect = 1.0f;
    u32 targetW = 1024;
    u32 targetH = 1024;

    std::vector<V2Tile*> mapTiles;
    u32 gridCols = 1;
    u32 gridRows = 1;
    u32 originalWidth = 1024;
    u32 originalHeight = 1024;

    // Cached maps for instant switching. Each entry owns its V2Tile* pointers
    // (the GPU textures stay alive until freeAllCachedMaps is called).
    struct CachedMap {
        std::vector<V2Tile*> tiles;
        u32 gridCols = 1, gridRows = 1;
        u32 originalWidth = 1024, originalHeight = 1024;
        float aspect = 1.0f;
    };
    std::map<std::string, CachedMap> mapCache;

    // Internal: load a map from disk and put it in the cache. Returns true on success.
    // Does NOT change the active map.
    bool cacheMapFromDisk(const std::string& path);
    // Internal: point the active-map members at a cached entry.
    void activateCachedMap(const std::string& path);

    // Zoom and Pan state
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = -40.0f;

    // Walkthrough line storage
    std::vector<std::string> walkLines;
    std::vector<int> walkLineYOffsets;
    int walkTotalHeight = 0;
    bool walkLoaded = false;
};

