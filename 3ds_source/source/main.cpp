#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <citro2d.h>
#include <citro3d.h>
#include "ui.h"
#include "settings.h"
#include "nes_core.h"
#include "3dslodepng.h"

// Embedded loading-screen logo (data/logo.png linked via bin2s in the Makefile).
// bin2s exposes the start pointer, an end sentinel, and the size.
extern "C" const u8 logo_png[];
extern "C" const u8 logo_png_end[];
extern "C" const u32 logo_png_size;

static C3D_Tex            g_logoTex;
static C2D_Image          g_logoImage = {0};
static Tex3DS_SubTexture  g_logoSubTex = {0};
static bool               g_logoReady = false;

// Embedded glow overlay PNGs (linked via bin2s in the Makefile).
// Both are 320×240. Top has glow on top/left/right (bottom edge open at hinge).
// Bottom has glow on bottom/left/right (top edge open at hinge).
// Top is drawn at x=40 to centre in the 400px top screen (matching the 320px
// bottom screen), so the side borders connect across the hinge.
extern "C" const u8 aquamarine_glow_top_png[];
extern "C" const u8 aquamarine_glow_top_png_end[];
extern "C" const u32 aquamarine_glow_top_png_size;

extern "C" const u8 aquamarine_glow_bottom_png[];
extern "C" const u8 aquamarine_glow_bottom_png_end[];
extern "C" const u32 aquamarine_glow_bottom_png_size;

static C3D_Tex            g_glowTopTex;
static C2D_Image          g_glowTopImage = {0};
static Tex3DS_SubTexture  g_glowTopSubTex = {0};
static bool               g_glowTopReady = false;

static C3D_Tex            g_glowBotTex;
static C2D_Image          g_glowBotImage = {0};
static Tex3DS_SubTexture  g_glowBotSubTex = {0};
static bool               g_glowBotReady = false;

extern volatile bool  nesFrameSucceeded;
extern volatile int   nesFrameCount;

// Decode the embedded 512x256 RGBA PNG into a GPU texture once at app startup.
static void initLogoTexture() {
    u8* rgba = nullptr;
    unsigned w = 0, h = 0;
    size_t logoBytes = (size_t)(logo_png_end - logo_png);
    if (lodepng_decode32(&rgba, &w, &h, logo_png, logoBytes) != 0 || !rgba) {
        if (rgba) free(rgba);
        return;
    }
    if (w != 512 || h != 256) {
        // Unexpected size — bail (caller will fall back to text-only loading screen).
        free(rgba);
        return;
    }
    u8* tiled = (u8*)linearAlloc(512 * 256 * 4);
    if (!tiled) { free(rgba); return; }

    // Morton-swizzle the RGBA pixels into the PICA200's tiled GPU_RGBA8 layout.
    for (u16 y = 0; y < 256; y++) {
        for (u16 x = 0; x < 512; x++) {
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
            u32 blockIndex  = blockY * (512 / 8) + blockX;
            u32 tiledOffset = (blockIndex * 64 + morton) * 4;
            u32 linearOffset = (y * 512 + x) * 4;
            // Source RGBA -> PICA200 ABGR byte order
            tiled[tiledOffset + 0] = rgba[linearOffset + 3]; // A
            tiled[tiledOffset + 1] = rgba[linearOffset + 2]; // B
            tiled[tiledOffset + 2] = rgba[linearOffset + 1]; // G
            tiled[tiledOffset + 3] = rgba[linearOffset + 0]; // R
        }
    }
    free(rgba);

    C3D_TexInit(&g_logoTex, 512, 256, GPU_RGBA8);
    C3D_TexSetFilter(&g_logoTex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexUpload(&g_logoTex, tiled);
    C3D_TexFlush(&g_logoTex);
    linearFree(tiled);

    // Crop to the visible artwork region within the 512×256 atlas.
    // The logo art sits roughly at x=22..490, y=8..235 — trim the surrounding
    // black so DrawImageAt can place the *artwork* dead-centre on screen.
    g_logoSubTex.width  = 463;
    g_logoSubTex.height = 227;
    g_logoSubTex.left   = 27.0f  / 512.0f;   // +5px left crop to visually centre artwork
    g_logoSubTex.top    = 235.0f / 256.0f;   // citro2d: top = max V (bottom row of crop)
    g_logoSubTex.right  = 490.0f / 512.0f;
    g_logoSubTex.bottom = 8.0f   / 256.0f;   // citro2d: bottom = min V (top row of crop)
    g_logoImage.tex     = &g_logoTex;
    g_logoImage.subtex  = &g_logoSubTex;
    g_logoReady = true;
}

// Decode an embedded RGBA PNG into a GPU texture (power-of-2 padded).
// Used for the glow overlay borders on both screens.
static bool initGlowTexture(const u8* pngData, const u8* pngEnd,
                             unsigned expectW, unsigned expectH,
                             unsigned texW, unsigned texH,
                             C3D_Tex* tex, C2D_Image* img,
                             Tex3DS_SubTexture* subtex) {
    u8* rgba = nullptr;
    unsigned w = 0, h = 0;
    size_t bytes = (size_t)(pngEnd - pngData);
    if (lodepng_decode32(&rgba, &w, &h, pngData, bytes) != 0 || !rgba) {
        if (rgba) free(rgba);
        return false;
    }
    if (w != expectW || h != expectH) { free(rgba); return false; }

    u8* tiled = (u8*)linearAlloc(texW * texH * 4);
    if (!tiled) { free(rgba); return false; }
    memset(tiled, 0, texW * texH * 4);   // zero-fill padding (fully transparent)

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
            tiled[tiledOffset + 0] = rgba[linearOff + 3]; // A
            tiled[tiledOffset + 1] = rgba[linearOff + 2]; // B
            tiled[tiledOffset + 2] = rgba[linearOff + 1]; // G
            tiled[tiledOffset + 3] = rgba[linearOff + 0]; // R
        }
    }
    free(rgba);

    C3D_TexInit(tex, texW, texH, GPU_RGBA8);
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexUpload(tex, tiled);
    C3D_TexFlush(tex);
    linearFree(tiled);

    subtex->width  = expectW;
    subtex->height = expectH;
    subtex->left   = 0.0f;
    subtex->top    = (float)expectH / (float)texH;
    subtex->right  = (float)expectW / (float)texW;
    subtex->bottom = 0.0f;
    img->tex    = tex;
    img->subtex = subtex;
    return true;
}

void saveRomBookmark(const char* path) {
	FILE* f = fopen("sdmc:/V2NES/bookmark_rom.txt", "wb");
	if (f) { fputs(path, f); fclose(f); }
}

// Clean a NES ROM filename into a readable game title
// "Legend of Zelda, The (USA).nes" → "Legend of Zelda"
std::string cleanGameName(const std::string& filename) {
	std::string s = filename;
	// Strip path
	size_t slash = s.find_last_of("/\\");
	if (slash != std::string::npos) s = s.substr(slash + 1);
	// Strip .nes extension
	size_t dot = s.rfind(".nes");
	if (dot == std::string::npos) dot = s.rfind(".NES");
	if (dot != std::string::npos) s = s.substr(0, dot);
	// Strip anything in parentheses or brackets: (USA), [!], (Rev A), etc.
	std::string clean;
	int depth = 0;
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] == '(' || s[i] == '[') { depth++; continue; }
		if (s[i] == ')' || s[i] == ']') { if (depth > 0) depth--; continue; }
		if (depth == 0) clean += s[i];
	}
	// Trim trailing whitespace, commas, dashes
	while (!clean.empty() && (clean.back() == ' ' || clean.back() == ',' || clean.back() == '-' || clean.back() == '_'))
		clean.pop_back();
	// Handle "Name, The" → "Name" (strip trailing ", The")
	if (clean.size() > 5) {
		std::string tail = clean.substr(clean.size() - 5);
		if (tail == ", The" || tail == ", the")
			clean = clean.substr(0, clean.size() - 5);
	}
	// Replace underscores with spaces
	for (size_t i = 0; i < clean.size(); i++)
		if (clean[i] == '_') clean[i] = ' ';
	return clean;
}

// Helper to extract the prefix from a game map name to identify maps from the same game
std::string getGamePrefix(const std::string& filename) {
	std::string base = filename;
	size_t slash = base.find_last_of("/\\");
	if (slash != std::string::npos) {
		base = base.substr(slash + 1);
	}
	size_t dot = base.find_last_of('.');
	if (dot != std::string::npos) {
		base = base.substr(0, dot);
	}
	size_t dash = base.find_last_of('-');
	if (dash != std::string::npos) {
		base = base.substr(0, dash);
	}
	return base;
}

// Simulated NES RAM array (2KB of work RAM)
u8 nesRAM[0x800] = {0};

void updateRAMMapSwitching(const std::string& prefix, int& currentMapIndex, V2UI& ui) {
    static int lastRAMVal = -1;
    static int pendingVal = -1;
    static int pendingFrames = 0;

    // Determine which RAM byte to monitor based on the running game prefix
    bool isZelda = (prefix.find("Zelda") != std::string::npos);
    bool isMario = (prefix.find("Mario") != std::string::npos);

    int currentRAMVal = 0;
    int currentQuest = 0;
    if (isZelda) {
        currentRAMVal = nesReadRAM(0x0010); // Zelda 1 dungeon byte (0=overworld, 1-9=dungeons)
        // Quest indicator address still unconfirmed — leave at 0 until we identify the real one.
    } else if (isMario) {
        currentRAMVal = nesReadRAM(0x075C); // SMB level byte (0=1-1, 1=1-2, etc.)
    } else {
        return; // RAM switching not set up for this game prefix
    }

    // Debounce: only act on a value that has been stable for 60 frames (~1 second).
    // This filters out transient RAM flickers during scene transitions or animations
    // that would otherwise cause the map to oscillate.
    if (currentRAMVal == lastRAMVal) {
        pendingVal = -1;
        pendingFrames = 0;
        return;
    }
    if (currentRAMVal != pendingVal) {
        pendingVal = currentRAMVal;
        pendingFrames = 1;
        return;
    }
    if (++pendingFrames < 60) {
        return; // Wait for the value to settle
    }

    lastRAMVal = currentRAMVal;
    
    // Search for a matching map file in our list
    int foundIndex = -1;
    
    if (isZelda) {
        if (currentRAMVal == 0) {
            // Overworld: Search for exactly the clean prefix map (no suffixes)
            for (size_t i = 0; i < ui.content.mapFiles.size(); i++) {
                std::string name = ui.content.mapFiles[i].name;
                if (name == prefix + ".v2m" || name == prefix ||
                    name.find("Overworld") != std::string::npos) {
                    foundIndex = (int)i;
                    break;
                }
            }
        } else {
            // Dungeon level. RAM value 1..9 maps directly to dungeon number.
            // Prefer maps that match the current quest (Q1/Q2) when multiple matches exist.
            int levelNum = currentRAMVal;
            char questTag[8];
            snprintf(questTag, sizeof(questTag), "Q%d", currentQuest + 1);

            char sub1[32], sub2[32], sub3[32], sub4[32];
            snprintf(sub1, sizeof(sub1), "Level%d", levelNum);
            snprintf(sub2, sizeof(sub2), "Dungeon%d", levelNum);
            snprintf(sub3, sizeof(sub3), "Level %d", levelNum);
            snprintf(sub4, sizeof(sub4), "-%d", levelNum);

            int genericMatch = -1;
            for (size_t i = 0; i < ui.content.mapFiles.size(); i++) {
                std::string name = ui.content.mapFiles[i].name;
                bool levelMatches =
                    name.find(sub1) != std::string::npos ||
                    name.find(sub2) != std::string::npos ||
                    name.find(sub3) != std::string::npos ||
                    name.find(sub4) != std::string::npos;
                if (!levelMatches) continue;

                // Prefer a quest-tagged match (e.g. "Q1") over a generic level match
                if (name.find(questTag) != std::string::npos) {
                    foundIndex = (int)i;
                    break;
                }
                if (genericMatch == -1) genericMatch = (int)i;
            }
            if (foundIndex == -1 && genericMatch != -1) foundIndex = genericMatch;
        }
    } else if (isMario) {
        // SMB Levels: 0 -> Level 1-1, 1 -> Level 1-2, etc.
        char sub1[32], sub2[32];
        snprintf(sub1, sizeof(sub1), "-1-%d", currentRAMVal + 1);
        snprintf(sub2, sizeof(sub2), "-%d", currentRAMVal + 1);
        
        for (size_t i = 0; i < ui.content.mapFiles.size(); i++) {
            std::string name = ui.content.mapFiles[i].name;
            if (name.find(sub1) != std::string::npos ||
                name.find(sub2) != std::string::npos) {
                foundIndex = (int)i;
                break;
            }
        }
    }
    
    if (foundIndex != -1 && foundIndex != currentMapIndex) {
        currentMapIndex = foundIndex;
        if (ui.content.loadMap(ui.content.mapFiles[currentMapIndex].path.c_str())) {
            ui.activeMapIndex = currentMapIndex;
            // Evict everything except overworld + newly loaded dungeon to keep VRAM free.
            const std::string& newPath = ui.content.mapFiles[currentMapIndex].path;
            const std::string& owPath  = ui.content.overworldMapPath;
            if (!owPath.empty() && newPath != owPath)
                ui.content.freeAllCachedMapsExcept(owPath, newPath);
            char notifyMsg[64];
            snprintf(notifyMsg, sizeof(notifyMsg), "Auto-Map: %s", ui.content.mapFiles[currentMapIndex].name.c_str());
            ui.showNotification(notifyMsg);
            printf("\x1b[10;1H[RAM] Changed -> Loaded: %s            ", ui.content.mapFiles[currentMapIndex].name.c_str());
        }
    }
}

int main(int argc, char* argv[])
{
	// Initialize services
	gfxInitDefault();
	cfguInit(); 
	ptmuInit();

	// Initialize audio
	#define AUDIO_RATE 22050
	#define AUDIO_SAMPLES 368   // ceil(22050/60); actual count per frame alternates 367/368 via accumulator
	#define AUDIO_BUF_COUNT 8
	bool audioOk = false;
	Result dspRes = dspInit();
	Result ndspRes = ndspInit();
	if (R_SUCCEEDED(dspRes) && R_SUCCEEDED(ndspRes)) {
		audioOk = true;
		ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		ndspChnReset(10);
		ndspChnSetFormat(10, NDSP_FORMAT_MONO_PCM16);
		ndspChnSetRate(10, (float)AUDIO_RATE);
		ndspChnSetInterp(10, NDSP_INTERP_NONE);
		float mix[12] = {0}; mix[0] = 1.0f; mix[1] = 1.0f;
		ndspChnSetMix(10, mix);
	}
	static short* audioBufs[AUDIO_BUF_COUNT];
	static ndspWaveBuf waveBufs[AUDIO_BUF_COUNT] = {{0}};
	if (audioOk) {
		for (int i = 0; i < AUDIO_BUF_COUNT; i++) {
			audioBufs[i] = (short*)linearAlloc(AUDIO_SAMPLES * sizeof(short));
			if (audioBufs[i]) {
				memset(audioBufs[i], 0, AUDIO_SAMPLES * sizeof(short));
				waveBufs[i].data_vaddr = audioBufs[i];
				waveBufs[i].nsamples = 0;
				waveBufs[i].status = NDSP_WBUF_FREE;
			}
		}
	}
	static int audioBufIdx = 0;

	V2UI ui;
	ui.init();

	// Decode the embedded loading-screen logo (must run AFTER C3D/C2D init).
	initLogoTexture();

	// Decode glow border overlays into GPU textures (both 320x240 → 512x256).
	g_glowTopReady = initGlowTexture(aquamarine_glow_top_png, aquamarine_glow_top_png_end,
	                                  320, 240, 512, 256,
	                                  &g_glowTopTex, &g_glowTopImage, &g_glowTopSubTex);
	g_glowBotReady = initGlowTexture(aquamarine_glow_bottom_png, aquamarine_glow_bottom_png_end,
	                                  320, 240, 512, 256,
	                                  &g_glowBotTex, &g_glowBotImage, &g_glowBotSubTex);

	V2Settings settings;
	settings.load();

	// NES core initialization
	nesInit();
	// Emulation runs synchronously on core 0 (parallel threading on core 1 was
	// net-negative: Old 3DS core 1 is slower and doesn't finish within vsync budget).
	// nesKickFrame() / nesWaitFrame() fall back to sync when g_emuThread == NULL.
	C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_Tex nesTex;
	C2D_Image nesImage = {0};
	Tex3DS_SubTexture nesSubTex = { 256, 240, 0.0f, 240.0f/256.0f, 1.0f, 0.0f };
	if (topTarget) {
		C3D_TexInit(&nesTex, 256, 256, GPU_RGB565);
		C3D_TexSetFilter(&nesTex, GPU_NEAREST, GPU_NEAREST);
		nesImage.tex = &nesTex;
		nesImage.subtex = &nesSubTex;
	}

	GameState state = STATE_STARTUP;
	int startupTimer = 0;   // counts frames; auto-advances to menu after ~3 s
	int currentMapIndex = 0;
	bool cameFromPlaying = false;
	bool nesRomLoaded = false;
	// Pending game-start: when set, the next loop iteration will do the heavy
	// ROM load + map preload while the user sees the "Loading..." screen.
	// -1 = "use bookmark / fallback to first ROM" (Play button path).
	// >= 0 = specific romFiles index (Game List path).
	int pendingRomIndex = -2; // -2 means "nothing pending"
	// Progressive loading: one map per frame to avoid multi-second freeze
	int    loadStep   = 0;
	int    loadTotal  = 0;
	size_t loadCursor = 0;
	int    loadChosenIdx = 0;
	std::string loadPrefix;
	nesRAM[0x0010] = 1; // Zelda starts in Overworld

	// Console disabled - top screen uses C2D for NES rendering

	// Top screen overlays — two separate buffers so debug and notifications
	// never stomp each other. topNotifyBuf is cleared every time a toast fires;
	// dbgOverlayBuf is cleared only on the ~4 Hz debug refresh tick.
	C2D_TextBuf topNotifyBuf  = C2D_TextBufNew(128);
	char topNotifyMsg[64] = {0};
	int topNotifyTimer = 0;
	int currentSlot = 1; // 1-3

	u64 lastBatteryCheck = 0;
	u64 lastEmuTime = 0;   // frame-pacing: emulate enough frames to stay in sync with wall-clock

	// Main loop
	while (aptMainLoop())
	{
		u64 loopStartMs = osGetTime();

		// Scan all the inputs
		hidScanInput();

		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();

		if ((kDown & KEY_START) && state != STATE_STARTUP && state != STATE_EMULATING && state != STATE_PLAYING && state != STATE_LOADING_GAME)
			break;

		// Startup splash: auto-advance after ~3 s (180 frames).
		// Button skip is only active after 30 frames to avoid held-key instant-skip.
		if (state == STATE_STARTUP) {
			++startupTimer;
			if (startupTimer >= 180 || (startupTimer >= 30 && kDown))
				state = STATE_MENU;
		}

		// L + R toggles between top screen (emulating) and bottom screen (playing)
		if ((kDown & KEY_R) && (kHeld & KEY_L) && (state == STATE_EMULATING || state == STATE_PLAYING)) {
			state = (state == STATE_EMULATING) ? STATE_PLAYING : STATE_EMULATING;
		}

		// L + Up/Down cycles save slot, L + Y saves, L + X loads
		if ((state == STATE_EMULATING || state == STATE_PLAYING) && (kHeld & KEY_L)) {
			if (kDown & KEY_UP) {
				currentSlot++;
				if (currentSlot > 3) currentSlot = 1;
				snprintf(topNotifyMsg, sizeof(topNotifyMsg), "Slot %d Selected", currentSlot);
				topNotifyTimer = 90;
			}
			if (kDown & KEY_DOWN) {
				currentSlot--;
				if (currentSlot < 1) currentSlot = 3;
				snprintf(topNotifyMsg, sizeof(topNotifyMsg), "Slot %d Selected", currentSlot);
				topNotifyTimer = 90;
			}
			if (kDown & KEY_Y) {
				if (nesSaveState(currentSlot)) {
					snprintf(topNotifyMsg, sizeof(topNotifyMsg), "State %d Saved!", currentSlot);
				} else {
					snprintf(topNotifyMsg, sizeof(topNotifyMsg), "Save Failed!");
				}
				topNotifyTimer = 120;
				ui.saveLoadFlashTimer = 5;
			}
			if (kDown & KEY_X) {
				if (nesLoadState(currentSlot)) {
					snprintf(topNotifyMsg, sizeof(topNotifyMsg), "State %d Loaded!", currentSlot);
				} else {
					snprintf(topNotifyMsg, sizeof(topNotifyMsg), "No State %d Found!", currentSlot);
				}
				topNotifyTimer = 120;
				ui.saveLoadFlashTimer = 5;
			}
		}

		// Handle Touch Visuals (Highlighting)
		if (kHeld & KEY_TOUCH) {
			touchPosition touch;
			hidTouchRead(&touch);
			if (state == STATE_MENU) {
				ui.checkTouch(touch, ui.playBtn);
				ui.checkTouch(touch, ui.mapsBtn);
				ui.checkTouch(touch, ui.walkBtn);
				ui.checkTouch(touch, ui.settingsBtn);
				ui.checkTouch(touch, ui.listBtn);
			} else if (state == STATE_SETTINGS) {
				ui.checkTouch(touch, ui.speedCatBtn);
				ui.checkTouch(touch, ui.backBtn);
			} else if (state == STATE_SETTINGS_SENSITIVITY) {
				ui.checkTouch(touch, ui.zoomSpeedBtn);
				ui.checkTouch(touch, ui.panSpeedBtn);
				ui.checkTouch(touch, ui.scrollSpeedBtn);
				ui.checkTouch(touch, ui.backBtn);
			} else if (state == STATE_GAME_LIST) {
				ui.checkTouch(touch, ui.backBtn);
			} else if (state == STATE_PLAYING || state == STATE_EMULATING) {
				ui.checkTouch(touch, ui.hudPrevBtn);
				ui.checkTouch(touch, ui.hudNextBtn);
				ui.checkTouch(touch, ui.hudShowMapBtn);
				ui.checkTouch(touch, ui.hudShowGuideBtn);
			}
		} else {
			ui.playBtn.pressed = ui.mapsBtn.pressed = ui.walkBtn.pressed = false;
			ui.settingsBtn.pressed = ui.listBtn.pressed = false;
			ui.speedCatBtn.pressed = false;
			ui.zoomSpeedBtn.pressed = ui.panSpeedBtn.pressed = ui.scrollSpeedBtn.pressed = false;
			ui.backBtn.pressed = false;
			ui.hudPrevBtn.pressed = ui.hudNextBtn.pressed = false;
			ui.hudShowMapBtn.pressed = ui.hudShowGuideBtn.pressed = false;
		}

		// Handle Touch Actions (Single Trigger)
		static bool touchPending = false;
		if (kDown & KEY_TOUCH) {
			touchPending = true;
		}
		if (!(kHeld & KEY_TOUCH)) {
			touchPending = false;
		}
		if (touchPending && (kHeld & KEY_TOUCH)) {
			touchPosition touch;
			hidTouchRead(&touch);
			if (touch.px != 0 || touch.py != 0) {
				touchPending = false;
				
				// Debug coordinate printing
								printf("\x1b[8;1HTap registered: px=%3d, py=%3d  ", touch.px, touch.py);

				if (state == STATE_MENU) {
				if (ui.checkTouch(touch, ui.playBtn)) {
					ui.content.scanFiles();
					if (ui.content.romFiles.empty()) {
						ui.showNotification("No ROMs Found! Copy to /V2NES/roms/");
					} else {
						// Defer the heavy load to next iteration so the user sees
						// the loading screen first.
						pendingRomIndex = -1; // -1 = bookmark / fallback to first
						state = STATE_LOADING_GAME;
					}
				}
				else if (ui.checkTouch(touch, ui.mapsBtn)) {
					ui.content.scanFiles();
					ui.folderGroups = V2Content::getGroups(ui.content.mapFiles);
					state = ui.folderGroups.size() == 1 ? STATE_MAP_LIST : STATE_MAP_FOLDER_LIST;
					if (state == STATE_MAP_LIST) ui.selectedGroup = ui.folderGroups[0];
					ui.selectedIndex = 0;
				}
				else if (ui.checkTouch(touch, ui.walkBtn)) {
					ui.content.scanFiles();
					ui.folderGroups = V2Content::getGroups(ui.content.walkFiles);
					state = ui.folderGroups.size() == 1 ? STATE_WALK_LIST : STATE_WALK_FOLDER_LIST;
					if (state == STATE_WALK_LIST) ui.selectedGroup = ui.folderGroups[0];
					ui.selectedIndex = 0;
				}
				else if (ui.checkTouch(touch, ui.settingsBtn)) {
					state = STATE_SETTINGS;
				}
				else if (ui.checkTouch(touch, ui.listBtn)) {
					ui.content.scanFiles();
					state = STATE_GAME_LIST;
					ui.selectedIndex = 0;
				}
			} else if (state == STATE_SETTINGS) {
				if (ui.checkTouch(touch, ui.speedCatBtn)) {
					state = STATE_SETTINGS_SENSITIVITY;
				}
				else if (ui.checkTouch(touch, ui.backBtn)) {
					state = STATE_MENU;
				}
			} else if (state == STATE_SETTINGS_SENSITIVITY) {
				if (ui.checkTouch(touch, ui.zoomSpeedBtn)) {
					int s = (settings.getZoomSpeed() % 3) + 1;
					settings.setZoomSpeed(s);
					settings.save();
					char msg[64];
					snprintf(msg, sizeof(msg), "Zoom Speed: %s", settings.getSpeedName(s));
					ui.showNotification(msg);
				}
				else if (ui.checkTouch(touch, ui.panSpeedBtn)) {
					int s = (settings.getPanSpeed() % 3) + 1;
					settings.setPanSpeed(s);
					settings.save();
					char msg[64];
					snprintf(msg, sizeof(msg), "Pan Speed: %s", settings.getSpeedName(s));
					ui.showNotification(msg);
				}
				else if (ui.checkTouch(touch, ui.scrollSpeedBtn)) {
					int s = (settings.getScrollSpeed() % 3) + 1;
					settings.setScrollSpeed(s);
					settings.save();
					char msg[64];
					snprintf(msg, sizeof(msg), "Scroll Speed: %s", settings.getSpeedName(s));
					ui.showNotification(msg);
				}
				else if (ui.checkTouch(touch, ui.backBtn)) {
					state = STATE_SETTINGS;
				}
			} else if (state == STATE_MAP_FOLDER_LIST) {
				if (ui.checkTouch(touch, ui.backBtn)) {
					state = cameFromPlaying ? STATE_PLAYING : STATE_MENU;
					if (cameFromPlaying) cameFromPlaying = false;
				} else {
					for (size_t i = 0; i < ui.folderGroups.size() && i < 5; i++) {
						float itemY = 50 + i * 25;
						if (touch.py > itemY && touch.py < itemY + 25) {
							ui.selectedGroup = ui.folderGroups[i];
							ui.selectedIndex = 0;
							state = STATE_MAP_LIST;
						}
					}
				}
			} else if (state == STATE_MAP_LIST) {
				if (ui.checkTouch(touch, ui.backBtn)) {
					// Back to folder list if there are multiple folders, else menu/game
					if (ui.folderGroups.size() > 1) {
						state = STATE_MAP_FOLDER_LIST;
						ui.selectedIndex = 0;
					} else {
						state = cameFromPlaying ? STATE_PLAYING : STATE_MENU;
						if (cameFromPlaying) cameFromPlaying = false;
					}
				} else {
					// Build filtered list to get the right paths
					std::vector<int> filtered;
					for (int i = 0; i < (int)ui.content.mapFiles.size(); i++) {
						std::string g = ui.content.mapFiles[i].group.empty() ? "Other" : ui.content.mapFiles[i].group;
						if (g == ui.selectedGroup) filtered.push_back(i);
					}
					for (size_t i = 0; i < filtered.size() && i < 5; i++) {
						float itemY = 50 + i * 25;
						if (touch.py > itemY && touch.py < itemY + 25) {
							ui.selectedIndex = i;
							int realIdx = filtered[i];
							if (ui.content.loadMap(ui.content.mapFiles[realIdx].path.c_str())) {
								currentMapIndex = realIdx;
								ui.activeMapIndex = realIdx;
								if (cameFromPlaying) {
									state = STATE_PLAYING;
									cameFromPlaying = false;
								} else {
									state = STATE_MAP_VIEW;
								}
							} else {
								ui.showNotification("Load Failed");
							}
						}
					}
				}
			} else if (state == STATE_WALK_FOLDER_LIST) {
				if (ui.checkTouch(touch, ui.backBtn)) {
					state = cameFromPlaying ? STATE_PLAYING : STATE_MENU;
					if (cameFromPlaying) cameFromPlaying = false;
				} else {
					for (size_t i = 0; i < ui.folderGroups.size() && i < 5; i++) {
						float itemY = 50 + i * 25;
						if (touch.py > itemY && touch.py < itemY + 25) {
							ui.selectedGroup = ui.folderGroups[i];
							ui.selectedIndex = 0;
							state = STATE_WALK_LIST;
						}
					}
				}
			} else if (state == STATE_WALK_LIST) {
				if (ui.checkTouch(touch, ui.backBtn)) {
					if (ui.folderGroups.size() > 1) {
						state = STATE_WALK_FOLDER_LIST;
						ui.selectedIndex = 0;
					} else {
						state = cameFromPlaying ? STATE_PLAYING : STATE_MENU;
						if (cameFromPlaying) cameFromPlaying = false;
					}
				} else {
					std::vector<int> filtered;
					for (int i = 0; i < (int)ui.content.walkFiles.size(); i++) {
						std::string g = ui.content.walkFiles[i].group.empty() ? "Other" : ui.content.walkFiles[i].group;
						if (g == ui.selectedGroup) filtered.push_back(i);
					}
					for (size_t i = 0; i < filtered.size() && i < 5; i++) {
						float itemY = 50 + i * 25;
						if (touch.py > itemY && touch.py < itemY + 25) {
							ui.selectedIndex = i;
							int realIdx = filtered[i];
							if (ui.content.loadWalk(ui.content.walkFiles[realIdx].path.c_str())) {
								ui.activeWalkIndex = realIdx;
								if (cameFromPlaying) {
									state = STATE_PLAYING;
									ui.gameplayMode = GAMEPLAY_GUIDE;
									cameFromPlaying = false;
								} else {
									state = STATE_WALK_VIEW;
								}
								ui.scrollOffset = ui.content.loadWalkPosition();
							} else {
								ui.showNotification("Load Failed");
							}
						}
					}
				}
			} else if (state == STATE_GAME_LIST) {
				if (ui.checkTouch(touch, ui.backBtn)) {
					state = STATE_MENU;
				} else {
					for (size_t i = 0; i < ui.content.romFiles.size() && i < 5; i++) {
						float itemY = 50 + i * 25;
						if (touch.py > itemY && touch.py < itemY + 25) {
							ui.selectedIndex = i;
							// Defer the heavy load to next iteration so the user
							// sees the loading screen first.
							pendingRomIndex = (int)i;
							state = STATE_LOADING_GAME;
							break;
						}
					}
				}
			} else if (state == STATE_PLAYING || state == STATE_EMULATING) {
				// Any touch while emulating pauses the game and opens the bottom screen
				if (state == STATE_EMULATING) {
					state = STATE_PLAYING;
				}
				// Handle Touch HUD Button Interactions
				if (ui.checkTouch(touch, ui.hudShowMapBtn)) {
					if (ui.gameplayMode == GAMEPLAY_MAP) {
						ui.content.scanFiles();
						ui.folderGroups = V2Content::getGroups(ui.content.mapFiles);
						cameFromPlaying = true;
						if (ui.folderGroups.size() == 1) {
							ui.selectedGroup = ui.folderGroups[0];
							state = STATE_MAP_LIST;
							ui.selectedIndex = 0;
						} else {
							state = STATE_MAP_FOLDER_LIST;
							ui.selectedIndex = 0;
						}
					} else {
						ui.gameplayMode = GAMEPLAY_MAP;
						ui.showNotification("Mode: Map");
					}
				}
				else if (ui.checkTouch(touch, ui.hudShowGuideBtn)) {
					if (ui.gameplayMode == GAMEPLAY_GUIDE) {
						ui.content.scanFiles();
						ui.folderGroups = V2Content::getGroups(ui.content.walkFiles);
						cameFromPlaying = true;
						if (ui.folderGroups.size() == 1) {
							ui.selectedGroup = ui.folderGroups[0];
							state = STATE_WALK_LIST;
							ui.selectedIndex = 0;
						} else {
							state = STATE_WALK_FOLDER_LIST;
							ui.selectedIndex = 0;
						}
					} else {
						ui.gameplayMode = GAMEPLAY_GUIDE;
						ui.showNotification("Mode: Guides");
					}
				}
				else if (ui.checkTouch(touch, ui.hudPrevBtn)) {
					if (!ui.content.mapFiles.empty()) {
						std::string currentPrefix = getGamePrefix(ui.content.mapFiles[currentMapIndex].name);
						std::vector<int> matchingIndices;
						for (size_t i = 0; i < ui.content.mapFiles.size(); i++) {
							if (getGamePrefix(ui.content.mapFiles[i].name) == currentPrefix) {
								matchingIndices.push_back((int)i);
							}
						}
						if (!matchingIndices.empty()) {
							int pos = -1;
							for (size_t i = 0; i < matchingIndices.size(); i++) {
								if (matchingIndices[i] == currentMapIndex) {
									pos = (int)i;
									break;
								}
							}
							if (pos != -1) {
								pos = (pos - 1 + matchingIndices.size()) % matchingIndices.size();
								currentMapIndex = matchingIndices[pos];
							} else {
								currentMapIndex = matchingIndices.back();
							}
						} else {
							if (currentMapIndex == 0) currentMapIndex = ui.content.mapFiles.size() - 1;
							else currentMapIndex--;
						}
						
						if (ui.content.loadMap(ui.content.mapFiles[currentMapIndex].path.c_str())) {
							const std::string& np = ui.content.mapFiles[currentMapIndex].path;
							const std::string& ow = ui.content.overworldMapPath;
							if (!ow.empty() && np != ow)
								ui.content.freeAllCachedMapsExcept(ow, np);
							char msg[64];
							snprintf(msg, sizeof(msg), "Level Map: %s", ui.content.mapFiles[currentMapIndex].name.c_str());
							ui.showNotification(msg);
						} else {
							ui.showNotification("Load Failed");
						}
					}
				}
				else if (ui.checkTouch(touch, ui.hudNextBtn)) {
					if (!ui.content.mapFiles.empty()) {
						std::string currentPrefix = getGamePrefix(ui.content.mapFiles[currentMapIndex].name);
						std::vector<int> matchingIndices;
						for (size_t i = 0; i < ui.content.mapFiles.size(); i++) {
							if (getGamePrefix(ui.content.mapFiles[i].name) == currentPrefix) {
								matchingIndices.push_back((int)i);
							}
						}
						if (!matchingIndices.empty()) {
							int pos = -1;
							for (size_t i = 0; i < matchingIndices.size(); i++) {
								if (matchingIndices[i] == currentMapIndex) {
									pos = (int)i;
									break;
								}
							}
							if (pos != -1) {
								pos = (pos + 1) % matchingIndices.size();
								currentMapIndex = matchingIndices[pos];
							} else {
								currentMapIndex = matchingIndices.front();
							}
						} else {
							currentMapIndex = (currentMapIndex + 1) % ui.content.mapFiles.size();
						}
						
						if (ui.content.loadMap(ui.content.mapFiles[currentMapIndex].path.c_str())) {
							const std::string& np = ui.content.mapFiles[currentMapIndex].path;
							const std::string& ow = ui.content.overworldMapPath;
							if (!ow.empty() && np != ow)
								ui.content.freeAllCachedMapsExcept(ow, np);
							char msg[64];
							snprintf(msg, sizeof(msg), "Level Map: %s", ui.content.mapFiles[currentMapIndex].name.c_str());
							ui.showNotification(msg);
						} else {
							ui.showNotification("Load Failed");
						}
					}
				}
			}
			}
		}

		// Handle Scrolling in Walkthrough
		if (state == STATE_WALK_VIEW) {
			int scrollStep = (settings.getScrollSpeed() == 1) ? 2 : ((settings.getScrollSpeed() == 3) ? 6 : 4);
			if (kHeld & KEY_UP) ui.scrollOffset -= scrollStep;
			if (kHeld & KEY_DOWN) ui.scrollOffset += scrollStep;
			if (ui.scrollOffset < 0) ui.scrollOffset = 0;
			int maxScroll = ui.content.getWalkHeight() - 200;
			if (maxScroll < 0) maxScroll = 0;
			if (ui.scrollOffset > maxScroll) ui.scrollOffset = maxScroll;
		}

		// Handle Touch stylus drag-to-pan in Map Mode
		static bool wasTouching = false;
		static u16 prevTouchX = 0;
		static u16 prevTouchY = 0;
		if (state == STATE_MAP_VIEW || (state == STATE_PLAYING && ui.gameplayMode == GAMEPLAY_MAP)) {
			if (kHeld & KEY_TOUCH) {
				touchPosition touch;
				hidTouchRead(&touch);
				// Drag/pan if touching above the HUD overlay bar (y < 209)
				if (state == STATE_MAP_VIEW || touch.py < 209) {
					if (wasTouching) {
						float dx = (float)touch.px - (float)prevTouchX;
						float dy = (float)touch.py - (float)prevTouchY;
						if (dx != 0.0f || dy != 0.0f) {
							ui.content.pan(dx, dy);
						}
					}
					prevTouchX = touch.px;
					prevTouchY = touch.py;
					wasTouching = true;
				} else {
					wasTouching = false;
				}
			} else {
				wasTouching = false;
			}
		}

		// Handle Map Zoom and Pan (Only active during gameplay when Map mode is toggled on)
		if (state == STATE_MAP_VIEW || (state == STATE_PLAYING && ui.gameplayMode == GAMEPLAY_MAP)) {
			float zoomFactor = (settings.getZoomSpeed() == 1) ? 1.07f : ((settings.getZoomSpeed() == 3) ? 1.25f : 1.15f);
			int panStep = (settings.getPanSpeed() == 1) ? 3 : ((settings.getPanSpeed() == 3) ? 9 : 6);

			if (kDown & KEY_Y) ui.content.zoomIn(zoomFactor);
			if (kDown & KEY_X) ui.content.zoomOut(zoomFactor);
			if (kHeld & KEY_UP) ui.content.pan(0, panStep);
			if (kHeld & KEY_DOWN) ui.content.pan(0, -panStep);
			if (kHeld & KEY_LEFT) ui.content.pan(panStep, 0);
			if (kHeld & KEY_RIGHT) ui.content.pan(-panStep, 0);
		}

		// Handle Walkthrough Scrolling with D-pad during Gameplay (Only active when Guide mode is toggled on)
		if (state == STATE_PLAYING && ui.gameplayMode == GAMEPLAY_GUIDE) {
			int scrollStep = (settings.getScrollSpeed() == 1) ? 2 : ((settings.getScrollSpeed() == 3) ? 6 : 4);
			if (kHeld & KEY_UP) {
				ui.scrollOffset -= scrollStep;
				if (ui.scrollOffset < 0) ui.scrollOffset = 0;
			}
			if (kHeld & KEY_DOWN) {
				ui.scrollOffset += scrollStep;
				int maxScroll = ui.content.getWalkHeight() - 180;
				if (maxScroll < 0) maxScroll = 0;
				if (ui.scrollOffset > maxScroll) ui.scrollOffset = maxScroll;
			}
		}

		// D-pad navigation and A-button confirm for list screens
		if (state == STATE_MAP_FOLDER_LIST || state == STATE_WALK_FOLDER_LIST) {
			int listSize = (int)ui.folderGroups.size();
			if (listSize > 5) listSize = 5;
			if (listSize > 0) {
				if (kDown & KEY_DOWN) ui.selectedIndex = (ui.selectedIndex + 1) % listSize;
				if (kDown & KEY_UP)   ui.selectedIndex = (ui.selectedIndex - 1 + listSize) % listSize;
			}
			if ((kDown & KEY_A) && ui.selectedIndex >= 0 && ui.selectedIndex < listSize) {
				ui.selectedGroup = ui.folderGroups[ui.selectedIndex];
				ui.selectedIndex = 0;
				state = (state == STATE_MAP_FOLDER_LIST) ? STATE_MAP_LIST : STATE_WALK_LIST;
			}
			if (kDown & KEY_B) {
				state = cameFromPlaying ? STATE_PLAYING : STATE_MENU;
				if (cameFromPlaying) cameFromPlaying = false;
			}
		}

		if (state == STATE_MAP_LIST || state == STATE_WALK_LIST || state == STATE_GAME_LIST) {
			// Build filtered index list for map/walk lists
			std::vector<int> filtered;
			if (state == STATE_MAP_LIST) {
				for (int i = 0; i < (int)ui.content.mapFiles.size(); i++) {
					std::string g = ui.content.mapFiles[i].group.empty() ? "Other" : ui.content.mapFiles[i].group;
					if (g == ui.selectedGroup) filtered.push_back(i);
				}
			} else if (state == STATE_WALK_LIST) {
				for (int i = 0; i < (int)ui.content.walkFiles.size(); i++) {
					std::string g = ui.content.walkFiles[i].group.empty() ? "Other" : ui.content.walkFiles[i].group;
					if (g == ui.selectedGroup) filtered.push_back(i);
				}
			}

			int rawSize = (state == STATE_GAME_LIST) ? (int)ui.content.romFiles.size() : (int)filtered.size();
			int listSize = (rawSize < 5) ? rawSize : 5;

			if (listSize > 0) {
				if (kDown & KEY_DOWN) ui.selectedIndex = (ui.selectedIndex + 1) % listSize;
				if (kDown & KEY_UP)   ui.selectedIndex = (ui.selectedIndex - 1 + listSize) % listSize;
			}

			if ((kDown & KEY_A) && ui.selectedIndex >= 0 && ui.selectedIndex < listSize) {
				if (state == STATE_MAP_LIST) {
					int realIdx = filtered[ui.selectedIndex];
					if (ui.content.loadMap(ui.content.mapFiles[realIdx].path.c_str())) {
						currentMapIndex = realIdx;
						ui.activeMapIndex = realIdx;
						if (cameFromPlaying) { state = STATE_PLAYING; cameFromPlaying = false; }
						else { state = STATE_MAP_VIEW; }
					} else { ui.showNotification("Load Failed"); }
				} else if (state == STATE_WALK_LIST) {
					int realIdx = filtered[ui.selectedIndex];
					if (ui.content.loadWalk(ui.content.walkFiles[realIdx].path.c_str())) {
						ui.activeWalkIndex = realIdx;
						ui.scrollOffset = ui.content.loadWalkPosition();
						if (cameFromPlaying) { state = STATE_PLAYING; ui.gameplayMode = GAMEPLAY_GUIDE; cameFromPlaying = false; }
						else { state = STATE_WALK_VIEW; }
					} else { ui.showNotification("Load Failed"); }
				} else if (state == STATE_GAME_LIST) {
					pendingRomIndex = ui.selectedIndex;
					state = STATE_LOADING_GAME;
				}
			}

			if (kDown & KEY_B) {
				if (state == STATE_MAP_LIST && ui.folderGroups.size() > 1)
					{ state = STATE_MAP_FOLDER_LIST; ui.selectedIndex = 0; }
				else if (state == STATE_WALK_LIST && ui.folderGroups.size() > 1)
					{ state = STATE_WALK_FOLDER_LIST; ui.selectedIndex = 0; }
				else if (state == STATE_GAME_LIST)
					state = STATE_MENU;
				else {
					state = cameFromPlaying ? STATE_PLAYING : STATE_MENU;
					if (cameFromPlaying) cameFromPlaying = false;
				}
			}
		}

		// Handle simulated NES RAM switching debug controls (Select + D-pad Left/Right)
		if ((state == STATE_PLAYING || state == STATE_EMULATING) && !ui.content.mapFiles.empty()) {
			std::string prefix = getGamePrefix(ui.content.mapFiles[currentMapIndex].name);
			bool isZelda = (prefix.find("Zelda") != std::string::npos);
			bool isMario = (prefix.find("Mario") != std::string::npos);

			if (kHeld & KEY_SELECT) {
				if (isZelda) {
					if (kDown & KEY_LEFT) {
						if (nesRAM[0x0010] > 1) {
							nesRAM[0x0010]--;
							printf("\x1b[12;1H[DEBUG] Zelda RAM 0x0010 -> 0x%02X (Overworld)    ", nesRAM[0x0010]);
						}
					}
					if (kDown & KEY_RIGHT) {
						if (nesRAM[0x0010] < 3) {
							nesRAM[0x0010]++;
							printf("\x1b[12;1H[DEBUG] Zelda RAM 0x0010 -> 0x%02X (Dungeon %d)    ", nesRAM[0x0010], nesRAM[0x0010] - 1);
						}
					}
				} else if (isMario) {
					if (kDown & KEY_LEFT) {
						if (nesRAM[0x075C] > 0) {
							nesRAM[0x075C]--;
							printf("\x1b[12;1H[DEBUG] SMB RAM 0x075C -> World 1-%d            ", nesRAM[0x075C] + 1);
						}
					}
					if (kDown & KEY_RIGHT) {
						if (nesRAM[0x075C] < 3) {
							nesRAM[0x075C]++;
							printf("\x1b[12;1H[DEBUG] SMB RAM 0x075C -> World 1-%d            ", nesRAM[0x075C] + 1);
						}
					}
				}
			}

			// Automatically update map based on RAM value changes
			updateRAMMapSwitching(prefix, currentMapIndex, ui);

			// Detect when active gameplay has begun (past title/intro/file-select).
			// On Zelda 1, $EB (current screen index) is 0 on the title sequence and
			// becomes non-zero once Link is in the world. Latching: once active, stay
			// active for this play session so the map doesn't blink off during transitions.
			if (!ui.nesGameplayActive && nesReadRAM(0x00EB) != 0) {
				ui.nesGameplayActive = true;
			}
		}

		// Pending game start: progressive loading across multiple frames.
		// Each step does one small unit of work then yields to the main loop
		// so that C3D_FrameBegin / C3D_FrameEnd and hidScanInput stay alive.
		// This prevents the 10-second freeze from synchronous SD reads.
		static bool loadingScreenShown = false;
		if (state == STATE_LOADING_GAME && (pendingRomIndex != -2 || loadStep > 0) && loadingScreenShown) {
			loadingScreenShown = false;

			// ---- Step 0: load ROM, compute prefix, count matching maps ----
			if (loadStep == 0) {
				int chosenIdx = pendingRomIndex;
				pendingRomIndex = -2;

				std::string romPathToLoad;
				if (chosenIdx == -1) {
					FILE* bf = fopen("sdmc:/V2NES/bookmark_rom.txt", "rb");
					if (bf) {
						char lastRom[512] = {0};
						fgets(lastRom, sizeof(lastRom), bf);
						fclose(bf);
						size_t len = strlen(lastRom);
						while (len > 0 && (lastRom[len-1] == '\n' || lastRom[len-1] == '\r')) lastRom[--len] = 0;
						if (len > 0) {
							for (size_t i = 0; i < ui.content.romFiles.size(); i++) {
								if (ui.content.romFiles[i].path == lastRom) {
									chosenIdx = (int)i;
									romPathToLoad = lastRom;
									break;
								}
							}
						}
					}
					if (chosenIdx == -1) chosenIdx = 0;
				}
				if (romPathToLoad.empty() && chosenIdx >= 0 && chosenIdx < (int)ui.content.romFiles.size()) {
					romPathToLoad = ui.content.romFiles[chosenIdx].path;
				}

				bool ok = !romPathToLoad.empty() && nesLoadROM(romPathToLoad.c_str());
				if (!ok) {
					ui.showNotification("Failed to load ROM!");
					state = STATE_MENU;
				} else {
					ui.activeRomIndex = chosenIdx;
					saveRomBookmark(romPathToLoad.c_str());
					loadChosenIdx = chosenIdx;
					loadPrefix = getGamePrefix(ui.content.romFiles[chosenIdx].name);
					loadCursor = 0;

					// Preload the LARGEST matching map first so it gets priority
					// GPU memory. The overworld is almost always the biggest
					// file (multi-tile grid) and is what the user spends most
					// of their time on — if a dungeon loads first and eats
					// VRAM, the overworld's tail tiles fail C3D_TexInit and
					// render as black holes. The active map MUST fit; smaller
					// dungeons are tolerant of preload failure (loadMap will
					// retry by evicting the cache on demand).
					if (!loadPrefix.empty()) {
						size_t biggestIdx = (size_t)-1;
						long   biggestSize = -1;
						for (size_t m = 0; m < ui.content.mapFiles.size(); m++) {
							if (getGamePrefix(ui.content.mapFiles[m].name) != loadPrefix) continue;
							struct stat st;
							if (stat(ui.content.mapFiles[m].path.c_str(), &st) == 0) {
								if ((long)st.st_size > biggestSize) {
									biggestSize = (long)st.st_size;
									biggestIdx = m;
								}
							} else if (biggestIdx == (size_t)-1) {
								biggestIdx = m;
							}
						}
						if (biggestIdx != (size_t)-1) {
							ui.content.preloadMap(ui.content.mapFiles[biggestIdx].path);
							// Remember which map is the overworld so it is never evicted.
							ui.content.overworldMapPath = ui.content.mapFiles[biggestIdx].path;
						}
					}
					loadCursor = 0;

					// Count all prefix-matching maps — preload every map for this game
					// during STATE_LOADING_GAME so dungeon switches mid-gameplay are
					// instant cache hits. The prefix filter in loadNextMap ensures no
					// other game's maps are touched, so VRAM stays clean.
					loadTotal = 0;
					for (size_t m = 0; m < ui.content.mapFiles.size(); m++) {
						if (getGamePrefix(ui.content.mapFiles[m].name) == loadPrefix)
							loadTotal++;
					}
					loadStep = 1;
				}
			}

			// ---- Steps 1..N: preload one map per frame ----
			else if (loadStep >= 1 && loadTotal > 0 && loadStep <= loadTotal) {
				if (!loadPrefix.empty() && ui.content.loadNextMap(loadPrefix, loadCursor)) {
					loadStep++;
				} else {
					loadStep = 999;
				}
			}

			// ---- Finalize: pick starting map, guide, reset NDSP ----
			else {
				bool foundMap = false;
				if (!loadPrefix.empty()) {
					for (size_t m = 0; m < ui.content.mapFiles.size(); m++) {
						if (getGamePrefix(ui.content.mapFiles[m].name) == loadPrefix) {
							ui.content.loadMap(ui.content.mapFiles[m].path.c_str());
							currentMapIndex = (int)m;
							ui.activeMapIndex = (int)m;
							foundMap = true;
							break;
						}
					}
				}
				if (!foundMap && !ui.content.mapFiles.empty()) {
					ui.content.loadMap(ui.content.mapFiles[0].path.c_str());
					currentMapIndex = 0;
					ui.activeMapIndex = 0;
				} else if (ui.content.mapFiles.empty()) {
					ui.content.freeMap();
					ui.activeMapIndex = -1;
				}

				bool foundGuide = false;
				if (!loadPrefix.empty()) {
					// 1. Honour the last-opened walk bookmark, but only when it
					//    actually belongs to the game we're now launching.
					std::string bookmarked = ui.content.loadBookmark();
					if (!bookmarked.empty()) {
						for (size_t g = 0; g < ui.content.walkFiles.size(); g++) {
							if (ui.content.walkFiles[g].path == bookmarked &&
								getGamePrefix(ui.content.walkFiles[g].name) == loadPrefix) {
								ui.content.loadWalk(ui.content.walkFiles[g].path.c_str());
								ui.scrollOffset = ui.content.loadWalkPosition();
								ui.activeWalkIndex = (int)g;
								foundGuide = true;
								break;
							}
						}
					}
					// 2. No usable bookmark — fall back to the first walk whose
					//    filename matches the game prefix.
					if (!foundGuide) {
						for (size_t g = 0; g < ui.content.walkFiles.size(); g++) {
							if (getGamePrefix(ui.content.walkFiles[g].name) == loadPrefix) {
								ui.content.loadWalk(ui.content.walkFiles[g].path.c_str());
								ui.scrollOffset = ui.content.loadWalkPosition();
								ui.activeWalkIndex = (int)g;
								foundGuide = true;
								break;
							}
						}
					}
				}
				if (!foundGuide && !ui.content.walkFiles.empty()) {
					ui.content.loadWalk(ui.content.walkFiles[0].path.c_str());
					ui.scrollOffset = ui.content.loadWalkPosition();
					ui.activeWalkIndex = 0;
				}

				nesRomLoaded = true;

				if (audioOk) {
					ndspChnReset(10);
					ndspChnSetFormat(10, NDSP_FORMAT_MONO_PCM16);
					ndspChnSetRate(10, (float)AUDIO_RATE);
					ndspChnSetInterp(10, NDSP_INTERP_NONE);
					float mix[12] = {0}; mix[0] = 1.0f; mix[1] = 1.0f;
					ndspChnSetMix(10, mix);
					for (int i = 0; i < AUDIO_BUF_COUNT; i++) {
						waveBufs[i].status = NDSP_WBUF_DONE;
						waveBufs[i].nsamples = 0;
					}
					// Pre-fill the NDSP queue with 3 silent buffers (~50ms cushion).
					// Without this the queue is always ≤1 buffer deep and a single
					// frame arriving 1ms late causes an audible crack.
					for (int i = 0; i < 3; i++) {
						memset(audioBufs[i], 0, AUDIO_SAMPLES * sizeof(short));
						waveBufs[i].nsamples = AUDIO_SAMPLES;
						DSP_FlushDataCache(audioBufs[i], AUDIO_SAMPLES * sizeof(short));
						ndspChnWaveBufAdd(10, &waveBufs[i]);
					}
					audioBufIdx = 3;
				}

				loadStep = 0;
				loadTotal = 0;
				loadCursor = 0;
				loadPrefix.clear();

				lastEmuTime = 0;  // reset frame pacer for new game
				state = STATE_EMULATING;
			}
		}

		// NES Emulation Loop — timing-based catch-up so the NES always runs
		// at real-time speed even when individual loop iterations exceed 16ms.
		if (state == STATE_EMULATING) {
			u32 nesButtons = 0;
			u32 held = hidKeysHeld();
			if (held & KEY_A)      nesButtons |= NES_BTN_A;
			if (held & KEY_B)      nesButtons |= NES_BTN_B;
			// KEY_X and KEY_Y are reserved as hotkeys only (save/load state, map zoom)
			// and must NOT pass through to the NES controller.
			if (held & KEY_UP)     nesButtons |= NES_BTN_UP;
			if (held & KEY_DOWN)   nesButtons |= NES_BTN_DOWN;
			if (held & KEY_LEFT)   nesButtons |= NES_BTN_LEFT;
			if (held & KEY_RIGHT)  nesButtons |= NES_BTN_RIGHT;
			if (held & KEY_START)  nesButtons |= NES_BTN_START;
			if (held & KEY_SELECT) nesButtons |= NES_BTN_SELECT;
			nesSetInput(nesButtons);

			u64 now = osGetTime();
			// First frame or returning from a pause/menu — don't catch up
			if (lastEmuTime == 0 || now - lastEmuTime > 100) {
				lastEmuTime = now;
			}
			int framesDue = (int)((now - lastEmuTime) / 16);
			if (framesDue < 1) framesDue = 1;
			if (framesDue > 2) framesDue = 2;   // cap to avoid death spiral
			lastEmuTime += (u64)framesDue * 16;

			static double audioAccum = 0.0;
			for (int f = 0; f < framesDue; f++) {
				nesEmulateFrame();
				// Fractional accumulator: 22050/60 = 367.5 samples per frame.
				// Alternate between 367 and 368 to hit exactly 22050 samples/sec
				// and eliminate the 30-sample/sec drift that caused periodic cracks.
				if (audioOk) {
					audioAccum += (double)AUDIO_RATE / 60.0;
					int samplesToGen = (int)audioAccum;
					audioAccum -= samplesToGen;
					// Scan for a free buffer slot — don't drop audio just because
					// the current index happens to be queued.
					for (int b = 0; b < AUDIO_BUF_COUNT; b++) {
						int idx = (audioBufIdx + b) % AUDIO_BUF_COUNT;
						ndspWaveBuf* wb = &waveBufs[idx];
						if (wb->status == NDSP_WBUF_FREE || wb->status == NDSP_WBUF_DONE) {
							int count;
							nesGetAudio(audioBufs[idx], samplesToGen, &count);
							if (count > 0 && count <= AUDIO_SAMPLES) {
								wb->nsamples = count;
								DSP_FlushDataCache(audioBufs[idx], count * sizeof(short));
								ndspChnWaveBufAdd(10, wb);
								audioBufIdx = (idx + 1) % AUDIO_BUF_COUNT;
							}
							break;
						}
					}
				}
			}
		}

		// Global Back Logic (B Button) — skip while NES is running.
		// Note: STATE_MAP/WALK_FOLDER_LIST and STATE_MAP/WALK_LIST handle B
		// themselves inside the d-pad block above; only non-list states are here.
		if ((kDown & KEY_B) && state != STATE_EMULATING) {
			if (state == STATE_SETTINGS_SENSITIVITY) state = STATE_SETTINGS;
			else if (state == STATE_SETTINGS) state = STATE_MENU;
			else if (state == STATE_GAME_LIST) state = STATE_MENU;
			else if (state == STATE_MAP_VIEW) state = STATE_MAP_LIST;
			else if (state == STATE_WALK_VIEW) { ui.content.saveWalkPosition(ui.scrollOffset); state = STATE_WALK_LIST; }
			else if (state == STATE_PLAYING) {
				// B on the bottom screen while a game is running returns to the top
				// screen — it does NOT exit the game. Use the home menu to quit.
				state = STATE_EMULATING;
			}
			// Only reset map view when we're not returning to the emulator
			if (state != STATE_EMULATING) {
				ui.content.resetView(214.0f);
			}
		}

		// Update Battery every 5 seconds
		u64 now = osGetTime();
		if (now - lastBatteryCheck > 5000) {
			u8 batteryPercent = 0;
			if (R_SUCCEEDED(PTMU_GetBatteryLevel(&batteryPercent))) {
				int percent = (int)batteryPercent * 20;
				if (percent > 100) percent = 100;
				if (percent < 0) percent = 0;
				ui.updateBattery(percent);
			}
			lastBatteryCheck = now;
		}

		// Render — SYNCDRAW so vsync is handled by FrameEnd.
		if (C3D_FrameBegin(C3D_FRAME_SYNCDRAW)) {
			// 1. Top screen: NES via C2D texture
			if (topTarget) {
				C2D_SceneBegin(topTarget);
				C2D_TargetClear(topTarget, C2D_Color32(0, 0, 0, 255));
				// Show logo on top screen during startup, menu, and loading.
				// Drawn inside the 320px content area (x=40..360); 40px side strips reserved for bezels.
				// SubTex is cropped to 469×255 content; scale=320/469 → 320×174px, y=(240-174)/2=33.
				bool showLogo = (state == STATE_STARTUP || state == STATE_MENU || state == STATE_LOADING_GAME);
				if (showLogo && g_logoReady) {
					// Scale logo to fill almost the full top-screen height.
					// SubTex is 468×227 (trimmed artwork). Scale to fit height:
					//   scale = 230/227 ≈ 1.013 → 474×230, centred in 400×240.
					float scale = 230.0f / 227.0f;
					float drawW = 463.0f * scale;
					float drawH = 227.0f * scale;
					float x = (400.0f - drawW) / 2.0f;
					float y = (240.0f - drawH) / 2.0f;
					C2D_DrawImageAt(g_logoImage, x, y, 0.7f, NULL, scale, scale);
				}
				// Loading status text below the logo
				// Loading progress bar moved to the bottom screen (rendered in ui.renderBottom).

				if (nesRomLoaded && nesImage.tex && state != STATE_LOADING_GAME) {
					// Only re-upload when the emulator produced a new frame.
					// Skips the morton swizzle + DMA during STATE_PLAYING (frozen
					// top screen while browsing maps/guides on the bottom screen).
					if (nesFrameChanged()) {
						u16* src = nesGetVisibleFrameBuffer();
						if (src) {
							C3D_TexUpload(&nesTex, src);
							C3D_TexFlush(&nesTex);
						}
					}
					C2D_DrawImageAt(nesImage, 37, 0, 0.5f, NULL, 320.0f / 256.0f, 1.0f);
				}
				// Glow border overlay — only shown outside gameplay
				if (g_glowTopReady && state != STATE_EMULATING) {
					C2D_DrawImageAt(g_glowTopImage, 40, 0, 0.8f, NULL, 1.0f, 1.0f);
				}
				// Top screen notification overlay
				if (topNotifyTimer > 0) {
					C2D_TextBufClear(topNotifyBuf);
					C2D_Text txt;
					C2D_TextParse(&txt, topNotifyBuf, topNotifyMsg);
					C2D_TextOptimize(&txt);
					// Semi-transparent black bar behind text (centered vertically)
					C2D_DrawRectSolid(40, 109, 0.9f, 320, 22, C2D_Color32(0, 0, 0, 160));
					C2D_DrawText(&txt, C2D_WithColor | C2D_AlignCenter, 200, 111, 0.95f, 0.6f, 0.6f, C2D_Color32(255, 255, 255, 255));
					topNotifyTimer--;
				}
			}
			// 2. Bottom screen: V2NES UI
			// Pass loading progress so the bottom screen can draw the bar.
			ui.hudLoadStep  = loadStep;
			ui.hudLoadTotal = loadTotal;
			// Player position marker: read the RAM byte specified by the .pos
			// sidecar file. Only active while a game is running and calibration
			// is loaded for the current map.
			if ((state == STATE_PLAYING || state == STATE_EMULATING) &&
			    ui.content.posCalib.valid) {
			    ui.playerRamVal = (int)nesReadRAM(ui.content.posCalib.ramAddr);
			} else {
			    ui.playerRamVal = -1;
			}
			ui.renderBottom(state, settings);
			// Bottom screen glow border — shown outside gameplay, OR briefly after a save/load.
			// z=0.9 when used as save flash (on top of everything); z=0.4 normally (behind HUD).
			if (g_glowBotReady) {
				if (ui.saveLoadFlashTimer > 0) {
					C2D_DrawImageAt(g_glowBotImage, 0, 0, 0.9f, NULL, 1.0f, 1.0f);
				} else if (state != STATE_EMULATING) {
					C2D_DrawImageAt(g_glowBotImage, 0, 0, 0.4f, NULL, 1.0f, 1.0f);
				}
			}
			C3D_FrameEnd(0);

			// If we just rendered the loading screen, mark it as shown so the
			// next iteration's check can fire the heavy load.
			if (state == STATE_LOADING_GAME) {
				loadingScreenShown = true;
			}
		}

		// Cap loop at 60 Hz. SYNCDRAW already provides vsync pacing via FrameEnd;
		// this sleep only fires on frames where no NES game is running (menus, etc.)
		// where FrameEnd returns immediately and the loop would otherwise run uncapped.
		u64 loopElapsed = osGetTime() - loopStartMs;
		if (loopElapsed < 16) {
			svcSleepThread((s64)(16 - loopElapsed) * 1000000LL);
		}
	}

	nesStopThread();    // no-op when thread was never started; safe to call always
	ptmuExit();
	cfguExit();
	if (audioOk) { ndspExit(); dspExit(); }
	gfxExit();
	return 0;
}
