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
#include "dream/framebuffer.h"
#include "dream/compat.h"  // For EmuState
#include "dream/stubs.h"   // For CPUExecFuncPtr
#include "mc6809.h"
#include "hd6309.h"
#include "tcc1014mmu.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "coco3.h"
#include <cstring>

namespace dream {

EmulationThread::EmulationThread()
    : m_framebuffer(std::make_unique<FrameBuffer>(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT))
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
    // Initialize memory subsystem (512K RAM by default)
    unsigned char* memory = MmuInit(_512K);
    if (memory == nullptr) {
        fprintf(stderr, "Failed to initialize MMU\n");
        m_framebuffer->clear(0xFF0000FF);  // Red = error
        return;
    }

    // Set up the global EmuState with our framebuffer
    EmuState.PTRsurface32 = m_framebuffer->pixels();
    EmuState.SurfacePitch = m_framebuffer->pitch();
    EmuState.BitDepth = 3;  // Index 3 = 32-bit mode (0=8, 1=16, 2=24, 3=32)
    EmuState.RamBuffer = memory;
    EmuState.EmulationRunning = 1;
    EmuState.WindowSize = VCC::Size(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);

    // Initialize GIME/SAM (must be before CPU reset to set up ROM pointer)
    GimeInit();
    GimeReset();
    mc6883_reset();  // Initialize SAM and ROM pointer

    // Initialize CPU (default to 6809)
    MC6809Init();
    MC6809Reset();

    // Disable audio for now (no audio backend implemented yet)
    SetAudioRate(0);

    // Reset misc (timers, interrupts, etc.)
    MiscReset();

    // Set the CPUExec function pointer to the real CPU
    // CPUExec is defined in core.cpp at global scope
    ::CPUExec = MC6809Exec;

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
                m_frameCallback(m_framebuffer->data(), FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
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
    // Update the surface pointer in case it changed
    EmuState.PTRsurface32 = m_framebuffer->pixels();
    EmuState.SurfacePitch = m_framebuffer->pitch();

    // Run one frame of emulation
    // RenderFrame handles CPU execution, GIME rendering, audio, etc.
    RenderFrame(&EmuState);
}

} // namespace dream
