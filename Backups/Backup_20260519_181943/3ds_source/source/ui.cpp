#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Colors
#define CLR_BG        C2D_Color32(20, 20, 25, 255)
#define CLR_BTN       C2D_Color32(45, 45, 60, 255)
#define CLR_BTN_HL    C2D_Color32(70, 70, 90, 255)
#define CLR_TEXT      C2D_Color32(255, 255, 255, 255)
#define CLR_ACCENT    C2D_Color32(0, 150, 255, 255)

// Static Helpers for Rounded Rectangles and Icon Drawing
static void drawRoundedRect(float x, float y, float w, float h, float r, u32 color) {
    C2D_DrawRectSolid(x + r, y, 0.5f, w - 2*r, h, color);
    C2D_DrawRectSolid(x, y + r, 0.5f, w, h - 2*r, color);
    C2D_DrawCircleSolid(x + r, y + r, 0.5f, r, color);
    C2D_DrawCircleSolid(x + w - r, y + r, 0.5f, r, color);
    C2D_DrawCircleSolid(x + r, y + h - r, 0.5f, r, color);
    C2D_DrawCircleSolid(x + w - r, y + h - r, 0.5f, r, color);
}

static void drawRoundedOutline(float x, float y, float w, float h, float r, float thickness, u32 color) {
    // Draw 4 straight sides
    C2D_DrawRectSolid(x + r, y, 0.5f, w - 2*r, thickness, color); // Top
    C2D_DrawRectSolid(x + r, y + h - thickness, 0.5f, w - 2*r, thickness, color); // Bottom
    C2D_DrawRectSolid(x, y + r, 0.5f, thickness, h - 2*r, color); // Left
    C2D_DrawRectSolid(x + w - thickness, y + r, 0.5f, thickness, h - 2*r, color); // Right

    // Draw 4 smooth 90-degree corners using 6 line segments each
    int segments = 6;
    // Top-Left corner
    for (int i = 0; i < segments; i++) {
        float a1 = M_PI + (i * (M_PI / 2.0f) / segments);
        float a2 = M_PI + ((i + 1) * (M_PI / 2.0f) / segments);
        C2D_DrawLine(x + r + cosf(a1)*r, y + r + sinf(a1)*r, color, x + r + cosf(a2)*r, y + r + sinf(a2)*r, color, thickness, 0.5f);
    }
    // Top-Right corner
    for (int i = 0; i < segments; i++) {
        float a1 = 1.5f*M_PI + (i * (M_PI / 2.0f) / segments);
        float a2 = 1.5f*M_PI + ((i + 1) * (M_PI / 2.0f) / segments);
        C2D_DrawLine(x + w - r + cosf(a1)*r, y + r + sinf(a1)*r, color, x + w - r + cosf(a2)*r, y + r + sinf(a2)*r, color, thickness, 0.5f);
    }
    // Bottom-Left corner
    for (int i = 0; i < segments; i++) {
        float a1 = M_PI - (i * (M_PI / 2.0f) / segments);
        float a2 = M_PI - ((i + 1) * (M_PI / 2.0f) / segments);
        C2D_DrawLine(x + r + cosf(a1)*r, y + h - r + sinf(a1)*r, color, x + r + cosf(a2)*r, y + h - r + sinf(a2)*r, color, thickness, 0.5f);
    }
    // Bottom-Right corner
    for (int i = 0; i < segments; i++) {
        float a1 = 0.0f + (i * (M_PI / 2.0f) / segments);
        float a2 = 0.0f + ((i + 1) * (M_PI / 2.0f) / segments);
        C2D_DrawLine(x + w - r + cosf(a1)*r, y + h - r + sinf(a1)*r, color, x + w - r + cosf(a2)*r, y + h - r + sinf(a2)*r, color, thickness, 0.5f);
    }
}

static void drawIcon(const char* label, float x, float y, u32 accentColor, u32 textColor, u32 hollowBgColor) {
    int ix = (int)x;
    int iy = (int)y;

    if (strcmp(label, "MAPS") == 0) {
        // High-resolution orthogonal road grid map (20x20)
        // Background card
        drawRoundedRect(ix + 1, iy + 1, 18, 18, 2.0f, textColor);
        
        // Vertical road (hollow block)
        C2D_DrawRectSolid(ix + 8, iy + 1, 0.5f, 4, 18, hollowBgColor);
        // Horizontal road (hollow block)
        C2D_DrawRectSolid(ix + 1, iy + 8, 0.5f, 18, 4, hollowBgColor);
        
        // Location Pin (Solid Circle + Center Dot)
        C2D_DrawCircleSolid(ix + 10, iy + 10, 0.5f, 4.0f, accentColor);
        C2D_DrawCircleSolid(ix + 10, iy + 10, 0.5f, 1.5f, textColor);
    }
    else if (strcmp(label, "GUIDES") == 0) {
        // High-resolution orthogonal document sheet (20x20)
        // Background card
        drawRoundedRect(ix + 2, iy + 1, 16, 18, 2.0f, textColor);
        
        // Text lines in the document
        C2D_DrawRectSolid(ix + 5, iy + 5, 0.5f, 10, 2, hollowBgColor);
        C2D_DrawRectSolid(ix + 5, iy + 9, 0.5f, 10, 2, hollowBgColor);
        C2D_DrawRectSolid(ix + 5, iy + 13, 0.5f, 10, 2, accentColor); // Highlight line
    }
    else if (strcmp(label, "SETTINGS") == 0) {
        // High-resolution modern settings sliders (20x20)
        // Slider Track 1
        C2D_DrawRectSolid(ix + 1, iy + 5, 0.5f, 18, 2, textColor);
        // Slider Thumb 1 (Circle + Center Dot)
        C2D_DrawCircleSolid(ix + 6, iy + 6, 0.5f, 4.0f, accentColor);
        C2D_DrawCircleSolid(ix + 6, iy + 6, 0.5f, 1.5f, hollowBgColor);

        // Slider Track 2
        C2D_DrawRectSolid(ix + 1, iy + 13, 0.5f, 18, 2, textColor);
        // Slider Thumb 2 (Circle + Center Dot)
        C2D_DrawCircleSolid(ix + 14, iy + 14, 0.5f, 4.0f, accentColor);
        C2D_DrawCircleSolid(ix + 14, iy + 14, 0.5f, 1.5f, hollowBgColor);
    }
    else if (strcmp(label, "GAME LIST") == 0) {
        // High-resolution list icon (20x20)
        // 3 square bullet points in accent color
        C2D_DrawRectSolid(ix + 2, iy + 3, 0.5f, 4, 4, accentColor);
        C2D_DrawRectSolid(ix + 2, iy + 9, 0.5f, 4, 4, accentColor);
        C2D_DrawRectSolid(ix + 2, iy + 15, 0.5f, 4, 4, accentColor);

        // 3 horizontal text lines in text color
        C2D_DrawRectSolid(ix + 8, iy + 4, 0.5f, 10, 2, textColor);
        C2D_DrawRectSolid(ix + 8, iy + 10, 0.5f, 10, 2, textColor);
        C2D_DrawRectSolid(ix + 8, iy + 16, 0.5f, 10, 2, textColor);
    }
}

V2UI::V2UI() {
    batteryPercent = 100;
    batteryColor = C2D_Color32(0, 255, 0, 255);
    notifyTimer = 0;
    activeMapIndex = -1;
    activeWalkIndex = -1;
    activeRomIndex = -1;
    pulseCounter = 0;
}

V2UI::~V2UI() {
    C2D_TextBufDelete(staticTextBuf);
    C2D_TextBufDelete(dynamicTextBuf);
    C2D_Fini();
    C3D_Fini();
}

void V2UI::init() {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    staticTextBuf = C2D_TextBufNew(4096);
    dynamicTextBuf = C2D_TextBufNew(512);

    // Initialize Main Buttons
    playBtn     = { 40, 40, 240, 60, "PLAY", CLR_BTN, false };
    mapsBtn     = { 40, 110, 115, 40, "MAPS", CLR_BTN, false };
    walkBtn     = { 165, 110, 115, 40, "GUIDES", CLR_BTN, false };
    settingsBtn = { 40, 160, 115, 40, "SETTINGS", CLR_BTN, false };
    listBtn     = { 165, 160, 115, 40, "GAME LIST", CLR_BTN, false };

    // Initialize Settings Category Buttons
    layoutCatBtn = { 40, 45, 240, 40, "[ LAYOUT ]", CLR_BTN, false };
    themeCatBtn  = { 40, 95, 240, 40, "[ THEME ]", CLR_BTN, false };
    speedCatBtn  = { 40, 145, 240, 40, "[ SENSITIVITY ]", CLR_BTN, false };

    // Initialize Settings Layout Buttons
    layout1Btn  = { 40, 40, 240, 40, "Classic Layout", CLR_BTN, false };
    layout2Btn  = { 40, 90, 240, 40, "Modern Layout", CLR_BTN, false };
    layout3Btn  = { 40, 140, 240, 40, "One-Handed Layout", CLR_BTN, false };
    backBtn     = { 40, 195, 240, 30, "BACK", C2D_Color32(80, 20, 20, 255), false };

    // Initialize Settings Sensitivity Buttons
    zoomSpeedBtn   = { 40, 40, 240, 40, "ZOOM: Medium", CLR_BTN, false };
    panSpeedBtn    = { 40, 90, 240, 40, "PAN: Medium", CLR_BTN, false };
    scrollSpeedBtn = { 40, 140, 240, 40, "SCROLL: Medium", CLR_BTN, false };

    // Initialize Settings Theme Buttons
    themeDarkBtn   = { 40, 40, 115, 40, "Dark Theme", CLR_BTN, false };
    themeLightBtn  = { 165, 40, 115, 40, "Light Theme", CLR_BTN, false };
    accentCycleBtn = { 40, 95, 240, 45, "ACCENT COLOR", CLR_BTN, false };

    // Initialize Multitasking HUD Buttons (Gameplay Overlay)
    hudPrevBtn      = { 10, 210, 25, 26, "[<]", CLR_BTN, false };
    hudNextBtn      = { 40, 210, 25, 26, "[>]", CLR_BTN, false };
    hudShowMapBtn   = { 75, 210, 110, 26, "SHOW MAP", CLR_BTN, false };
    hudShowGuideBtn = { 200, 210, 110, 26, "SHOW GUIDE", CLR_BTN, false };

    // Initialize HUD Back Button for Map/Walk Views
    hudBackBtn      = { 110, 210, 100, 26, "BACK", C2D_Color32(80, 20, 20, 255), false };

    // Prepare static text
    C2D_TextParse(&playText, staticTextBuf, playBtn.label);
    C2D_TextOptimize(&playText);
    C2D_TextParse(&mapsText, staticTextBuf, mapsBtn.label);
    C2D_TextOptimize(&mapsText);
    C2D_TextParse(&walkText, staticTextBuf, walkBtn.label);
    C2D_TextOptimize(&walkText);
    C2D_TextParse(&settingsText, staticTextBuf, settingsBtn.label);
    C2D_TextOptimize(&settingsText);
    C2D_TextParse(&listText, staticTextBuf, listBtn.label);
    C2D_TextOptimize(&listText);

    C2D_TextParse(&layoutCatText, staticTextBuf, layoutCatBtn.label);
    C2D_TextOptimize(&layoutCatText);
    C2D_TextParse(&themeCatText, staticTextBuf, themeCatBtn.label);
    C2D_TextOptimize(&themeCatText);
    C2D_TextParse(&speedCatText, staticTextBuf, speedCatBtn.label);
    C2D_TextOptimize(&speedCatText);

    C2D_TextParse(&layout1Text, staticTextBuf, layout1Btn.label);
    C2D_TextOptimize(&layout1Text);
    C2D_TextParse(&layout2Text, staticTextBuf, layout2Btn.label);
    C2D_TextOptimize(&layout2Text);
    C2D_TextParse(&layout3Text, staticTextBuf, layout3Btn.label);
    C2D_TextOptimize(&layout3Text);
    C2D_TextParse(&backText, staticTextBuf, backBtn.label);
    C2D_TextOptimize(&backText);

    C2D_TextParse(&hudPrevText, staticTextBuf, hudPrevBtn.label);
    C2D_TextOptimize(&hudPrevText);
    C2D_TextParse(&hudNextText, staticTextBuf, hudNextBtn.label);
    C2D_TextOptimize(&hudNextText);
    C2D_TextParse(&hudShowMapText, staticTextBuf, hudShowMapBtn.label);
    C2D_TextOptimize(&hudShowMapText);
    C2D_TextParse(&hudShowGuideText, staticTextBuf, hudShowGuideBtn.label);
    C2D_TextOptimize(&hudShowGuideText);

    C2D_TextParse(&themeDarkText, staticTextBuf, themeDarkBtn.label);
    C2D_TextOptimize(&themeDarkText);
    C2D_TextParse(&themeLightText, staticTextBuf, themeLightBtn.label);
    C2D_TextOptimize(&themeLightText);
    C2D_TextParse(&accentCycleText, staticTextBuf, accentCycleBtn.label);
    C2D_TextOptimize(&accentCycleText);

    C2D_TextParse(&versionText, staticTextBuf, "V2NES v1.6.0");
    C2D_TextOptimize(&versionText);
}

void V2UI::showNotification(const char* text) {
    notifyTimer = 120; // 2 seconds at 60fps
    printf("\x1b[15;1H\x1b[K[ALERT] %s", text);
}

void V2UI::updateBattery(int percent) {
    batteryPercent = percent;
    if (percent > 20) batteryColor = C2D_Color32(0, 255, 100, 255);
    else if (percent > 10) batteryColor = C2D_Color32(255, 255, 0, 255);
    else batteryColor = C2D_Color32(255, 0, 0, 255);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", batteryPercent);

    // Use dynamic buffer to avoid leaking into static memory
    static C2D_TextBuf batBuf = C2D_TextBufNew(64);
    C2D_TextBufClear(batBuf);
    C2D_TextParse(&batteryText, batBuf, buf);
    C2D_TextOptimize(&batteryText);
}

bool V2UI::checkTouch(touchPosition touch, Button& btn) {
    if (touch.px >= btn.x && touch.px <= btn.x + btn.w &&
        touch.py >= btn.y && touch.py <= btn.y + btn.h) {
        btn.pressed = true;
        return true;
    }
    btn.pressed = false;
    return false;
}

void V2UI::drawButton(Button& btn) {
    bool isTransparentZoom = false;
    float cornerRadius = 10.0f;

    u32 color;
    if (btn.pressed) {
        color = activeBtnHlColor;
    } else if (btn.color == C2D_Color32(80, 20, 20, 255)) {
        color = activeBtnColor;
    } else if (btn.color == CLR_BTN) {
        color = activeBtnColor;
    } else {
        color = btn.color;
    }

    if (!isTransparentZoom) {
        // Draw main button background rounded rectangle
        drawRoundedRect(btn.x, btn.y, btn.w, btn.h, cornerRadius, color);

        u8 ar = (activeAccentColor >> 0) & 0xFF;
        u8 ag = (activeAccentColor >> 8) & 0xFF;
        u8 ab = (activeAccentColor >> 16) & 0xFF;

        // Crisp 1px accent border (brighter on press)
        u32 borderColor = btn.pressed ? C2D_Color32(ar, ag, ab, 255) : C2D_Color32(ar, ag, ab, 180);
        drawRoundedOutline(btn.x, btn.y, btn.w, btn.h, cornerRadius, 1.0f, borderColor);

        // Subtle white inner glass-edge highlight (light catching the rim)
        u32 glassEdge = C2D_Color32(255, 255, 255, 40);
        drawRoundedOutline(btn.x + 1.0f, btn.y + 1.0f, btn.w - 2.0f, btn.h - 2.0f, cornerRadius - 1.0f, 1.0f, glassEdge);
    }

    // Draw label and optional procedural icons
    C2D_Text* text = nullptr;
    bool hasIcon = false;

    if (strcmp(btn.label, "PLAY") == 0) text = &playText;
    else if (strcmp(btn.label, "MAPS") == 0) { text = &mapsText; }
    else if (strcmp(btn.label, "GUIDES") == 0) { text = &walkText; }
    else if (strcmp(btn.label, "SETTINGS") == 0) { text = &settingsText; }
    else if (strcmp(btn.label, "GAME LIST") == 0) { text = &listText; }
    else if (strcmp(btn.label, "[ LAYOUT ]") == 0) text = &layoutCatText;
    else if (strcmp(btn.label, "[ THEME ]") == 0) text = &themeCatText;
    else if (strcmp(btn.label, "Classic Layout") == 0) text = &layout1Text;
    else if (strcmp(btn.label, "Modern Layout") == 0) text = &layout2Text;
    else if (strcmp(btn.label, "One-Handed Layout") == 0) text = &layout3Text;
    else if (strcmp(btn.label, "BACK") == 0) text = &backText;
    else if (strcmp(btn.label, "[<]") == 0) text = &hudPrevText;
    else if (strcmp(btn.label, "[>]") == 0) text = &hudNextText;
    else if (strcmp(btn.label, "SHOW MAP") == 0) text = &hudShowMapText;
    else if (strcmp(btn.label, "SHOW GUIDE") == 0) text = &hudShowGuideText;
    else if (strcmp(btn.label, "Dark Theme") == 0) text = &themeDarkText;
    else if (strcmp(btn.label, "Light Theme") == 0) text = &themeLightText;
    else if (strcmp(btn.label, "ACCENT COLOR") == 0) text = &accentCycleText;
    else if (strcmp(btn.label, "[ SENSITIVITY ]") == 0) text = &speedCatText;
    else if (strncmp(btn.label, "ZOOM:", 5) == 0) {
        C2D_TextParse(&zoomSpeedText, dynamicTextBuf, btn.label);
        C2D_TextOptimize(&zoomSpeedText);
        text = &zoomSpeedText;
    }
    else if (strncmp(btn.label, "PAN:", 4) == 0) {
        C2D_TextParse(&panSpeedText, dynamicTextBuf, btn.label);
        C2D_TextOptimize(&panSpeedText);
        text = &panSpeedText;
    }
    else if (strncmp(btn.label, "SCROLL:", 7) == 0) {
        C2D_TextParse(&scrollSpeedText, dynamicTextBuf, btn.label);
        C2D_TextOptimize(&scrollSpeedText);
        text = &scrollSpeedText;
    }

    if (text) {
        float textW, textH;
        float scale = isTransparentZoom ? 0.8f : ((btn.w < 50) ? 0.4f : ((btn.w < 90) ? 0.45f : 0.6f));
        u32 txtColor = isTransparentZoom ? (btn.pressed ? activeAccentColor : C2D_Color32(255, 255, 255, 180)) : activeTextColor;
        C2D_TextGetDimensions(text, scale, scale, &textW, &textH);

        if (hasIcon && !isTransparentZoom) {
            float iconSize = 20.0f;
            float spacing = 8.0f;
            float totalW = iconSize + spacing + textW;
            float startX = btn.x + (btn.w - totalW) / 2;

            // Draw procedural premium icon
            drawIcon(btn.label, startX, btn.y + (btn.h - iconSize) / 2, activeAccentColor, txtColor, color);

            // Draw text aligned adjacent to the icon
            C2D_DrawText(text, C2D_WithColor, startX + iconSize + spacing, btn.y + (btn.h - textH) / 2, 0.5f, scale, scale, txtColor);
        } else {
            C2D_DrawText(text, C2D_WithColor, btn.x + (btn.w - textW) / 2, btn.y + (btn.h - textH) / 2, 0.5f, scale, scale, txtColor);
        }

        if (strncmp(btn.label, "ZOOM:", 5) == 0 || strncmp(btn.label, "PAN:", 4) == 0 || strncmp(btn.label, "SCROLL:", 7) == 0) {
            C2D_TextBufClear(dynamicTextBuf);
        }
    }
}

void V2UI::renderBottom(GameState state, V2Settings& settings) {
    // 8 original 3DS Accent Colors mapping
    static const u32 accentColors[8] = {
        C2D_Color32(0, 220, 200, 255),    // Aquamarin Wave
        C2D_Color32(230, 20, 40, 255),    // Shimmer Red
        C2D_Color32(240, 210, 0, 255),    // Yellow lemon
        C2D_Color32(50, 220, 50, 255),    // Neox Green
        C2D_Color32(160, 40, 220, 255),   // Dusty Purple
        C2D_Color32(255, 100, 0, 255),    // Crazy Orange
        C2D_Color32(240, 50, 160, 255),   // Pink glamour
        C2D_Color32(170, 170, 185, 255)   // Old'e Responsible
    };

    int accentIdx = (int)settings.getAccent();
    if (accentIdx < 0 || accentIdx >= 8) accentIdx = 0;
    activeAccentColor = accentColors[accentIdx];

    if (settings.getTheme() == THEME_LIGHT) {
        activeBgColor     = C2D_Color32(245, 245, 240, 255); // Parchment Off-White
        activeBtnColor    = C2D_Color32(255, 255, 255, 160); // Semi-transparent glass
        activeBtnHlColor  = C2D_Color32(230, 230, 230, 160); // Semi-transparent pressed
        activeTextColor   = C2D_Color32(50, 50, 50, 255);    // Dark Charcoal
    } else {
        activeBgColor     = C2D_Color32(20, 20, 25, 255);    // Deep Charcoal Slate
        activeBtnColor    = C2D_Color32(45, 45, 60, 160);    // Semi-transparent dark glass
        activeBtnHlColor  = C2D_Color32(70, 70, 90, 160);    // Semi-transparent pressed
        activeTextColor   = C2D_Color32(255, 255, 255, 255); // Crisp White
    }

    u32 activeListBgColor = (settings.getTheme() == THEME_LIGHT) ? C2D_Color32(230, 230, 220, 255) : C2D_Color32(30, 30, 40, 255);
    u32 activeHudBg = (settings.getTheme() == THEME_LIGHT) ? C2D_Color32(240, 240, 240, 230) : C2D_Color32(10, 10, 15, 230);

    // Apply color choices to standard buttons dynamically
    playBtn.color = mapsBtn.color = walkBtn.color = settingsBtn.color = listBtn.color = activeBtnColor;
    layoutCatBtn.color = themeCatBtn.color = speedCatBtn.color = activeBtnColor;
    accentCycleBtn.color = activeBtnColor;
    themeDarkBtn.color = themeLightBtn.color = activeBtnColor;
    zoomSpeedBtn.color = panSpeedBtn.color = scrollSpeedBtn.color = activeBtnColor;
    hudPrevBtn.color = hudNextBtn.color = hudShowMapBtn.color = hudShowGuideBtn.color = activeBtnColor;

    // Apply specific dynamic color overrides for selected visual settings
    ControlLayout activeLayout = settings.getLayout();
    layout1Btn.color = (activeLayout == LAYOUT_CLASSIC) ? activeAccentColor : activeBtnColor;
    layout2Btn.color = (activeLayout == LAYOUT_MODERN) ? activeAccentColor : activeBtnColor;
    layout3Btn.color = (activeLayout == LAYOUT_ONE_HANDED) ? activeAccentColor : activeBtnColor;

    ThemeMode activeTheme = settings.getTheme();
    themeDarkBtn.color = (activeTheme == THEME_DARK) ? activeAccentColor : activeBtnColor;
    themeLightBtn.color = (activeTheme == THEME_LIGHT) ? activeAccentColor : activeBtnColor;

    static C3D_RenderTarget* target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C2D_TargetClear(target, activeBgColor);
    C2D_SceneBegin(target);

    if (state == STATE_MENU) {
        drawButton(playBtn);
        drawButton(mapsBtn);
        drawButton(walkBtn);
        drawButton(settingsBtn);
        drawButton(listBtn);
    } else if (state == STATE_SETTINGS) {
        drawButton(layoutCatBtn);
        drawButton(themeCatBtn);
        drawButton(speedCatBtn);
        drawButton(backBtn);
    } else if (state == STATE_SETTINGS_SENSITIVITY) {
        static char zoomLabel[32];
        static char panLabel[32];
        static char scrollLabel[32];

        snprintf(zoomLabel, sizeof(zoomLabel), "ZOOM: %s", settings.getSpeedName(settings.getZoomSpeed()));
        snprintf(panLabel, sizeof(panLabel), "PAN: %s", settings.getSpeedName(settings.getPanSpeed()));
        snprintf(scrollLabel, sizeof(scrollLabel), "SCROLL: %s", settings.getSpeedName(settings.getScrollSpeed()));

        zoomSpeedBtn.label = zoomLabel;
        panSpeedBtn.label = panLabel;
        scrollSpeedBtn.label = scrollLabel;

        drawButton(zoomSpeedBtn);
        drawButton(panSpeedBtn);
        drawButton(scrollSpeedBtn);
        drawButton(backBtn);
    } else if (state == STATE_SETTINGS_LAYOUT) {
        drawButton(layout1Btn);
        drawButton(layout2Btn);
        drawButton(layout3Btn);
        drawButton(backBtn);
    } else if (state == STATE_SETTINGS_THEME) {
        drawButton(themeDarkBtn);
        drawButton(themeLightBtn);
        drawButton(accentCycleBtn);
        drawButton(backBtn);
    } else if (state == STATE_MAP_LIST) {
        C2D_DrawRectSolid(20, 40, 0.5f, 280, 150, activeListBgColor);
        for (size_t i = 0; i < content.mapFiles.size() && i < 5; i++) {
            float y = 50 + i * 25;
            bool isActive = (activeMapIndex == (int)i);
            
            if (isActive) {
                u32 hlBarColor = (activeAccentColor & 0x00FFFFFF) | (40 << 24);
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, hlBarColor);
            }

            u32 color = isActive ? activeAccentColor : activeTextColor;
            char name[32];
            strncpy(name, content.mapFiles[i].name.c_str(), 28);
            name[28] = '\0';
            if (content.mapFiles[i].name.length() > 28) strcat(name, "...");

            C2D_Text fileTxt;
            C2D_TextParse(&fileTxt, dynamicTextBuf, name);
            C2D_TextOptimize(&fileTxt);
            C2D_DrawText(&fileTxt, C2D_WithColor, 30, y, 0.5f, 0.5f, 0.5f, color);
            C2D_TextBufClear(dynamicTextBuf);
        }
        drawButton(backBtn);
    } else if (state == STATE_MAP_VIEW) {
        content.drawMap(240.0f);
    } else if (state == STATE_WALK_LIST) {
        C2D_DrawRectSolid(20, 40, 0.5f, 280, 150, activeListBgColor);
        for (size_t i = 0; i < content.walkFiles.size() && i < 5; i++) {
            float y = 50 + i * 25;
            bool isActive = (activeWalkIndex == (int)i);
            
            if (isActive) {
                u32 hlBarColor = (activeAccentColor & 0x00FFFFFF) | (40 << 24);
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, hlBarColor);
            }

            u32 color = isActive ? activeAccentColor : activeTextColor;
            char name[32];
            strncpy(name, content.walkFiles[i].name.c_str(), 28);
            name[28] = '\0';
            if (content.walkFiles[i].name.length() > 28) strcat(name, "...");

            C2D_Text fileTxt;
            C2D_TextParse(&fileTxt, dynamicTextBuf, name);
            C2D_TextOptimize(&fileTxt);
            C2D_DrawText(&fileTxt, C2D_WithColor, 30, y, 0.5f, 0.5f, 0.5f, color);
            C2D_TextBufClear(dynamicTextBuf);
        }
        drawButton(backBtn);
    } else if (state == STATE_WALK_VIEW) {
        content.drawWalk(scrollOffset);
    } else if (state == STATE_GAME_LIST) {
        C2D_DrawRectSolid(20, 40, 0.5f, 280, 150, activeListBgColor);
        if (content.romFiles.empty()) {
            C2D_Text emptyTxt;
            C2D_TextParse(&emptyTxt, dynamicTextBuf, "No ROMs Found! (/V2NES/roms/)");
            C2D_TextOptimize(&emptyTxt);
            C2D_DrawText(&emptyTxt, C2D_WithColor, 30, 80, 0.5f, 0.5f, 0.5f, activeTextColor);
            C2D_TextBufClear(dynamicTextBuf);
        } else {
            for (size_t i = 0; i < content.romFiles.size() && i < 5; i++) {
                float y = 50 + i * 25;
                bool isActive = (activeRomIndex == (int)i);
                
                if (isActive) {
                    u32 hlBarColor = (activeAccentColor & 0x00FFFFFF) | (40 << 24);
                    C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, hlBarColor);
                }

                u32 color = isActive ? activeAccentColor : activeTextColor;
                char name[32];
                strncpy(name, content.romFiles[i].name.c_str(), 28);
                name[28] = '\0';
                if (content.romFiles[i].name.length() > 28) strcat(name, "...");

                C2D_Text fileTxt;
                C2D_TextParse(&fileTxt, dynamicTextBuf, name);
                C2D_TextOptimize(&fileTxt);
                C2D_DrawText(&fileTxt, C2D_WithColor, 30, y, 0.5f, 0.5f, 0.5f, color);
                C2D_TextBufClear(dynamicTextBuf);
            }
        }
        drawButton(backBtn);
    } else if (state == STATE_PLAYING) {
        // Dual-mode rendering during gameplay based on the active HUD setting
        if (gameplayMode == GAMEPLAY_MAP) {
            content.drawMap(206.0f);
        } else {
            content.drawWalk(scrollOffset);
        }

        // Draw sleek glass background overlay bar for HUD
        C2D_DrawRectSolid(0, 206, 0.5f, 320, 34, activeHudBg);

        // Draw our static Touch Buttons
        drawButton(hudPrevBtn);
        drawButton(hudNextBtn);
        drawButton(hudShowMapBtn);
        drawButton(hudShowGuideBtn);
    }

    // Notifications (rendered on top screen console)
    if (notifyTimer > 0) {
        notifyTimer--;
        if (notifyTimer == 0) {
            printf("\x1b[15;1H\x1b[K"); // Clear notification line on top console
        }
    }

    // Footer: Version (left), Clock (center), Battery (right)
    if (state != STATE_PLAYING) {
        float footerY = 226.0f;
        float footerScale = 0.45f;

        // Draw footer background strip (glassmorphism/semi-transparent)
        u32 barBg = (settings.getTheme() == THEME_LIGHT) ? C2D_Color32(230, 230, 230, 220) : C2D_Color32(15, 15, 20, 220);
        u32 barBorder = (settings.getTheme() == THEME_LIGHT) ? C2D_Color32(0, 0, 0, 30) : C2D_Color32(255, 255, 255, 30);

        C2D_DrawRectSolid(0, 224, 0.5f, SCREEN_WIDTH, 16, barBg);
        C2D_DrawRectSolid(0, 224, 0.5f, SCREEN_WIDTH, 1, barBorder);

        u32 footerColor = (settings.getTheme() == THEME_LIGHT) ? C2D_Color32(80, 80, 80, 255) : C2D_Color32(210, 210, 210, 255);

        C2D_DrawText(&versionText, C2D_WithColor, 8, footerY, 0.5f, footerScale, footerScale, footerColor);

        {
            static C2D_TextBuf clockBuf = C2D_TextBufNew(64);
            time_t rawtime;
            time(&rawtime);
            struct tm* t = localtime(&rawtime);
            char timStr[32];
            int hour = t->tm_hour;
            const char* ampm = (hour >= 12) ? "PM" : "AM";
            if (hour > 12) hour -= 12;
            if (hour == 0) hour = 12;
            snprintf(timStr, sizeof(timStr), "%d:%02d %s", hour, t->tm_min, ampm);

            C2D_TextBufClear(clockBuf);
            C2D_Text clockText;
            C2D_TextParse(&clockText, clockBuf, timStr);
            C2D_TextOptimize(&clockText);
            float tw, th;
            C2D_TextGetDimensions(&clockText, footerScale, footerScale, &tw, &th);
            C2D_DrawText(&clockText, C2D_WithColor, (SCREEN_WIDTH - tw) / 2.0f, footerY, 0.5f, footerScale, footerScale, footerColor);
        }

        float batW, batH;
        C2D_TextGetDimensions(&batteryText, footerScale, footerScale, &batW, &batH);
        C2D_DrawText(&batteryText, C2D_WithColor, SCREEN_WIDTH - batW - 8.0f, footerY, 0.5f, footerScale, footerScale, batteryColor);
    }

    // Savestate Screen Notification Flash
    if (saveLoadFlashTimer > 0) {
        C2D_DrawRectangle(0, 0, 0.9f, SCREEN_WIDTH, 4, activeAccentColor, activeAccentColor, activeAccentColor, activeAccentColor); // Top
        C2D_DrawRectangle(0, SCREEN_HEIGHT - 4, 0.9f, SCREEN_WIDTH, 4, activeAccentColor, activeAccentColor, activeAccentColor, activeAccentColor); // Bottom
        C2D_DrawRectangle(0, 0, 0.9f, 4, SCREEN_HEIGHT, activeAccentColor, activeAccentColor, activeAccentColor, activeAccentColor); // Left
        C2D_DrawRectangle(SCREEN_WIDTH - 4, 0, 0.9f, 4, SCREEN_HEIGHT, activeAccentColor, activeAccentColor, activeAccentColor, activeAccentColor); // Right
        saveLoadFlashTimer--;
    }
}





