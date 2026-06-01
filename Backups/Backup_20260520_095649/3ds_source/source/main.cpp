#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <citro2d.h>
#include <citro3d.h>
#include "ui.h"
#include "settings.h"
#include "nes_core.h"

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
    
    // Determine which RAM byte to monitor based on the running game prefix
    bool isZelda = (prefix.find("Zelda") != std::string::npos);
    bool isMario = (prefix.find("Mario") != std::string::npos);
    
    int currentRAMVal = 0;
    if (isZelda) {
        currentRAMVal = nesRAM[0x0010];
    } else if (isMario) {
        currentRAMVal = nesRAM[0x075C]; // SMB level byte (0=1-1, 1=1-2, etc.)
    } else {
        return; // RAM switching not set up for this game prefix
    }
    
    if (currentRAMVal == lastRAMVal) {
        return; // RAM value hasn't changed
    }
    
    lastRAMVal = currentRAMVal;
    
    // Search for a matching map file in our list
    int foundIndex = -1;
    
    if (isZelda) {
        if (currentRAMVal <= 1) {
            // Overworld: Search for exactly the clean prefix map (no suffixes)
            for (size_t i = 0; i < ui.content.mapFiles.size(); i++) {
                std::string name = ui.content.mapFiles[i].name;
                if (name == prefix + ".v2m" || name == prefix) {
                    foundIndex = (int)i;
                    break;
                }
            }
        } else {
            // Dungeon level: Search for "-LevelX" or "-DungeonX" or "-X"
            int levelNum = currentRAMVal - 1; // 2 -> Level 1, 3 -> Level 2
            char sub1[32], sub2[32], sub3[32];
            snprintf(sub1, sizeof(sub1), "Level%d", levelNum);
            snprintf(sub2, sizeof(sub2), "Dungeon%d", levelNum);
            snprintf(sub3, sizeof(sub3), "-%d", levelNum);
            
            for (size_t i = 0; i < ui.content.mapFiles.size(); i++) {
                std::string name = ui.content.mapFiles[i].name;
                if (name.find(sub1) != std::string::npos || 
                    name.find(sub2) != std::string::npos ||
                    name.find(sub3) != std::string::npos) {
                    foundIndex = (int)i;
                    break;
                }
            }
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
			if (!ui.content.loadMapView()) {
				ui.content.resetView(206.0f);
			}
			ui.activeMapIndex = currentMapIndex;
            char notifyMsg[64];
            snprintf(notifyMsg, sizeof(notifyMsg), "Auto-Map: %s", ui.content.mapFiles[currentMapIndex].name.c_str());
            ui.showNotification(notifyMsg);
            
            // Log to top debug console
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

	V2UI ui;
	ui.init();

	V2Settings settings;
	settings.load();

	// NES core initialization
	C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	bool nesReady = false;
	C3D_Tex nesTex;
	C2D_Image nesImage = {0};
	// SubTexture UV coords: map the 256x240 visible area from the 512x256 GPU texture
	// 3DS GPU V-axis is flipped: V=1.0 = top of texture, V=0.0 = bottom
	// U: left=0.0, right=256/512=0.5    V: top=1.0, bottom=1.0-(240/256)=0.0625
	Tex3DS_SubTexture nesSubTex = { 256, 240, 0.0f, 1.0f, 0.5f, 1.0f - (240.0f/256.0f) };
	C3D_TexInit(&nesTex, 512, 256, GPU_RGB565);
	C3D_TexSetFilter(&nesTex, GPU_NEAREST, GPU_NEAREST);
	nesInit();
	nesReady = true;
	nesImage.tex = &nesTex;
	nesImage.subtex = &nesSubTex;

	GameState state = STATE_MENU;
	int currentMapIndex = 0;
	bool cameFromPlaying = false;
	nesRAM[0x0010] = 1; // Zelda starts in Overworld

	// Console disabled for now — top screen uses C2D for NES rendering, bottom uses V2NES UI

	u64 lastBatteryCheck = 0;

	// Main loop
	while (aptMainLoop())
	{
		// Scan all the inputs
		hidScanInput();

		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();

		if (kDown & KEY_START)
			break;

		// Quick Save / Load triggers
		if (kDown & KEY_L) {
			ui.showNotification("SAVED");
			ui.saveLoadFlashTimer = 5;
		}
		if (kDown & KEY_R) {
			ui.showNotification("LOADED");
			ui.saveLoadFlashTimer = 5;
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
				ui.checkTouch(touch, ui.layoutCatBtn);
				ui.checkTouch(touch, ui.themeCatBtn);
				ui.checkTouch(touch, ui.speedCatBtn);
				ui.checkTouch(touch, ui.backBtn);
			} else if (state == STATE_SETTINGS_LAYOUT) {
				ui.checkTouch(touch, ui.layout1Btn);
				ui.checkTouch(touch, ui.layout2Btn);
				ui.checkTouch(touch, ui.layout3Btn);
				ui.checkTouch(touch, ui.backBtn);
			} else if (state == STATE_SETTINGS_THEME) {
				ui.checkTouch(touch, ui.themeDarkBtn);
				ui.checkTouch(touch, ui.themeLightBtn);
				ui.checkTouch(touch, ui.accentCycleBtn);
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
				ui.checkTouch(touch, ui.hudBackBtn);
			}
		} else {
			ui.playBtn.pressed = ui.mapsBtn.pressed = ui.walkBtn.pressed = false;
			ui.settingsBtn.pressed = ui.listBtn.pressed = false;
			ui.layoutCatBtn.pressed = ui.themeCatBtn.pressed = ui.speedCatBtn.pressed = false;
			ui.layout1Btn.pressed = ui.layout2Btn.pressed = ui.layout3Btn.pressed = false;
			ui.themeDarkBtn.pressed = ui.themeLightBtn.pressed = ui.accentCycleBtn.pressed = false;
			ui.zoomSpeedBtn.pressed = ui.panSpeedBtn.pressed = ui.scrollSpeedBtn.pressed = false;
			ui.backBtn.pressed = false;
			ui.hudPrevBtn.pressed = ui.hudNextBtn.pressed = false;
			ui.hudShowMapBtn.pressed = ui.hudShowGuideBtn.pressed = false;
			ui.hudBackBtn.pressed = false;
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
					ui.showNotification("Starting NES Core...");
					ui.content.scanFiles();
					
					// Find first ROM and start emulation
					bool romLoaded = false;
					for (size_t i = 0; i < ui.content.romFiles.size(); i++) {
						if (nesLoadROM(ui.content.romFiles[i].path.c_str())) {
							ui.activeRomIndex = (int)i;
							romLoaded = true;
							break;
						}
					}
					if (!romLoaded) {
						ui.showNotification("No ROMs Found! Copy to /V2NES/roms/");
						state = STATE_MENU;
						break;
					}

					// 1. Find and load the default map
					if (!ui.content.mapFiles.empty()) {
						currentMapIndex = 0;
						ui.content.loadMap(ui.content.mapFiles[currentMapIndex].path.c_str());
						ui.content.resetView(206.0f);
						ui.activeMapIndex = 0;
					} else {
						ui.content.freeMap();
						ui.activeMapIndex = -1;
					}

					// 2. Automatically find and load the corresponding guide file matching this game prefix
					if (!ui.content.mapFiles.empty()) {
						std::string prefix = getGamePrefix(ui.content.mapFiles[currentMapIndex].name);
						bool foundGuide = false;
						for (size_t i = 0; i < ui.content.walkFiles.size(); i++) {
							if (getGamePrefix(ui.content.walkFiles[i].name) == prefix) {
								ui.content.loadWalk(ui.content.walkFiles[i].path.c_str());
								ui.scrollOffset = ui.content.loadWalkPosition();
								ui.activeWalkIndex = (int)i;
								foundGuide = true;
								break;
							}
						}
						if (!foundGuide && !ui.content.walkFiles.empty()) {
							ui.content.loadWalk(ui.content.walkFiles[0].path.c_str());
							ui.activeWalkIndex = 0;
						}
					}
					
					state = STATE_EMULATING;
				}
				else if (ui.checkTouch(touch, ui.mapsBtn)) {
					ui.content.scanFiles();
					state = STATE_MAP_LIST;
					ui.selectedIndex = -1;
				}
				else if (ui.checkTouch(touch, ui.walkBtn)) {
					ui.content.scanFiles();
					std::string bookmarkPath = ui.content.loadBookmark();
					bool foundBookmark = false;
					if (!bookmarkPath.empty()) {
						for (size_t i = 0; i < ui.content.walkFiles.size(); i++) {
							if (ui.content.walkFiles[i].path == bookmarkPath) {
								if (ui.content.loadWalk(bookmarkPath.c_str())) {
									ui.activeWalkIndex = (int)i;
									ui.scrollOffset = ui.content.loadWalkPosition();
									state = STATE_WALK_VIEW;
									foundBookmark = true;
								}
								break;
							}
						}
					}
					if (!foundBookmark) {
						state = STATE_WALK_LIST;
						ui.selectedIndex = -1;
					}
				}
				else if (ui.checkTouch(touch, ui.settingsBtn)) {
					state = STATE_SETTINGS;
				}
				else if (ui.checkTouch(touch, ui.listBtn)) {
					ui.content.scanFiles();
					state = STATE_GAME_LIST;
					ui.selectedIndex = -1;
				}
			} else if (state == STATE_SETTINGS) {
				if (ui.checkTouch(touch, ui.layoutCatBtn)) {
					state = STATE_SETTINGS_LAYOUT;
				}
				else if (ui.checkTouch(touch, ui.themeCatBtn)) {
					state = STATE_SETTINGS_THEME;
				}
				else if (ui.checkTouch(touch, ui.speedCatBtn)) {
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
			} else if (state == STATE_SETTINGS_LAYOUT) {
				if (ui.checkTouch(touch, ui.layout1Btn)) {
					settings.setLayout(LAYOUT_CLASSIC);
					settings.save();
					ui.showNotification("Classic Saved");
					printf("\x1b[3;1HLayout: Classic            ");
				}
				else if (ui.checkTouch(touch, ui.layout2Btn)) {
					settings.setLayout(LAYOUT_MODERN);
					settings.save();
					ui.showNotification("Modern Saved");
					printf("\x1b[3;1HLayout: Modern             ");
				}
				else if (ui.checkTouch(touch, ui.layout3Btn)) {
					settings.setLayout(LAYOUT_ONE_HANDED);
					settings.save();
					ui.showNotification("One-Handed Saved");
					printf("\x1b[3;1HLayout: One-Handed         ");
				}
				else if (ui.checkTouch(touch, ui.backBtn)) {
					state = STATE_SETTINGS;
				}
			} else if (state == STATE_SETTINGS_THEME) {
				if (ui.checkTouch(touch, ui.themeDarkBtn)) {
					settings.setTheme(THEME_DARK);
					settings.save();
					ui.showNotification("Dark Theme Saved");
				}
				else if (ui.checkTouch(touch, ui.themeLightBtn)) {
					ui.showNotification("Dark Theme Only");
				}
				else if (ui.checkTouch(touch, ui.accentCycleBtn)) {
					int currentAccent = (int)settings.getAccent();
					currentAccent = (currentAccent + 1) % ACCENT_COUNT;
					settings.setAccent((AccentColor)currentAccent);
					settings.save();
					
					char msg[64];
					snprintf(msg, sizeof(msg), "Accent: %s", settings.getAccentName((AccentColor)currentAccent));
					ui.showNotification(msg);
				}
				else if (ui.checkTouch(touch, ui.backBtn)) {
					state = STATE_SETTINGS;
				}
			} else if (state == STATE_MAP_LIST) {
				if (ui.checkTouch(touch, ui.backBtn)) {
					if (cameFromPlaying) {
						state = STATE_PLAYING;
						cameFromPlaying = false;
					} else {
						state = STATE_MENU;
					}
				} else {
					for (size_t i = 0; i < ui.content.mapFiles.size() && i < 5; i++) {
						float itemY = 50 + i * 25;
						if (touch.py > itemY && touch.py < itemY + 25) {
							ui.selectedIndex = i;
							if (ui.content.loadMap(ui.content.mapFiles[i].path.c_str())) {
								currentMapIndex = i;
								ui.activeMapIndex = i;
								bool loadedView = ui.content.loadMapView();
								if (cameFromPlaying) {
									if (!loadedView) ui.content.resetView(206.0f);
									state = STATE_PLAYING;
									cameFromPlaying = false;
								} else {
									if (!loadedView) ui.content.resetView(240.0f);
									state = STATE_MAP_VIEW;
								}
							} else {
								ui.showNotification("Load Failed");
							}
						}
					}
				}
			} else if (state == STATE_WALK_LIST) {
				if (ui.checkTouch(touch, ui.backBtn)) {
					if (cameFromPlaying) {
						state = STATE_PLAYING;
						cameFromPlaying = false;
					} else {
						state = STATE_MENU;
					}
				} else {
					for (size_t i = 0; i < ui.content.walkFiles.size() && i < 5; i++) {
						float itemY = 50 + i * 25;
						if (touch.py > itemY && touch.py < itemY + 25) {
							ui.selectedIndex = i;
							if (ui.content.loadWalk(ui.content.walkFiles[i].path.c_str())) {
								ui.activeWalkIndex = i;
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
							ui.activeRomIndex = i;
							
							std::string romName = ui.content.romFiles[i].name;
							std::string romPrefix = getGamePrefix(romName);
							
							char msg[128];
							snprintf(msg, sizeof(msg), "Booting %s...", romPrefix.c_str());
							ui.showNotification(msg);
							
							// Auto-load matching map
							bool foundMap = false;
							for (size_t m = 0; m < ui.content.mapFiles.size(); m++) {
								if (getGamePrefix(ui.content.mapFiles[m].name) == romPrefix) {
									ui.content.loadMap(ui.content.mapFiles[m].path.c_str());
									if (!ui.content.loadMapView()) {
										ui.content.resetView(206.0f);
									}
									currentMapIndex = m;
									ui.activeMapIndex = m;
									foundMap = true;
									break;
								}
							}
							if (!foundMap && !ui.content.mapFiles.empty()) {
								ui.content.loadMap(ui.content.mapFiles[0].path.c_str());
								if (!ui.content.loadMapView()) {
									ui.content.resetView(206.0f);
								}
								currentMapIndex = 0;
								ui.activeMapIndex = 0;
							}
							
							// Auto-load matching guide
							bool foundGuide = false;
							for (size_t g = 0; g < ui.content.walkFiles.size(); g++) {
								if (getGamePrefix(ui.content.walkFiles[g].name) == romPrefix) {
									ui.content.loadWalk(ui.content.walkFiles[g].path.c_str());
									ui.activeWalkIndex = g;
									foundGuide = true;
									break;
								}
							}
							if (!foundGuide && !ui.content.walkFiles.empty()) {
								ui.content.loadWalk(ui.content.walkFiles[0].path.c_str());
								ui.activeWalkIndex = 0;
							}
						
							// Load ROM into NES emulator
							if (!nesLoadROM(ui.content.romFiles[i].path.c_str())) {
								ui.showNotification("Failed to load ROM!");
								break;
							}
							
							state = STATE_EMULATING;
							break;
						}
					}
				}
			} else if (state == STATE_PLAYING || state == STATE_EMULATING) {
				// Handle Touch HUD Button Interactions
				if (ui.checkTouch(touch, ui.hudShowMapBtn)) {
					if (ui.gameplayMode == GAMEPLAY_MAP) {
						ui.content.scanFiles();
						state = STATE_MAP_LIST;
						cameFromPlaying = true;
						ui.selectedIndex = currentMapIndex;
					} else {
						ui.gameplayMode = GAMEPLAY_MAP;
						ui.showNotification("Mode: Map");
					}
				}
				else if (ui.checkTouch(touch, ui.hudShowGuideBtn)) {
					if (ui.gameplayMode == GAMEPLAY_GUIDE) {
						ui.content.scanFiles();
						state = STATE_WALK_LIST;
						cameFromPlaying = true;
						ui.selectedIndex = -1;
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
							if (!ui.content.loadMapView()) {
								ui.content.resetView(206.0f);
							}
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
							if (!ui.content.loadMapView()) {
								ui.content.resetView(206.0f);
							}
							char msg[64];
							snprintf(msg, sizeof(msg), "Level Map: %s", ui.content.mapFiles[currentMapIndex].name.c_str());
							ui.showNotification(msg);
						} else {
							ui.showNotification("Load Failed");
						}
					}
				}
				else if (ui.checkTouch(touch, ui.hudBackBtn)) {
					ui.content.freeMap();
					state = STATE_MENU;
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
		}

		// Handle Touch stylus drag-to-pan in Map Mode
		static bool wasTouching = false;
		static u16 prevTouchX = 0;
		static u16 prevTouchY = 0;
		if (kDown & KEY_TOUCH) wasTouching = false;
		if (state == STATE_MAP_VIEW || (state == STATE_PLAYING && ui.gameplayMode == GAMEPLAY_MAP)) {
			float viewportH = (state == STATE_PLAYING) ? 206.0f : 240.0f;
			if (kHeld & KEY_TOUCH) {
				touchPosition touch;
				hidTouchRead(&touch);
				// Drag/pan if touching above the HUD overlay bar (y < 206)
				if (state == STATE_MAP_VIEW || touch.py < 206) {
					if (wasTouching) {
						float dx = (float)touch.px - (float)prevTouchX;
						float dy = (float)touch.py - (float)prevTouchY;
						if (dx != 0.0f || dy != 0.0f) {
							ui.content.pan(dx, dy, viewportH);
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
			float viewportH = (state == STATE_PLAYING) ? 206.0f : 240.0f;
			float zoomFactor = (settings.getZoomSpeed() == 1) ? 1.10f : ((settings.getZoomSpeed() == 3) ? 1.40f : 1.25f);
			int panStep = (settings.getPanSpeed() == 1) ? 3 : ((settings.getPanSpeed() == 3) ? 9 : 6);

			if (kDown & KEY_Y) ui.content.zoomIn(zoomFactor, viewportH);
			if (kDown & KEY_X) ui.content.zoomOut(zoomFactor, viewportH);
			if (kHeld & KEY_UP) ui.content.pan(0, -panStep, viewportH);
			if (kHeld & KEY_DOWN) ui.content.pan(0, panStep, viewportH);
			if (kHeld & KEY_LEFT) ui.content.pan(-panStep, 0, viewportH);
			if (kHeld & KEY_RIGHT) ui.content.pan(panStep, 0, viewportH);
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

		// Handle simulated NES RAM switching debug controls (Select + D-pad Left/Right)
		if (state == STATE_PLAYING && !ui.content.mapFiles.empty()) {
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
		}

		// NES Emulation Loop (when actively emulating)
		if (state == STATE_EMULATING) {
			// Map 3DS buttons to NES controller
			static u32 prevNESButtons = 0;
			u32 nesButtons = 0;
			u32 held = hidKeysHeld();
			if (held & KEY_A)      nesButtons |= NES_BTN_A;
			if (held & KEY_B)      nesButtons |= NES_BTN_B;
			if (held & KEY_X)      nesButtons |= NES_BTN_A;
			if (held & KEY_Y)      nesButtons |= NES_BTN_B;
			if (held & KEY_UP)     nesButtons |= NES_BTN_UP;
			if (held & KEY_DOWN)   nesButtons |= NES_BTN_DOWN;
			if (held & KEY_LEFT)   nesButtons |= NES_BTN_LEFT;
			if (held & KEY_RIGHT)  nesButtons |= NES_BTN_RIGHT;
			if (held & KEY_START)  nesButtons |= NES_BTN_START;
			if (held & KEY_SELECT) nesButtons |= NES_BTN_SELECT;

			// Only send input on change and then emulate frame
			nesSetInput(nesButtons);
			nesEmulateFrame();
		}

		// Global Back Logic (B Button)
		if (kDown & KEY_B) {
			if (state == STATE_SETTINGS_LAYOUT || state == STATE_SETTINGS_THEME || state == STATE_SETTINGS_SENSITIVITY) state = STATE_SETTINGS;
			else if (state == STATE_SETTINGS) state = STATE_MENU;
			else if (state == STATE_MAP_LIST || state == STATE_WALK_LIST || state == STATE_GAME_LIST) {
				if (cameFromPlaying) {
					state = STATE_PLAYING;
					cameFromPlaying = false;
				} else {
					state = STATE_MENU;
				}
			}
			else if (state == STATE_MAP_VIEW) { ui.content.saveMapView(); state = STATE_MAP_LIST; }
			else if (state == STATE_WALK_VIEW) { ui.content.saveWalkPosition(ui.scrollOffset); state = STATE_WALK_LIST; }
			else if (state == STATE_PLAYING) {
				ui.content.freeMap();
				state = STATE_MENU;
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

		// Render
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		// Top screen: NES framebuffer or dark background
		if (topTarget) {
			C2D_SceneBegin(topTarget);
			C2D_TargetClear(topTarget, C2D_Color32(0, 0, 0, 255));
			if (state == STATE_EMULATING && nesReady && nesImage.tex) {
				u16* fb = nesGetFrameBuffer();
				if (fb) {
					// Flush the CPU data cache so the GPU can see the PPU's linear framebuffer
					GSPGPU_FlushDataCache(fb, 512 * 256 * sizeof(u16));
					// Use the hardware display transfer to convert from linear to GPU-tiled format
					// Input:  512x256 linear RGB565 (PPU framebuffer)
					// Output: 512x256 tiled RGB565 (GPU texture)
					C3D_SyncDisplayTransfer(
						(u32*)fb,                GX_BUFFER_DIM(512, 256),
						(u32*)nesTex.data,       GX_BUFFER_DIM(512, 256),
						(GX_TRANSFER_FLIP_VERT(0) |
						 GX_TRANSFER_OUT_TILED(1) |
						 GX_TRANSFER_RAW_COPY(0) |
						 GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) |
						 GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) |
						 GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
					);
					// NES framebuffer is 512x256 (512px stride per scanline)
					// Visible area is left 256x240 pixels, scaled to fill 400x240 top screen
					C2D_DrawImageAt(nesImage, 0, 0, 0.5f, NULL, 400.0f / 256.0f, 1.0f);
				}
			}
		}
		ui.renderBottom(state, settings);
		C3D_FrameEnd(0);
	}

	ptmuExit();
	cfguExit();
	gfxExit();
	return 0;
}

