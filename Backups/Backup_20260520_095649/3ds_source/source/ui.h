#ifndef UI_H
#define UI_H

#include <3ds.h>
#include <citro2d.h>
#include "content.h"
#include "settings.h"

enum GameState {
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
    STATE_EMULATING
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
    Button layoutCatBtn, themeCatBtn, speedCatBtn;

    // Settings Layout Buttons
    Button layout1Btn, layout2Btn, layout3Btn, backBtn;

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

private:
    C2D_TextBuf staticTextBuf;
    C2D_Text playText, mapsText, walkText, settingsText, listText, batteryText;
    C2D_Text layout1Text, layout2Text, layout3Text, backText, notifyText;
    C2D_Text layoutCatText, themeCatText, speedCatText, versionText;
    C2D_Text hudShowGuideText, hudShowMapText, hudPrevText, hudNextText;
    C2D_Text themeDarkText, themeLightText, accentCycleText;
    C2D_Text zoomSpeedText, panSpeedText, scrollSpeedText;
    
    int batteryPercent;
    u32 batteryColor;
    
    int notifyTimer;
    C2D_TextBuf dynamicTextBuf;

    void drawButton(Button& btn);
};

#endif





