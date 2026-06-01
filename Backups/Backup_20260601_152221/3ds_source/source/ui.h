#ifndef UI_H
#define UI_H

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <string>
#include <vector>
#include "content.h"
#include "settings.h"

enum GameState {
    STATE_STARTUP,
    STATE_MENU,
    STATE_SETTINGS,
    STATE_SETTINGS_SENSITIVITY,
    STATE_GAME_LIST,
    STATE_PLAYING,
    STATE_MAP_FOLDER_LIST,   // two-level nav: pick a game folder
    STATE_MAP_LIST,          // then pick a map within that folder
    STATE_MAP_VIEW,
    STATE_WALK_FOLDER_LIST,  // two-level nav: pick a game folder
    STATE_WALK_LIST,         // then pick a guide within that folder
    STATE_WALK_VIEW,
    STATE_EMULATING,
    STATE_LOADING_GAME
};

enum GameplayMode {
    GAMEPLAY_MAP,
    GAMEPLAY_GUIDE
};

struct Button {
    float x, y, w, h;
    const char* label;
    u32 color;
    bool pressed;
};

class V2UI {
public:
    V2UI();
    ~V2UI();

    void init();
    void renderBottom(GameState state, V2Settings& settings);
    void showNotification(const char* text);
    void updateBattery(int percent);
    bool checkTouch(touchPosition touch, Button& btn);

    u32 activeBgColor;
    u32 activeBtnColor;
    u32 activeTextColor;
    u32 activeAccentColor;
    u32 activeBtnHlColor;

    // Main Menu Buttons
    Button playBtn, mapsBtn, walkBtn, settingsBtn, listBtn;

    // Settings Buttons
    Button speedCatBtn, backBtn;

    // Settings Sensitivity/Speed Buttons
    Button zoomSpeedBtn, panSpeedBtn, scrollSpeedBtn;

    V2Content content;
    int scrollOffset = 0;
    int selectedIndex = -1;
    int activeMapIndex = -1;
    int activeWalkIndex = -1;
    int activeRomIndex = -1;
    int saveLoadFlashTimer = 0;
    u32 pulseCounter = 0;

    // Two-level folder navigation
    std::string selectedGroup;              // which game folder is currently open
    std::vector<std::string> folderGroups;  // cached group list for the current scan

    // Gameplay Mode & HUD Touch Buttons
    GameplayMode gameplayMode = GAMEPLAY_MAP;
    Button hudShowMapBtn, hudShowGuideBtn, hudPrevBtn, hudNextBtn;
    Button hudBackBtn; // Back button for full map/walk views

    // True once the running NES game has reached active gameplay (past title/menus).
    // The bottom-screen map is hidden until this is set so we don't show the
    // overworld during the intro/file-select sequence.
    bool nesGameplayActive = false;

    // Loading progress passed in from main.cpp each frame so renderBottom can
    // draw the progress bar on the bottom screen during STATE_LOADING_GAME.
    int hudLoadStep  = 0;
    int hudLoadTotal = 0;

    // Current NES RAM value for the player-position marker.
    // Set by main.cpp each frame (read from posCalib.ramAddr via nesReadRAM).
    // -1 means "no marker" (no calibration loaded or not in gameplay).
    int playerRamVal = -1;

private:
    // HUD icon sprite sheet (data/hud_icons.png, 128×128, 4×32×32 icons)
    // Loaded via tex3ds+bin2s — no runtime swizzle, instant GPU upload.
    // Icons: 0=prev(<), 1=next(>), 2=map(M), 3=guide(G)
    C3D_Tex           hudIconsTex;
    Tex3DS_Texture    hudIconsT3x;
    C2D_Image         hudIcons[4];
    Tex3DS_SubTexture hudIconSubTexs[4];
    bool              hudIconsReady = false;

    // Per-button glow overlays (aquamarine_glow_{back,fowd,maps,walk}.png)
    C3D_Tex           hudGlowTex[4];
    C2D_Image         hudGlowImg[4];
    Tex3DS_SubTexture hudGlowSubTex[4];
    bool              hudGlowReady[4] = {false, false, false, false};

    // Main-menu button glow overlays.
    // Index 0 = PLAY (240x60), 1 = the four 115x40 buttons (maps/walk/settings/list).
    C3D_Tex           menuGlowTex[2];
    C2D_Image         menuGlowImg[2];
    Tex3DS_SubTexture menuGlowSubTex[2];
    bool              menuGlowReady[2] = {false, false};

    C2D_TextBuf staticTextBuf;
    C2D_Text playText, mapsText, walkText, settingsText, listText, batteryText;
    C2D_Text backText, notifyText;
    C2D_Text speedCatText, versionText;
    C2D_Text hudShowGuideText, hudShowMapText, hudPrevText, hudNextText;
    C2D_Text hudGuideText, hudBackText;
    C2D_Text zoomSpeedText, panSpeedText, scrollSpeedText;
    
    int batteryPercent;
    u32 batteryColor;
    
    int notifyTimer;
    C2D_TextBuf dynamicTextBuf;

    void drawButton(Button& btn, bool noBorder = false);
};

std::string cleanGameName(const std::string& filename);

#endif





