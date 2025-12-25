#ifndef CUTIE_COMPAT_H
#define CUTIE_COMPAT_H

// Compatibility header for legacy VCC code
// Provides types that were previously in defines.h

#include "types.h"
#include "debugger.h"
#include <cstdint>
#include <atomic>
#include <cassert>
#include <cstring>
#include <array>

// Legacy constants
constexpr auto MAX_LOADSTRING = 400u;
constexpr auto FRAMEINTERVAL = 120u;
constexpr auto TARGETFRAMERATE = 60u;
constexpr auto SAMPLESPERFRAME = 262u;
constexpr auto FRAMESPERSECORD = 59.923;
constexpr auto LINESPERSCREEN = 262.0;
constexpr auto NANOSECOND = 1000000000.0;
constexpr auto COLORBURST = 3579545.0;
constexpr auto AUDIOBUFFERS = 12u;
constexpr auto QUERY = 255u;

// Interrupt type constants (for debugger/tracing)
constexpr auto FIRQ = 0;
constexpr auto IRQ = 1;
constexpr auto NMI = 2;

// Forward declaration
struct SystemState;

// VCC namespace for legacy compatibility
namespace VCC {
    const int DefaultWidth = 640;
    const int DefaultHeight = 480;

    // Alias for CPUState from types.h
    using CPUState = cutie::CPUState;

    union Pixel;

    struct Point {
        int x{}, y{};
        Point() = default;
        Point(int x, int y) : x(x), y(y) {}
    };

    struct Size {
        int w{}, h{};
        Size() = default;
        Size(int w, int h) : w(w), h(h) {}
    };

    struct Rect : Point, Size {
        Rect() = default;
        Rect(int x, int y, int w, int h) : Point(x, y), Size(w, h) {}
        bool IsDefaultX() const { return false; }
        bool IsDefaultY() const { return false; }
        int Top() const { return y; }
        int Left() const { return x; }
        int Right() const { return x + w; }
        int Bottom() const { return y + h; }
    };

    // Bounds checking array type
    template <class T, size_t Size>
    struct Array {
        void MemCpyFrom(const void* buffer, size_t bytes) {
            assert(bytes <= sizeof(data));
            memcpy(&data[0], buffer, bytes);
        }

        void MemCpyTo(void* outBuffer, size_t bytes) {
            MemCpyTo(0, outBuffer, bytes);
        }

        void MemCpyTo(size_t startElement, void* outBuffer, size_t bytes) {
            assert(startElement * sizeof(T) + bytes <= sizeof(data));
            memcpy(outBuffer, &data[startElement], bytes);
        }

        void MemSet(uint8_t fill, size_t bytes) {
            assert(bytes <= sizeof(data));
            memset(data.data(), fill, bytes);
        }

        T& operator[](size_t index) { return data[index]; }
        const T& operator[](size_t index) const { return data[index]; }

    private:
        std::array<T, Size> data;
    };

    // Video array with wrap-around bounds checking
    template <class T, size_t Size>
    struct VideoArray {
        explicit VideoArray(T* p) : data(p) {}

        T& operator[](size_t index) {
            assert(index < Size);
            return data[index % Size];
        }

        const T& operator[](size_t index) const {
            assert(index < Size);
            return data[index % Size];
        }

    private:
        T* data;
    };

    // Abstract interface for system state
    struct ISystemState {
        enum { OK };
        virtual ~ISystemState() = default;

        virtual int GetWindowHandle(void** handle) = 0;
        virtual int GetRect(int rectOption, Rect* rect) = 0;
        virtual void SetSurface(void* ptr, uint8_t bitDepth, long stride) = 0;
    };

    // Stub for debugger haltpoint functionality
    inline void ApplyHaltpoints(bool) {}
}

// Main system state structure
struct SystemState {
    void* WindowHandle = nullptr;
    void* ConfigDialog = nullptr;
    void* WindowInstance = nullptr;

    unsigned char* RamBuffer = nullptr;
    unsigned short* WRamBuffer = nullptr;
    std::atomic<unsigned char> RamSize{0};

    double CPUCurrentSpeed = 0.0;
    unsigned char DoubleSpeedMultiplyer = 0;
    unsigned char DoubleSpeedFlag = 0;
    unsigned char TurboSpeedFlag = 0;
    unsigned char CpuType = 0;
    unsigned char FrameSkip = 1;  // Must be >= 1 to avoid division by zero
    unsigned char BitDepth = 0;
    unsigned char Throttle = 0;

    unsigned char* PTRsurface8 = nullptr;
    unsigned short* PTRsurface16 = nullptr;
    unsigned int* PTRsurface32 = nullptr;
    long SurfacePitch = 0;

    unsigned short LineCounter = 0;
    unsigned char ScanLines = 0;
    unsigned char EmulationRunning = 0;
    unsigned char ResetPending = 0;

    VCC::Size WindowSize;
    unsigned char FullScreen = 0;
    bool Exiting = false;
    unsigned char MousePointer = 0;
    unsigned char OverclockFlag = 0;
    char StatusLine[256] = {0};
    float FPS = 0.0f;

    // Debugger stub
    VCC::Debugger::Debugger Debugger;
};

// Abstract interface over SystemState
struct SystemStatePtr : VCC::ISystemState {
    explicit SystemStatePtr(SystemState* state) : state(state) {}

    int GetWindowHandle(void** handle) override {
        *handle = state->WindowHandle;
        return OK;
    }
    int GetRect(int /*rectOption*/, VCC::Rect* rect) override {
        rect->x = 0;
        rect->y = 0;
        rect->w = state->WindowSize.w;
        rect->h = state->WindowSize.h;
        return OK;
    }
    void SetSurface(void* ptr, uint8_t bitDepth, long stride) override {
        state->PTRsurface8 = static_cast<unsigned char*>(ptr);
        state->PTRsurface16 = static_cast<unsigned short*>(ptr);
        state->PTRsurface32 = static_cast<unsigned int*>(ptr);
        state->BitDepth = bitDepth;
        state->SurfacePitch = stride;
    }

private:
    SystemState* state;
};

// Global emulator state
extern SystemState EmuState;

// Audio rate lists
static char RateList[4][7] = {"Mute", "11025", "22050", "44100"};
static unsigned int iRateList[4] = {0, 11025, 22050, 44100};

#endif // CUTIE_COMPAT_H
