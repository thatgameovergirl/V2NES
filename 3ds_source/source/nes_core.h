#ifndef NES_CORE_H
#define NES_CORE_H

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize NES core emulator
bool nesInit();

// Load a ROM file
bool nesLoadROM(const char* path);

// Emulate one frame (call once per frame)
void nesEmulateFrame();

// Get frame buffer (16-bit RGB565, 256x240 pixels)
u16* nesGetFrameBuffer();

// Get packed visible frame buffer (compact 256x240, no 512-pitch stride)
// Returns a pointer valid until next call to nesEmulateFrame
u16* nesGetVisibleFrameBuffer();

// Get frame buffer dimensions
void nesGetFrameBufferSize(int* width, int* height);

// Set NES controller buttons (bitmask using NES button defines below)
void nesSetInput(u32 buttons);

// Get audio samples (call after each emulated frame)
// Returns mono 16-bit samples, sets count to number of samples
void nesGetAudio(short* buffer, int maxSamples, int* count);

// Returns true if the frame changed since last nesGetVisibleFrameBuffer call
bool nesFrameChanged();

// Read a byte from the NES CPU's 2KB internal RAM (addresses 0x0000-0x07FF)
u8 nesReadRAM(u16 addr);

// Save state to slot (1-3), writes .st1/.st2/.st3 next to ROM
bool nesSaveState(int slot);

// Load state from slot (1-3), reads .st1/.st2/.st3 next to ROM
bool nesLoadState(int slot);

// Shutdown and cleanup
void nesShutdown();

// Background emulation thread (runs on core 1, parallel with rendering on core 0).
// Call nesStartThread() once after nesInit(), nesStopThread() before nesShutdown().
// Replace direct nesEmulateFrame() calls with nesKickFrame() + nesWaitFrame() pairs.
// Falls back to synchronous emulation if thread creation fails.
void nesStartThread();
void nesStopThread();
void nesKickFrame();   // signal the emu thread to run one frame
void nesWaitFrame();   // block until the kicked frame completes

// NES Button bitmask values
#define NES_BTN_A       0x01
#define NES_BTN_B       0x02
#define NES_BTN_SELECT  0x04
#define NES_BTN_START   0x08
#define NES_BTN_UP      0x10
#define NES_BTN_DOWN    0x20
#define NES_BTN_LEFT    0x40
#define NES_BTN_RIGHT   0x80

#ifdef __cplusplus
}
#endif

#endif
