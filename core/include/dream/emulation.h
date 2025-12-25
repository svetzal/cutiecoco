#ifndef DREAM_EMULATION_H
#define DREAM_EMULATION_H
/*
Copyright 2024 DREAM-VCC Contributors
This file is part of DREAM-VCC.

    DREAM-VCC is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DREAM-VCC is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DREAM-VCC.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

namespace dream {

// Frame timing constants for CoCo 3 emulation
constexpr double FRAME_RATE = 59.923;
constexpr auto FRAME_DURATION = std::chrono::duration<double>(1.0 / FRAME_RATE);

// Framebuffer dimensions (CoCo 3 max resolution)
constexpr int FRAMEBUFFER_WIDTH = 640;
constexpr int FRAMEBUFFER_HEIGHT = 480;

/**
 * @brief Callback invoked when a frame is ready for display
 * @param pixels Pointer to RGBA pixel data (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * 4 bytes)
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 */
using FrameReadyCallback = std::function<void(const uint8_t* pixels, int width, int height)>;

/**
 * @brief Manages the emulation loop in a separate thread
 *
 * This class runs the CoCo 3 emulation at the correct frame rate (~59.923 Hz)
 * using std::chrono for precise timing. It signals the main thread when
 * each frame is ready for display.
 */
class EmulationThread {
public:
    EmulationThread();
    ~EmulationThread();

    // Non-copyable
    EmulationThread(const EmulationThread&) = delete;
    EmulationThread& operator=(const EmulationThread&) = delete;

    /**
     * @brief Start the emulation thread
     * @param callback Function called when each frame is ready
     */
    void start(FrameReadyCallback callback);

    /**
     * @brief Stop the emulation thread and wait for it to finish
     */
    void stop();

    /**
     * @brief Check if emulation is currently running
     */
    bool isRunning() const { return m_running.load(); }

    /**
     * @brief Pause emulation (thread keeps running but doesn't advance)
     */
    void pause();

    /**
     * @brief Resume emulation after pause
     */
    void resume();

    /**
     * @brief Check if emulation is paused
     */
    bool isPaused() const { return m_paused.load(); }

    /**
     * @brief Reset the emulated system
     */
    void reset();

    /**
     * @brief Get the current FPS (frames per second)
     */
    float getFps() const { return m_fps.load(); }

    /**
     * @brief Enable or disable throttling (run at full speed if disabled)
     */
    void setThrottled(bool throttled) { m_throttled.store(throttled); }

    /**
     * @brief Check if throttling is enabled
     */
    bool isThrottled() const { return m_throttled.load(); }

private:
    void threadMain();
    void renderFrame();
    void initializeEmulation();

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_throttled{true};
    std::atomic<bool> m_resetRequested{false};
    std::atomic<float> m_fps{0.0f};

    FrameReadyCallback m_frameCallback;
    std::mutex m_callbackMutex;

    // Framebuffer (RGBA format)
    std::vector<uint8_t> m_framebuffer;

    // Timing
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_frameStartTime;
    int m_frameCount{0};
    Clock::time_point m_fpsCounterStartTime;
};

} // namespace dream

#endif // DREAM_EMULATION_H
