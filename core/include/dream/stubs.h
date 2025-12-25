#ifndef DREAM_STUBS_H
#define DREAM_STUBS_H
/*
Copyright 2024 DREAM-VCC Contributors
Stub implementations for removed Windows-specific functionality.

These stubs allow the emulation code to compile while the full
Qt implementations are being developed.
*/

#include <cstdint>

// ============================================================================
// Cassette stubs (was in Cassette.h)
// ============================================================================

inline uint8_t GetMotorState() { return 0; }
inline void FlushCassetteBuffer(unsigned char*, unsigned int*) {}
inline void LoadCassetteBuffer(unsigned char*, unsigned int*) {}
inline uint8_t GetCasSample() { return 0x80; }  // Silence
inline void SetCassetteSample(uint8_t) {}
inline unsigned int GetTapeRate() { return 44100; }

// ============================================================================
// DirectDraw/Display stubs (was in DirectDrawInterface.h)
// ============================================================================

inline int LockScreen() { return 0; }  // Return 0 = success
inline void UnlockScreen(void*) {}
inline uint8_t GetMem(unsigned int) { return 0; }

// ============================================================================
// Audio stubs (was in audio.h - old Windows version)
// ============================================================================

inline int GetFreeBlockCount() { return 4; }  // Pretend we have buffer space
inline void FlushAudioBuffer(unsigned int*, unsigned int) {}
inline void ResetAudio() {}
inline unsigned int GetDACSample() { return 0x80008000; }  // Silence (stereo)

// ============================================================================
// Keyboard stubs (was in keyboard.h)
// ============================================================================

inline void PasteIntoQueue(const std::string&) {}

// ============================================================================
// Config stubs (was in config.h)
// ============================================================================

struct ForcedAspectData {
    int x = 0;
    int y = 0;
};

inline ForcedAspectData GetForcedAspectBorderPadding() {
    return ForcedAspectData{0, 0};
}

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

// ============================================================================
// PIA cassette/mux stubs (from mc6821)
// ============================================================================

#define CAS_SILENCE 0x80
#define PIA_MUX_CASSETTE 1

inline int GetMuxState() { return 0; }  // Not cassette mode

#endif // DREAM_STUBS_H
