#include "ui.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "3dslodepng.h"

// Embedded HUD icon sprite sheet (data/hud_icons.png → tex3ds → bin2s)
extern "C" const u8  hud_icons_t3x[];
extern "C" const u8  hud_icons_t3x_end[];
extern "C" const u32 hud_icons_t3x_size;

// Embedded per-button glow overlays (data/aquamarine_glow_{back,fowd,maps,walk}.png)
extern "C" const u8 aquamarine_glow_back_png[];
extern "C" const u8 aquamarine_glow_back_png_end[];
extern "C" const u8 aquamarine_glow_4wrd_png[];
extern "C" const u8 aquamarine_glow_4wrd_png_end[];
extern "C" const u8 aquamarine_glow_maps_png[];
extern "C" const u8 aquamarine_glow_maps_png_end[];
extern "C" const u8 aquamarine_glow_walk_png[];
extern "C" const u8 aquamarine_glow_walk_png_end[];

// Embedded main-menu button glow overlays (data/aquamarine_glow_{play,menu}.png)
extern "C" const u8 aquamarine_glow_play_png[];
extern "C" const u8 aquamarine_glow_play_png_end[];
extern "C" const u8 aquamarine_glow_menu_png[];
extern "C" const u8 aquamarine_glow_menu_png_end[];

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

static unsigned nextPow2(unsigned v) {
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
    return v + 1;
}

static bool initSmallGlowTex(const u8* pngData, const u8* pngEnd,
                              C3D_Tex* tex, C2D_Image* img,
                              Tex3DS_SubTexture* subtex) {
    u8* rgba = nullptr;
    unsigned w = 0, h = 0;
    size_t bytes = (size_t)(pngEnd - pngData);
    if (lodepng_decode32(&rgba, &w, &h, pngData, bytes) != 0 || !rgba) {
        if (rgba) free(rgba);
        return false;
    }
    unsigned texW = nextPow2(w);
    unsigned texH = nextPow2(h);

    u8* tiled = (u8*)linearAlloc(texW * texH * 4);
    if (!tiled) { free(rgba); return false; }
    memset(tiled, 0, texW * texH * 4);

    for (unsigned y = 0; y < h; y++) {
        for (unsigned x = 0; x < w; x++) {
            u16 blockX = x / 8;
            u16 blockY = y / 8;
            u8  pixelX = x % 8;
            u8  pixelY = y % 8;
            u32 morton = 0;
            morton |= (pixelX & 1) << 0;
            morton |= (pixelY & 1) << 1;
            morton |= (pixelX & 2) << 1;
            morton |= (pixelY & 2) << 2;
            morton |= (pixelX & 4) << 2;
            morton |= (pixelY & 4) << 3;
            u32 blockIndex  = blockY * (texW / 8) + blockX;
            u32 tiledOffset = (blockIndex * 64 + morton) * 4;
            u32 linearOff   = (y * w + x) * 4;
            tiled[tiledOffset + 0] = rgba[linearOff + 3];
            tiled[tiledOffset + 1] = rgba[linearOff + 2];
            tiled[tiledOffset + 2] = rgba[linearOff + 1];
            tiled[tiledOffset + 3] = rgba[linearOff + 0];
        }
    }
    free(rgba);

    C3D_TexInit(tex, texW, texH, GPU_RGBA8);
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexUpload(tex, tiled);
    C3D_TexFlush(tex);
    linearFree(tiled);

    // UV convention (matches the working hud_icons slicing above):
    //   top = 1.0 maps to image row 0 (content top); bottom = (texH-h)/texH maps
    //   to image row h (content bottom). Content sits at the top-left of the
    //   padded texture, so left=0 and right=w/texW. Using top=h/texH,bottom=0
    //   (as the full-screen glow does) shifts a small padded sprite upward.
    subtex->width  = w;
    subtex->height = h;
    subtex->left   = 0.0f;
    subtex->right  = (float)w / (float)texW;
    subtex->top    = 1.0f;
    subtex->bottom = (float)(texH - h) / (float)texH;
    img->tex    = tex;
    img->subtex = subtex;
    return true;
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
    for (int i = 0; i < 4; i++) {
        if (hudGlowReady[i]) C3D_TexDelete(&hudGlowTex[i]);
    }
    for (int i = 0; i < 2; i++) {
        if (menuGlowReady[i]) C3D_TexDelete(&menuGlowTex[i]);
    }
    C2D_TextBufDelete(staticTextBuf);
    C2D_TextBufDelete(dynamicTextBuf);
    C2D_Fini();
    C3D_Fini();
}

void V2UI::init() {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Load HUD icon sprite sheet via tex3ds — pre-tiled by tex3ds at build time,
    // so Tex3DS_TextureImport is just a fast GPU upload with no CPU swizzle.
    hudIconsT3x = Tex3DS_TextureImport(hud_icons_t3x,
                                        (size_t)(hud_icons_t3x_end - hud_icons_t3x),
                                        &hudIconsTex, nullptr, false);
    if (hudIconsT3x) {
        C3D_TexSetFilter(&hudIconsTex, GPU_LINEAR, GPU_LINEAR);

        // Manually slice the 128×128 sheet into four 32×32 icon regions (one row).
        // UV convention: top=1.0 → image row 0; bottom = 1-(rowH/texH).
        for (int i = 0; i < 4; i++) {
            float l = (i * 32) / 128.0f;
            float r = ((i + 1) * 32) / 128.0f;
            hudIconSubTexs[i] = { 32, 32, l, 1.0f, r, 0.75f };
            hudIcons[i].tex    = &hudIconsTex;
            hudIcons[i].subtex = &hudIconSubTexs[i];
        }
        hudIconsReady = true;
    }

    staticTextBuf = C2D_TextBufNew(4096);
    dynamicTextBuf = C2D_TextBufNew(512);

    // Initialize Main Buttons
    playBtn     = { 40, 40, 240, 60, "PLAY", CLR_BTN, false };
    mapsBtn     = { 40, 110, 115, 40, "MAPS", CLR_BTN, false };
    walkBtn     = { 165, 110, 115, 40, "GUIDES", CLR_BTN, false };
    settingsBtn = { 40, 160, 115, 40, "SETTINGS", CLR_BTN, false };
    listBtn     = { 165, 160, 115, 40, "GAME LIST", CLR_BTN, false };

    // Initialize Settings Buttons
    speedCatBtn  = { 40, 45, 240, 40, "[ SENSITIVITY ]", CLR_BTN, false };
    backBtn      = { 40, 195, 240, 30, "BACK", C2D_Color32(80, 20, 20, 255), false };

    // Initialize Settings Sensitivity Buttons
    zoomSpeedBtn   = { 40, 40, 240, 40, "ZOOM: Medium", CLR_BTN, false };
    panSpeedBtn    = { 40, 90, 240, 40, "PAN: Medium", CLR_BTN, false };
    scrollSpeedBtn = { 40, 140, 240, 40, "SCROLL: Medium", CLR_BTN, false };

    // Initialize Multitasking HUD Buttons (Gameplay Overlay)
    // Layout: BACK | FWD   [battery · clock]   MAPS | GUIDES
    // HUD bar is 31px tall (y=209..240). Buttons fill the bar height (y=211, h=27).
    hudPrevBtn      = { 2,   211, 54, 27, "BACK",   CLR_BTN, false };
    hudNextBtn      = { 60,  211, 60, 27, "4WRD",   CLR_BTN, false };
    hudShowMapBtn   = { 202, 211, 52, 27, "MAPS",   CLR_BTN, false };
    hudShowGuideBtn = { 258, 211, 60, 27, "WALK",   CLR_BTN, false };

    // Battery lives where [X] used to be — no longer a touch button
    hudBackBtn      = { 0, 0, 0, 0, "", CLR_BTN, false }; // unused

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

    // Theme button label is dynamic (shows current accent name) — not pre-parsed.
    C2D_TextParse(&speedCatText, staticTextBuf, speedCatBtn.label);
    C2D_TextOptimize(&speedCatText);
    C2D_TextParse(&backText, staticTextBuf, backBtn.label);
    C2D_TextOptimize(&backText);

    // HUD text buttons — pre-parse stable labels into static slots.
    // "BACK" reuses backText (already parsed above).
    C2D_TextParse(&hudShowMapText,   staticTextBuf, "MAPS");
    C2D_TextOptimize(&hudShowMapText);
    C2D_TextParse(&hudShowGuideText, staticTextBuf, "WALK");
    C2D_TextOptimize(&hudShowGuideText);
    C2D_TextParse(&hudNextText, staticTextBuf, "4WRD");
    C2D_TextOptimize(&hudNextText);

    C2D_TextParse(&versionText, staticTextBuf, "V2NES v1.8.6");
    C2D_TextOptimize(&versionText);

    // Load per-button glow overlay textures
    struct { const u8* data; const u8* end; } glowPngs[4] = {
        { aquamarine_glow_back_png, aquamarine_glow_back_png_end },
        { aquamarine_glow_4wrd_png, aquamarine_glow_4wrd_png_end },
        { aquamarine_glow_maps_png, aquamarine_glow_maps_png_end },
        { aquamarine_glow_walk_png, aquamarine_glow_walk_png_end },
    };
    for (int i = 0; i < 4; i++) {
        hudGlowReady[i] = initSmallGlowTex(glowPngs[i].data, glowPngs[i].end,
                                             &hudGlowTex[i], &hudGlowImg[i],
                                             &hudGlowSubTex[i]);
    }

    // Load main-menu button glow overlays (0=PLAY 240x60, 1=115x40 buttons).
    struct { const u8* data; const u8* end; } menuPngs[2] = {
        { aquamarine_glow_play_png, aquamarine_glow_play_png_end },
        { aquamarine_glow_menu_png, aquamarine_glow_menu_png_end },
    };
    for (int i = 0; i < 2; i++) {
        menuGlowReady[i] = initSmallGlowTex(menuPngs[i].data, menuPngs[i].end,
                                            &menuGlowTex[i], &menuGlowImg[i],
                                            &menuGlowSubTex[i]);
    }
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

void V2UI::drawButton(Button& btn, bool noBorder) {
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

        // HUD buttons (noBorder) rely on the glow overlay for their edge,
        // so skip the procedural accent border + glass-edge highlight.
        if (!noBorder) {
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
    }

    // Draw label and optional procedural icons
    C2D_Text* text = nullptr;
    bool hasIcon = false;

    if (strcmp(btn.label, "PLAY") == 0) text = &playText;
    else if (strcmp(btn.label, "MAPS") == 0) { text = &mapsText; }
    else if (strcmp(btn.label, "GUIDES") == 0) { text = &walkText; }
    else if (strcmp(btn.label, "SETTINGS") == 0) { text = &settingsText; }
    else if (strcmp(btn.label, "GAME LIST") == 0) { text = &listText; }
    else if (strcmp(btn.label, "BACK") == 0) text = &backText;
    else if (strcmp(btn.label, "4WRD") == 0) text = &hudNextText;
    else if (strcmp(btn.label, "MAPS") == 0)   text = &hudShowMapText;
    else if (strcmp(btn.label, "WALK") == 0)   text = &hudShowGuideText;
    else if (strcmp(btn.label, "[X]") == 0) text = &hudBackText;
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
            // Center the label both horizontally (AlignCenter about the button
            // mid-x) and vertically within the button.
            C2D_DrawText(text, C2D_WithColor | C2D_AlignCenter,
                         btn.x + btn.w / 2.0f, btn.y + (btn.h - textH) / 2.0f,
                         0.5f, scale, scale, txtColor);
        }

        if (strncmp(btn.label, "ZOOM:", 5) == 0 || strncmp(btn.label, "PAN:", 4) == 0 ||
            strncmp(btn.label, "SCROLL:", 7) == 0) {
            C2D_TextBufClear(dynamicTextBuf);
        }
    }
}

void V2UI::renderBottom(GameState state, V2Settings& settings) {
    // Fixed theme: Aquamarin Wave accent, dark background
    activeAccentColor = C2D_Color32(0, 220, 200, 255);
    activeBgColor     = C2D_Color32(20, 20, 25, 255);
    activeBtnColor    = C2D_Color32(45, 45, 60, 160);
    activeBtnHlColor  = C2D_Color32(70, 70, 90, 160);
    activeTextColor   = C2D_Color32(255, 255, 255, 255);

    u32 activeListBgColor = C2D_Color32(30, 30, 40, 255);
    u32 activeHudBg       = C2D_Color32(10, 10, 15, 230);

    // Apply colors to buttons
    playBtn.color = mapsBtn.color = walkBtn.color = settingsBtn.color = listBtn.color = activeBtnColor;
    speedCatBtn.color = activeBtnColor;
    zoomSpeedBtn.color = panSpeedBtn.color = scrollSpeedBtn.color = activeBtnColor;
    hudPrevBtn.color = hudNextBtn.color = hudShowMapBtn.color = hudShowGuideBtn.color = activeBtnColor;

    static C3D_RenderTarget* target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C2D_TargetClear(target, activeBgColor);
    C2D_SceneBegin(target);

    if (state == STATE_STARTUP) {
        // Show a small "App loading..." hint while the app initialises.
        static C2D_TextBuf startupBuf = C2D_TextBufNew(64);
        static bool startupBufReady = false;
        static C2D_Text startupTxt;
        if (!startupBufReady) {
            C2D_TextParse(&startupTxt, startupBuf, "App loading...");
            C2D_TextOptimize(&startupTxt);
            startupBufReady = true;
        }
        float tw, th;
        C2D_TextGetDimensions(&startupTxt, 0.5f, 0.5f, &tw, &th);
        C2D_DrawText(&startupTxt, C2D_WithColor,
            (SCREEN_WIDTH - tw) / 2.0f, (SCREEN_HEIGHT - th) / 2.0f,
            0.5f, 0.5f, 0.5f, C2D_Color32(180, 180, 180, 255));
    } else if (state == STATE_MENU) {
        // noBorder=true: the glow overlay below provides the edge treatment.
        drawButton(playBtn, true);
        drawButton(mapsBtn, true);
        drawButton(walkBtn, true);
        drawButton(settingsBtn, true);
        drawButton(listBtn, true);

        // Glow overlays: index 0 = PLAY (240x60), index 1 = the four 115x40 buttons.
        if (menuGlowReady[0])
            C2D_DrawImageAt(menuGlowImg[0], playBtn.x, playBtn.y, 0.6f, NULL, 1.0f, 1.0f);
        if (menuGlowReady[1]) {
            Button* menuBtns[4] = { &mapsBtn, &walkBtn, &settingsBtn, &listBtn };
            for (int i = 0; i < 4; i++)
                C2D_DrawImageAt(menuGlowImg[1], menuBtns[i]->x, menuBtns[i]->y, 0.6f, NULL, 1.0f, 1.0f);
        }
    } else if (state == STATE_SETTINGS) {
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
    } else if (state == STATE_MAP_FOLDER_LIST) {
        C2D_DrawRectSolid(20, 40, 0.5f, 280, 150, activeListBgColor);
        C2D_Text hdrTxt; C2D_TextParse(&hdrTxt, dynamicTextBuf, "Maps - Select Game");
        C2D_TextOptimize(&hdrTxt); C2D_DrawText(&hdrTxt, C2D_WithColor, 30, 22, 0.5f, 0.45f, 0.45f, activeAccentColor);
        C2D_TextBufClear(dynamicTextBuf);
        for (size_t i = 0; i < folderGroups.size() && i < 5; i++) {
            float y = 50 + i * 25;
            bool isCursor = (selectedIndex == (int)i);
            if (isCursor)
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (100 << 24));
            u32 color = isCursor ? activeAccentColor : activeTextColor;
            char name[32];
            strncpy(name, folderGroups[i].c_str(), 28);
            name[28] = '\0';
            if (folderGroups[i].length() > 28) strcat(name, "...");
            C2D_Text folderTxt;
            C2D_TextParse(&folderTxt, dynamicTextBuf, name);
            C2D_TextOptimize(&folderTxt);
            // Draw a small folder indicator ">" before the name
            C2D_DrawText(&folderTxt, C2D_WithColor, 34, y, 0.5f, 0.5f, 0.5f, color);
            C2D_TextBufClear(dynamicTextBuf);
        }
        drawButton(backBtn);
    } else if (state == STATE_MAP_LIST) {
        // Build filtered list for the selected group
        std::vector<const V2File*> filtered;
        for (const auto& f : content.mapFiles) {
            std::string g = f.group.empty() ? "Other" : f.group;
            if (g == selectedGroup) filtered.push_back(&f);
        }
        C2D_DrawRectSolid(20, 40, 0.5f, 280, 150, activeListBgColor);
        for (size_t i = 0; i < filtered.size() && i < 5; i++) {
            float y = 50 + i * 25;
            bool isActive = (activeMapIndex >= 0 && content.mapFiles[activeMapIndex].path == filtered[i]->path);
            bool isCursor = (selectedIndex == (int)i);
            if (isCursor)
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (100 << 24));
            else if (isActive)
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (40 << 24));
            u32 color = (isCursor || isActive) ? activeAccentColor : activeTextColor;
            char name[32];
            strncpy(name, filtered[i]->name.c_str(), 28);
            name[28] = '\0';
            if (filtered[i]->name.length() > 28) strcat(name, "...");
            C2D_Text fileTxt;
            C2D_TextParse(&fileTxt, dynamicTextBuf, name);
            C2D_TextOptimize(&fileTxt);
            C2D_DrawText(&fileTxt, C2D_WithColor, 30, y, 0.5f, 0.5f, 0.5f, color);
            C2D_TextBufClear(dynamicTextBuf);
        }
        drawButton(backBtn);
    } else if (state == STATE_MAP_VIEW) {
        content.drawMap(240.0f);
    } else if (state == STATE_WALK_FOLDER_LIST) {
        C2D_DrawRectSolid(20, 40, 0.5f, 280, 150, activeListBgColor);
        C2D_Text hdrTxt2; C2D_TextParse(&hdrTxt2, dynamicTextBuf, "Guides - Select Game");
        C2D_TextOptimize(&hdrTxt2); C2D_DrawText(&hdrTxt2, C2D_WithColor, 30, 22, 0.5f, 0.45f, 0.45f, activeAccentColor);
        C2D_TextBufClear(dynamicTextBuf);
        for (size_t i = 0; i < folderGroups.size() && i < 5; i++) {
            float y = 50 + i * 25;
            bool isCursor = (selectedIndex == (int)i);
            if (isCursor)
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (100 << 24));
            u32 color = isCursor ? activeAccentColor : activeTextColor;
            char name[32];
            strncpy(name, folderGroups[i].c_str(), 28);
            name[28] = '\0';
            if (folderGroups[i].length() > 28) strcat(name, "...");
            C2D_Text folderTxt;
            C2D_TextParse(&folderTxt, dynamicTextBuf, name);
            C2D_TextOptimize(&folderTxt);
            C2D_DrawText(&folderTxt, C2D_WithColor, 34, y, 0.5f, 0.5f, 0.5f, color);
            C2D_TextBufClear(dynamicTextBuf);
        }
        drawButton(backBtn);
    } else if (state == STATE_WALK_LIST) {
        // Build filtered list for the selected group
        std::vector<const V2File*> filtered;
        for (const auto& f : content.walkFiles) {
            std::string g = f.group.empty() ? "Other" : f.group;
            if (g == selectedGroup) filtered.push_back(&f);
        }
        C2D_DrawRectSolid(20, 40, 0.5f, 280, 150, activeListBgColor);
        for (size_t i = 0; i < filtered.size() && i < 5; i++) {
            float y = 50 + i * 25;
            bool isActive = (activeWalkIndex >= 0 && content.walkFiles[activeWalkIndex].path == filtered[i]->path);
            bool isCursor = (selectedIndex == (int)i);
            if (isCursor)
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (100 << 24));
            else if (isActive)
                C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (40 << 24));
            u32 color = (isCursor || isActive) ? activeAccentColor : activeTextColor;
            char name[32];
            strncpy(name, filtered[i]->name.c_str(), 28);
            name[28] = '\0';
            if (filtered[i]->name.length() > 28) strcat(name, "...");
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
                bool isCursor = (selectedIndex == (int)i);

                if (isCursor) {
                    C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (100 << 24));
                } else if (isActive) {
                    C2D_DrawRectSolid(24, y - 3, 0.5f, 272, 21, (activeAccentColor & 0x00FFFFFF) | (40 << 24));
                }

                u32 color = (isCursor || isActive) ? activeAccentColor : activeTextColor;
                char name[32];
                std::string cleaned = cleanGameName(content.romFiles[i].name);
                strncpy(name, cleaned.c_str(), 28);
                name[28] = '\0';
                if (cleaned.length() > 28) strcat(name, "...");

                C2D_Text fileTxt;
                C2D_TextParse(&fileTxt, dynamicTextBuf, name);
                C2D_TextOptimize(&fileTxt);
                C2D_DrawText(&fileTxt, C2D_WithColor, 30, y, 0.5f, 0.5f, 0.5f, color);
                C2D_TextBufClear(dynamicTextBuf);
            }
        }
        drawButton(backBtn);
    } else if (state == STATE_LOADING_GAME) {
        // Progress bar on the bottom screen while maps are being cached.
        float progress = 0.0f;
        if (hudLoadStep == 0)
            progress = 0.0f;
        else if (hudLoadStep >= 999 || hudLoadTotal <= 0 || hudLoadStep >= hudLoadTotal)
            progress = 1.0f;
        else
            progress = (float)hudLoadStep / (float)hudLoadTotal;

        const char* loadMsg = (hudLoadStep == 0)            ? "Loading ROM..."  :
                              (hudLoadStep > hudLoadTotal)   ? "Finalizing..."   :
                                                               "Caching Maps...";
        C2D_Text loadTxt;
        C2D_TextBufClear(dynamicTextBuf);
        C2D_TextParse(&loadTxt, dynamicTextBuf, loadMsg);
        C2D_TextOptimize(&loadTxt);
        float tw, th;
        C2D_TextGetDimensions(&loadTxt, 0.55f, 0.55f, &tw, &th);
        C2D_DrawText(&loadTxt, C2D_WithColor,
            (SCREEN_WIDTH - tw) / 2.0f, 105.0f, 0.5f, 0.55f, 0.55f, activeTextColor);
        C2D_TextBufClear(dynamicTextBuf);

        // Track
        C2D_DrawRectSolid(20.0f, 124.0f, 0.8f, 280.0f, 10.0f, C2D_Color32(40, 40, 50, 255));
        // Fill (accent color, scaled by progress)
        if (progress > 0.0f)
            C2D_DrawRectSolid(20.0f, 124.0f, 0.85f, 280.0f * progress, 10.0f, activeAccentColor);
    } else if (state == STATE_PLAYING || state == STATE_EMULATING) {
        // Hold off rendering the map/guide until the NES game has reached active
        // gameplay (past title screen / intro). Otherwise show a "waiting" message.
        if (!nesGameplayActive) {
            C2D_TextBufClear(dynamicTextBuf);
            C2D_Text waitTxt;
            C2D_TextParse(&waitTxt, dynamicTextBuf, "Waiting for game to start...");
            C2D_TextOptimize(&waitTxt);
            C2D_DrawText(&waitTxt, C2D_WithColor | C2D_AlignCenter, 160, 95, 0.5f, 0.55f, 0.55f, activeTextColor);
            C2D_TextBufClear(dynamicTextBuf);
        } else if (gameplayMode == GAMEPLAY_MAP) {
            content.drawMap(209.0f, playerRamVal);
        } else {
            content.drawWalk(scrollOffset);
        }

        // Draw sleek glass background overlay bar for HUD (31px tall: y=209..240)
        C2D_DrawRectSolid(0, 209, 0.5f, 320, 31, activeHudBg);

        // HUD touch buttons — text labels, finger-sized.
        // noBorder=true: the glow overlay below provides the edge treatment.
        drawButton(hudPrevBtn, true);
        drawButton(hudNextBtn, true);
        drawButton(hudShowMapBtn, true);
        drawButton(hudShowGuideBtn, true);

        // Draw rounded-corner glow overlay on HUD buttons
        Button* hudBtns[4] = { &hudPrevBtn, &hudNextBtn, &hudShowMapBtn, &hudShowGuideBtn };
        for (int i = 0; i < 4; i++) {
            if (hudGlowReady[i]) {
                C2D_DrawImageAt(hudGlowImg[i], hudBtns[i]->x, hudBtns[i]->y, 0.6f, NULL, 1.0f, 1.0f);
            }
        }

        // Middle: battery % (right-of-center) + time (left-of-center)
        {
            // Battery % centered in the middle of the HUD bar
            const float scale = 0.45f;
            float batW, batH;
            C2D_TextGetDimensions(&batteryText, scale, scale, &batW, &batH);
            C2D_DrawText(&batteryText, C2D_WithColor,
                160.0f - batW / 2.0f, 220.0f,
                0.5f, scale, scale, batteryColor);
        }
    }

    // Notifications
    if (notifyTimer > 0) {
        notifyTimer--;
    }

    // Footer: Version (left), Clock (center), Battery (right)
    if (state != STATE_PLAYING && state != STATE_EMULATING) {
        float footerY = 226.0f;
        float footerScale = 0.45f;

        // Draw footer background strip
        C2D_DrawRectSolid(0, 224, 0.5f, SCREEN_WIDTH, 16, C2D_Color32(15, 15, 20, 220));
        C2D_DrawRectSolid(0, 224, 0.5f, SCREEN_WIDTH, 1,  C2D_Color32(255, 255, 255, 30));

        u32 footerColor = C2D_Color32(210, 210, 210, 255);

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

    // Savestate flash: the bottom glow overlay is drawn by main.cpp at z=0.9
    // whenever saveLoadFlashTimer > 0. Decrement the timer here each frame.
    if (saveLoadFlashTimer > 0) {
        saveLoadFlashTimer--;
    }
}





