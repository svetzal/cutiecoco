#ifndef CUTIE_STUBS_H
#define CUTIE_STUBS_H
/*
Copyright 2024-2025 CutieCoCo Contributors
Stub implementations for removed Windows-specific functionality.

These stubs allow the emulation code to compile while the full
Qt implementations are being developed.
*/

#include <cstdint>
#include <cstdio>
#include <string>

// ============================================================================
// Cassette stubs (was in Cassette.h)
// Note: GetCasSample, SetCassetteSample defined in mc6821.cpp
// ============================================================================

inline uint8_t GetMotorState() { return 0; }
inline void FlushCassetteBuffer(unsigned char*, unsigned int*) {}
inline void LoadCassetteBuffer(unsigned char*, unsigned int*) {}
inline unsigned int GetTapeRate() { return 44100; }
inline void Motor(unsigned char) {}  // Cassette motor control

// ============================================================================
// DirectDraw/Display stubs (was in DirectDrawInterface.h)
// ============================================================================

inline int LockScreen() { return 0; }  // Return 0 = success
inline void UnlockScreen(void*) {}
inline uint8_t GetMem(unsigned int) { return 0; }

// ============================================================================
// Audio stubs (was in audio.h - old Windows version)
// Note: GetDACSample is defined in mc6821.cpp
// ============================================================================

// Default audio sample rate
constexpr unsigned int AUDIO_RATE = 44100;

inline int GetFreeBlockCount() { return 4; }  // Pretend we have buffer space
inline void FlushAudioBuffer(unsigned int*, unsigned int) {}
inline void ResetAudio() {}

// Pak audio sample (from cartridge)
inline unsigned int PackAudioSample() { return 0; }

// ============================================================================
// Joystick - now implemented in cutie/joystick.h
// ============================================================================

// Joystick functions are now provided by cutie/joystick.h:
// - vccJoystickGetButtonBits()
// - vccJoystickStartRamp()
// - vccJoystickGetComparison()

// ============================================================================
// Keyboard stubs (was in keyboard.h)
// ============================================================================

inline void PasteIntoQueue(const std::string&) {}

// ============================================================================
// Config stubs (was in config.h)
// ============================================================================

#include <filesystem>
#include <string>

struct ForcedAspectData {
    int x = 0;
    int y = 0;
};

inline ForcedAspectData GetForcedAspectBorderPadding() {
    return ForcedAspectData{0, 0};
}

inline bool GetUseCustomSystemRom() { return false; }
inline std::filesystem::path GetCustomSystemRomPath() { return {}; }

// ============================================================================
// Windows API stubs
// ============================================================================

// MessageBox stub - just logs and returns
#ifndef MB_OK
#define MB_OK 0
#define MB_TASKMODAL 0
#define MB_TOPMOST 0
#define MB_SETFOREGROUND 0
#define MB_ICONERROR 0
#endif

inline int MessageBox(void*, const char* message, const char*, unsigned int) {
    fprintf(stderr, "MessageBox: %s\n", message);
    return 0;
}

// OutputDebugString stub
inline void OutputDebugString(const char* msg) {
    fprintf(stderr, "Debug: %s", msg);
}

// Palette type constants
#define PALETTE_RGB 0
#define PALETTE_NTSC 1

// Palette config stub
inline int GetPaletteType() { return PALETTE_RGB; }

// Screen clear stub (was in DirectDrawInterface or similar)
struct SystemState;
inline void Cls(unsigned int, SystemState*) {}

// ============================================================================
// Throttle stubs (was in throttle.h - Windows implementation)
// ============================================================================

inline void CalibrateThrottle() {}
inline void StartRender() {}
inline void EndRender(unsigned char) {}
inline void FrameWait() {}
inline float CalculateFPS() { return 60.0f; }

// ============================================================================
// Vcc.h stubs
// ============================================================================

inline void SetCPUMultiplyerFlag(unsigned char) {}
inline unsigned char GetCPUMultiplyerFlag() { return 1; }
inline void SetTurboMode(unsigned char) {}  // Turbo speed mode

// ============================================================================
// CPU interrupt stubs
// These forward to the actual CPU when it's active
// ============================================================================

// Forward declarations of actual CPU functions (mc6809.cpp, hd6309.cpp)
void MC6809AssertInterupt(unsigned char, unsigned char);
void MC6809DeAssertInterupt(unsigned char);
void HD6309AssertInterupt(unsigned char, unsigned char);
void HD6309DeAssertInterupt(unsigned char);

// Current CPU type (0 = 6809, 1 = 6309)
extern unsigned char CurrentCPUType;

// Wrapper functions that forward to the active CPU
inline void CPUAssertInterupt(unsigned char irqType, unsigned char state) {
    if (CurrentCPUType == 0) {
        MC6809AssertInterupt(irqType, state);
    } else {
        HD6309AssertInterupt(irqType, state);
    }
}

inline void CPUDeAssertInterupt(unsigned char irqType) {
    if (CurrentCPUType == 0) {
        MC6809DeAssertInterupt(irqType);
    } else {
        HD6309DeAssertInterupt(irqType);
    }
}

// ============================================================================
// PIA cassette/mux stubs (from mc6821)
// Note: GetMuxState and PIA_MUX_* are defined in mc6821.h
// ============================================================================

#define CAS_SILENCE 0x80

// ============================================================================
// CPU execution (was in Vcc.cpp)
// ============================================================================

// CPU execution function pointer type
typedef int (*CPUExecFuncPtr)(int);

// Default CPU execution stub - returns 0 cycles executed
// Real implementation sets this to MC6809Exec or HD6309Exec
int CPUExecStub(int cycles);

// Extern declaration for CPUExec function pointer
extern CPUExecFuncPtr CPUExec;

// ============================================================================
// pakinterface stubs (was in pakinterface.cpp)
// ============================================================================

// Storage for the system ROM path (set by Qt on startup)
inline std::filesystem::path& getSystemRomPathStorage() {
    static std::filesystem::path path;
    return path;
}

// Set the system ROM path (called by Qt app on startup)
inline void SetSystemRomPath(const std::filesystem::path& path) {
    getSystemRomPathStorage() = path;
}

// Get the path to system ROMs directory
// This returns the path set by Qt, or falls back to current directory
inline std::filesystem::path PakGetSystemRomPath() {
    const auto& path = getSystemRomPathStorage();
    if (!path.empty()) {
        return path;
    }
    // Fallback for development - look in current working directory
    return std::filesystem::current_path() / "system-roms";
}

// Cartridge functions - now implemented in cutie/cartridge.h
// Forward declarations for C-compatible functions
extern "C" {
    unsigned char vccCartridgeRead(unsigned short address);
    void vccCartridgeWritePort(unsigned char port, unsigned char value);
    unsigned char vccCartridgeReadPort(unsigned char port);
    unsigned char vccCartridgeIsInserted();
}

// Read from cartridge/pak memory
inline unsigned char PackMem8Read(unsigned short address) {
    return vccCartridgeRead(address);
}

// Read from pak port
inline unsigned char PakReadPort(unsigned char port) {
    return vccCartridgeReadPort(port);
}

// Write to pak port
inline void PakWritePort(unsigned char port, unsigned char value) {
    vccCartridgeWritePort(port, value);
}

// Pak timer tick - called each frame (no-op for simple ROM carts)
inline void PakTimer() {
    // ROM cartridges don't need timer ticks
}

#endif // CUTIE_STUBS_H
