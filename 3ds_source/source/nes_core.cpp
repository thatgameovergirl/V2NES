#include "nes_core.h"
#include <string.h>
#include <malloc.h>

// NES core classes (include order matters for PPU.h dependencies)
#include "MMU.h"
#include "PPU.h"
#include "APU.h"
#include "PAD.h"
#include "NES.h"
#include "palette.h"
#include "Config.h"

static NES*    nes     = nullptr;
static u16*    frameBuf = nullptr;
static u16*    visibleBuf = nullptr;
static u8      linecolor[256] = {0};
static short   audioBuf[1024] = {0};
static char    currentRomPath[512] = {0};
static bool    visibleBufDirty = true;
volatile bool  nesFrameSucceeded = false;
volatile int   nesFrameCount = 0;

// Background emulation thread state
static LightEvent    g_emuKick;             // main → emu: "run one frame"
static LightEvent    g_emuDone;             // emu → main: "frame complete"
static Thread        g_emuThread = NULL;
static volatile bool g_emuQuit   = false;

bool nesInit() {
    Config.sound.nRate = 22050;
    Config.sound.nBits = 16;
    if (frameBuf == nullptr) {
        frameBuf = (u16*)linearAlloc(512 * 256 * 4);
        if (!frameBuf) return false;
        memset(frameBuf, 0, 512 * 256 * 4);
    }
    if (visibleBuf == nullptr) {
        visibleBuf = (u16*)linearAlloc(256 * 256 * 2);
        if (!visibleBuf) return false;
    }
    nespalInitialize();
    return true;
}

bool nesLoadROM(const char* path) {
    if (nes) {
        delete nes;
        nes = nullptr;
    }
    nes = new NES(path);
    if (nes->error) {
        delete nes;
        nes = nullptr;
        currentRomPath[0] = 0;
        return false;
    }
    strncpy(currentRomPath, path, sizeof(currentRomPath) - 1);
    currentRomPath[sizeof(currentRomPath) - 1] = 0;
    nes->ppu->SetScreenRGBAPtr(frameBuf, linecolor);
    nes->Reset();
    return true;
}

void nesEmulateFrame() {
    if (!nes) { return; }
    nes->ppu->SetScreenRGBAPtr(frameBuf, linecolor);
    nes->EmulateFrame(true);
    visibleBufDirty = true;
    nesFrameSucceeded = true;
    nesFrameCount++;
}

bool nesFrameChanged() {
    return visibleBufDirty;
}

u8 nesReadRAM(u16 addr) {
    // VirtuaNES exposes the NES internal 2KB RAM as the global RAM[] array.
    // Mirror addresses 0x0800-0x1FFF wrap back to 0x0000-0x07FF.
    return RAM[addr & 0x07FF];
}

u16* nesGetFrameBuffer() {
    return frameBuf;
}

u16* nesGetVisibleFrameBuffer() {
    if (!frameBuf || !visibleBuf) return nullptr;
    // Skip swizzle if frame hasn't changed (game paused / browsing maps)
    if (!visibleBufDirty) return visibleBuf;
    visibleBufDirty = false;
    // Swizzle into Morton-order (PICA200 tiled format)
    // 256x256 texture, 8x8 blocks, RGB565 = 2 bytes per pixel
    for (u16 y = 0; y < 240; y++) {
        for (u16 x = 0; x < 256; x++) {
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
            u32 blockIndex = blockY * 32 + blockX;
            u32 tiledOffset = (blockIndex * 64 + morton);
            u16 src = frameBuf[y * 512 + x];
            visibleBuf[tiledOffset] = src;
        }
    }
    return visibleBuf;
}

void nesGetFrameBufferSize(int* width, int* height) {
    if (width)  *width  = 256;
    if (height) *height = 240;
}

void nesSetInput(u32 buttons) {
    if (!nes || !nes->pad) return;
    // Set NES controller state via public API
    // buttons bitmask: A=1, B=2, Select=4, Start=8, Up=16, Down=32, Left=64, Right=128
    nes->pad->SetSyncData(buttons);
}

void nesGetAudio(short* buffer, int maxSamples, int* count) {
    if (!nes || !nes->apu || !buffer) { if (count) *count = 0; return; }
    int samples = maxSamples;
    if (samples > 8192) samples = 8192;
    // Write APU output directly into caller's buffer — no intermediate copy
    nes->apu->Process((u8*)buffer, samples * 2, false);
    if (count) *count = samples;
}

// Build state file path: replace .nes with .st1/.st2/.st3
static void buildStatePath(char* out, size_t outSize, int slot) {
    strncpy(out, currentRomPath, outSize - 1);
    out[outSize - 1] = 0;
    char ext[8];
    snprintf(ext, sizeof(ext), ".st%d", slot);
    char* dot = strrchr(out, '.');
    if (dot) {
        strncpy(dot, ext, outSize - (dot - out) - 1);
    } else {
        strncat(out, ext, outSize - strlen(out) - 1);
    }
}

bool nesSaveState(int slot) {
    if (!nes || currentRomPath[0] == 0) return false;
    char statePath[512];
    buildStatePath(statePath, sizeof(statePath), slot);
    return nes->SaveState(statePath) ? true : false;
}

bool nesLoadState(int slot) {
    if (!nes || currentRomPath[0] == 0) return false;
    char statePath[512];
    buildStatePath(statePath, sizeof(statePath), slot);
    return nes->LoadState(statePath) ? true : false;
}

// ---------------------------------------------------------------------------
// Background emulation thread — runs on core 1
// ---------------------------------------------------------------------------
static void emuThreadEntry(void* /*arg*/) {
    while (true) {
        LightEvent_Wait(&g_emuKick);
        if (g_emuQuit) return;          // exit without signalling — threadJoin handles sync
        nesEmulateFrame();
        LightEvent_Signal(&g_emuDone);
    }
}

void nesStartThread() {
    LightEvent_Init(&g_emuKick, RESET_ONESHOT);
    LightEvent_Init(&g_emuDone, RESET_ONESHOT);
    g_emuQuit   = false;
    // Priority 0x30 = normal user-land; affinity 1 = core 1 (available on both
    // Old and New 3DS), leaving core 0 free for rendering.
    g_emuThread = threadCreate(emuThreadEntry, nullptr, 128 * 1024, 0x30, 1, false);
    // If threadCreate returns NULL the kick/wait helpers fall back to sync mode.
}

void nesStopThread() {
    if (!g_emuThread) return;
    g_emuQuit = true;
    LightEvent_Signal(&g_emuKick);          // wake thread so it sees the quit flag
    threadJoin(g_emuThread, U64_MAX);       // wait for it to fully return
    threadFree(g_emuThread);
    g_emuThread = NULL;
}

void nesKickFrame() {
    if (!g_emuThread) { nesEmulateFrame(); return; }   // sync fallback
    LightEvent_Signal(&g_emuKick);
}

void nesWaitFrame() {
    if (!g_emuThread) return;              // sync fallback: already done
    LightEvent_Wait(&g_emuDone);
}

void nesShutdown() {
    if (nes) {
        delete nes;
        nes = nullptr;
    }
    if (frameBuf) {
        linearFree(frameBuf);
        frameBuf = nullptr;
    }
    if (visibleBuf) {
        linearFree(visibleBuf);
        visibleBuf = nullptr;
    }
}
