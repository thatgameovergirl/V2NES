#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <string>
#include <vector>

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

