#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

#include <cstdint>
#include <vector>

namespace cutie {
namespace win32 {

// Windows audio output using waveOut API
// TODO: Add WASAPI backend option for lower latency (DREAM-VCC-q2i)
class Win32Audio {
public:
    Win32Audio();
    ~Win32Audio();

    // Initialize audio with sample rate
    bool init(uint32_t sampleRate);
    void shutdown();

    // Audio buffer management
    void submitSamples(const int16_t* samples, size_t count);
    size_t getQueuedSampleCount() const;

    // State
    bool isInitialized() const { return m_initialized; }
    uint32_t sampleRate() const { return m_sampleRate; }

private:
    static void CALLBACK waveOutCallback(
        HWAVEOUT hwo, UINT msg, DWORD_PTR instance,
        DWORD_PTR param1, DWORD_PTR param2);

    void onBufferDone(WAVEHDR* header);

private:
    HWAVEOUT m_waveOut = nullptr;
    bool m_initialized = false;
    uint32_t m_sampleRate = 44100;

    // Double-buffered audio
    static constexpr int NUM_BUFFERS = 4;
    static constexpr int BUFFER_SIZE_SAMPLES = 2048;

    struct AudioBuffer {
        WAVEHDR header = {};
        std::vector<int16_t> data;
        bool inUse = false;
    };

    AudioBuffer m_buffers[NUM_BUFFERS];
    int m_currentBuffer = 0;
    size_t m_queuedSamples = 0;
};

}  // namespace win32
}  // namespace cutie
