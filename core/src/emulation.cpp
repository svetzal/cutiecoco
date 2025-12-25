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

#include "dream/emulation.h"
#include <cstring>

namespace dream {

EmulationThread::EmulationThread()
    : m_framebuffer(FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * 4, 0)
{
}

EmulationThread::~EmulationThread()
{
    stop();
}

void EmulationThread::start(FrameReadyCallback callback)
{
    if (m_running.load()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_frameCallback = std::move(callback);
    }

    m_running.store(true);
    m_paused.store(false);
    m_thread = std::thread(&EmulationThread::threadMain, this);
}

void EmulationThread::stop()
{
    if (!m_running.load()) {
        return;
    }

    m_running.store(false);
    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_frameCallback = nullptr;
}

void EmulationThread::pause()
{
    m_paused.store(true);
}

void EmulationThread::resume()
{
    m_paused.store(false);
}

void EmulationThread::reset()
{
    m_resetRequested.store(true);
}

void EmulationThread::initializeEmulation()
{
    // TODO: Initialize CPU, MMU, GIME, etc.
    // For now, fill framebuffer with CoCo green screen
    for (int y = 0; y < FRAMEBUFFER_HEIGHT; ++y) {
        for (int x = 0; x < FRAMEBUFFER_WIDTH; ++x) {
            size_t offset = (y * FRAMEBUFFER_WIDTH + x) * 4;
            m_framebuffer[offset + 0] = 0;    // R
            m_framebuffer[offset + 1] = 255;  // G (CoCo green)
            m_framebuffer[offset + 2] = 0;    // B
            m_framebuffer[offset + 3] = 255;  // A
        }
    }

    m_resetRequested.store(false);
}

void EmulationThread::threadMain()
{
    initializeEmulation();

    m_frameStartTime = Clock::now();
    m_fpsCounterStartTime = Clock::now();
    m_frameCount = 0;

    while (m_running.load()) {
        // Handle reset request
        if (m_resetRequested.load()) {
            initializeEmulation();
        }

        // Handle pause
        if (m_paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            m_frameStartTime = Clock::now();
            continue;
        }

        // Record frame start time
        auto frameStart = Clock::now();

        // Render one frame
        renderFrame();

        // Notify callback that frame is ready
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            if (m_frameCallback) {
                m_frameCallback(m_framebuffer.data(), FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
            }
        }

        // Frame timing / throttling
        if (m_throttled.load()) {
            auto frameEnd = Clock::now();
            auto elapsed = frameEnd - frameStart;
            auto targetDuration = std::chrono::duration_cast<Clock::duration>(FRAME_DURATION);

            if (elapsed < targetDuration) {
                // Sleep for remaining time (with some margin for scheduler jitter)
                auto sleepTime = targetDuration - elapsed;
                auto sleepMs = std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime);

                if (sleepMs.count() > 1) {
                    // Sleep for most of the remaining time
                    std::this_thread::sleep_for(sleepMs - std::chrono::milliseconds(1));
                }

                // Busy-wait for the remaining time for precision
                while (Clock::now() - frameStart < targetDuration) {
                    std::this_thread::yield();
                }
            }
        }

        // Calculate FPS every second
        ++m_frameCount;
        auto now = Clock::now();
        auto fpsDuration = std::chrono::duration<double>(now - m_fpsCounterStartTime);
        if (fpsDuration.count() >= 1.0) {
            m_fps.store(static_cast<float>(m_frameCount / fpsDuration.count()));
            m_frameCount = 0;
            m_fpsCounterStartTime = now;
        }
    }
}

void EmulationThread::renderFrame()
{
    // TODO: This is where the actual emulation happens
    // For now, generate a test pattern that changes over time

    static int frame = 0;
    ++frame;

    // Generate a simple animated test pattern
    // This demonstrates the framebuffer is working and being updated
    for (int y = 0; y < FRAMEBUFFER_HEIGHT; ++y) {
        for (int x = 0; x < FRAMEBUFFER_WIDTH; ++x) {
            size_t offset = (y * FRAMEBUFFER_WIDTH + x) * 4;

            // Create a CoCo-like green screen with a simple pattern
            // Border area
            bool isBorder = (x < 32 || x >= FRAMEBUFFER_WIDTH - 32 ||
                            y < 24 || y >= FRAMEBUFFER_HEIGHT - 24);

            if (isBorder) {
                // CoCo green border
                m_framebuffer[offset + 0] = 0;
                m_framebuffer[offset + 1] = 255;
                m_framebuffer[offset + 2] = 0;
                m_framebuffer[offset + 3] = 255;
            } else {
                // Animated content area - simple color bars that scroll
                int col = ((x - 32) / 64 + frame / 30) % 8;

                // CoCo palette colors (simplified)
                static const uint8_t palette[8][3] = {
                    {0, 0, 0},       // Black
                    {0, 0, 255},     // Blue
                    {0, 255, 0},     // Green
                    {0, 255, 255},   // Cyan
                    {255, 0, 0},     // Red
                    {255, 0, 255},   // Magenta
                    {255, 255, 0},   // Yellow
                    {255, 255, 255}, // White
                };

                m_framebuffer[offset + 0] = palette[col][0];
                m_framebuffer[offset + 1] = palette[col][1];
                m_framebuffer[offset + 2] = palette[col][2];
                m_framebuffer[offset + 3] = 255;
            }
        }
    }
}

} // namespace dream
