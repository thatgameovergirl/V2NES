#ifndef UI_H
#define UI_H

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>
#include "content.h"
#include "settings.h"

enum GameState {
    STATE_STARTUP,
    STATE_MENU,
    STATE_SETTINGS,
    STATE_SETTINGS_LAYOUT,
    STATE_SETTINGS_THEME,
    STATE_SETTINGS_SENSITIVITY,
    STATE_GAME_LIST,
    STATE_PLAYING,
    STATE_MAP_LIST,
    STATE_MAP_VIEW,
    STATE_WALK_LIST,
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

    // Settings Category Buttons
    Button themeCatBtn, speedCatBtn;

    // Back button (shared across settings screens)
    Button backBtn;

    // Settings Theme Buttons
    Button themeDarkBtn, themeLightBtn, accentCycleBtn;

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

private:
    // HUD icon sprite sheet (data/hud_icons.png, 128×128, 4×32×32 icons)
    // Loaded via tex3ds+bin2s — no runtime swizzle, instant GPU upload.
    // Icons: 0=prev(<), 1=next(>), 2=map(M), 3=guide(G)
    C3D_Tex           hudIconsTex;
    Tex3DS_Texture    hudIconsT3x;
    C2D_Image         hudIcons[4];
    Tex3DS_SubTexture hudIconSubTexs[4];
    bool              hudIconsReady = false;

    C2D_TextBuf staticTextBuf;
    C2D_Text playText, mapsText, walkText, settingsText, listText, batteryText;
    C2D_Text backText, notifyText;
    C2D_Text themeCatText, speedCatText, versionText;
    C2D_Text hudShowGuideText, hudShowMapText, hudPrevText, hudNextText;
    C2D_Text hudGuideText, hudBackText;
    C2D_Text themeDarkText, themeLightText, accentCycleText;
    C2D_Text zoomSpeedText, panSpeedText, scrollSpeedText;
    
    int batteryPercent;
    u32 batteryColor;
    
    int notifyTimer;
    C2D_TextBuf dynamicTextBuf;

    void drawButton(Button& btn);
};

std::string cleanGameName(const std::string& filename);

#endif





