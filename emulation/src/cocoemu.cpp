/*
Copyright 2024-2025 CutieCoCo Contributors
This file is part of CutieCoCo.

    CutieCoCo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CutieCoCo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CutieCoCo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cutie/emulator.h"
#include "cutie/framebuffer.h"
#include "cutie/keyboard.h"
#include "cutie/joystick.h"
#include "cutie/cartridge.h"
#include "cutie/compat.h"  // For EmuState
#include "cutie/stubs.h"   // For CPUExec
#include "mc6809.h"
#include "hd6309.h"
#include "tcc1014mmu.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "coco3.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace cutie {

namespace {
    // Framebuffer dimensions (CoCo 3 max resolution)
    constexpr int FRAMEBUFFER_WIDTH = 640;
    constexpr int FRAMEBUFFER_HEIGHT = 480;

    // Convert MemorySize enum to legacy MMU size values
    // Legacy macros: _128K=0, _512K=1, _2M=2
    unsigned char toMmuSize(MemorySize size) {
        switch (size) {
            case MemorySize::Mem128K: return 0;  // _128K
            case MemorySize::Mem512K: return 1;  // _512K
            case MemorySize::Mem2M:   return 2;  // _2M
            case MemorySize::Mem8M:   return 2;  // Max supported is 2M for now
        }
        return 1;  // Default to 512K
    }
}

/**
 * @brief Concrete implementation of the CocoEmulator interface
 */
class CocoEmulatorImpl : public CocoEmulator {
public:
    explicit CocoEmulatorImpl(const EmulatorConfig& config)
        : m_config(config)
        , m_framebuffer(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT)
    {
    }

    ~CocoEmulatorImpl() override {
        shutdown();
    }

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool init() override {
        if (m_ready) {
            return true;
        }

        // Initialize memory subsystem
        m_memory = MmuInit(toMmuSize(m_config.memorySize));
        if (m_memory == nullptr) {
            m_lastError = "Failed to initialize MMU";
            return false;
        }

        // Set up the global EmuState with our framebuffer
        EmuState.PTRsurface32 = m_framebuffer.pixels();
        EmuState.SurfacePitch = m_framebuffer.pitch();
        EmuState.BitDepth = 3;  // Index 3 = 32-bit mode
        EmuState.RamBuffer = m_memory;
        EmuState.EmulationRunning = 1;
        EmuState.WindowSize = VCC::Size(FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);

        // Initialize GIME/SAM (must be before CPU reset to set up ROM pointer)
        GimeInit();
        GimeReset();
        mc6883_reset();

        // Initialize CPU based on config
        if (m_config.cpuType == CpuType::HD6309) {
            HD6309Init();
            HD6309Reset();
            ::CPUExec = HD6309Exec;
        } else {
            MC6809Init();
            MC6809Reset();
            ::CPUExec = MC6809Exec;
        }

        // Reset misc (timers, interrupts, etc.)
        // IMPORTANT: Must be called BEFORE SetAudioRate because MiscReset
        // resets audio timing variables to 0
        MiscReset();

        // Enable audio at configured sample rate
        // The audio buffer is drained after each frame in runFrame()
        if (m_config.audioSampleRate > 0) {
            SetAudioRate(m_config.audioSampleRate);
            // Reserve space for audio samples (max ~44100/60 = ~735 samples per frame)
            m_audioSamples.reserve(1024);
        } else {
            SetAudioRate(0);
        }

        m_cpuType = m_config.cpuType;
        m_ready = true;
        m_lastError.clear();

        return true;
    }

    void reset() override {
        if (!m_ready) {
            return;
        }

        // Reset GIME/SAM first
        GimeReset();
        mc6883_reset();

        // Reset CPU
        if (m_cpuType == CpuType::HD6309) {
            HD6309Reset();
        } else {
            MC6809Reset();
        }

        // Reset misc
        MiscReset();
    }

    void shutdown() override {
        if (!m_ready) {
            return;
        }

        EmuState.EmulationRunning = 0;
        m_ready = false;
    }

    // ========================================================================
    // Execution
    // ========================================================================

    void runFrame() override {
        if (!m_ready) {
            return;
        }

        // Update the surface pointer in case it changed
        EmuState.PTRsurface32 = m_framebuffer.pixels();
        EmuState.SurfacePitch = m_framebuffer.pitch();

        // Run one frame of emulation
        RenderFrame(&EmuState);

        // Capture audio samples from the legacy buffer
        // This also resets the audio index for the next frame
        captureAudioSamples();
    }

    void captureAudioSamples() {
        // Get raw samples from coco3.cpp (32-bit stereo)
        const unsigned int* rawBuffer = GetAudioBuffer();
        unsigned int sampleCount = GetAudioSampleCount();

        // Clear previous frame's samples
        m_audioSamples.clear();

        if (sampleCount == 0 || rawBuffer == nullptr) {
            return;
        }

        // Convert 32-bit stereo to 16-bit mono
        // Legacy format: low 16 bits = left, high 16 bits = right
        // CoCo DAC outputs 0 at idle, positive values for audio
        // Scale by 4x to use more of the 16-bit range
        constexpr int SCALE = 4;

        m_audioSamples.reserve(sampleCount);
        for (unsigned int i = 0; i < sampleCount; ++i) {
            // Extract left channel (low 16 bits) as signed
            int sample = static_cast<int>(rawBuffer[i] & 0xFFFF);
            // Scale for volume
            sample = sample * SCALE;
            // Clamp to 16-bit range
            if (sample > 32767) sample = 32767;
            m_audioSamples.push_back(static_cast<int16_t>(sample));
        }

        // Reset the audio index for the next frame
        ResetAudioIndex();
    }

    int runCycles(int cycles) override {
        if (!m_ready || cycles <= 0) {
            return 0;
        }

        // Call the appropriate CPU execution function
        if (::CPUExec) {
            return ::CPUExec(cycles);
        }
        return 0;
    }

    // ========================================================================
    // Input
    // ========================================================================

    void setKeyState(int row, int col, bool pressed) override {
        // Convert row/col to CocoKey enum
        // CocoKey is laid out as: row * 8 + col
        if (row < 0 || row >= 7 || col < 0 || col >= 8) {
            return;
        }

        auto key = static_cast<CocoKey>(row * 8 + col);
        auto& kb = getKeyboard();

        if (pressed) {
            kb.keyDown(key);
        } else {
            kb.keyUp(key);
        }
    }

    void setJoystickAxis(int joystick, int axis, int value) override {
        auto& joy = getJoystick();
        joy.setAxis(joystick, axis, value);
    }

    void setJoystickButton(int joystick, int button, bool pressed) override {
        auto& joy = getJoystick();
        joy.setButton(joystick, button, pressed);
    }

    // ========================================================================
    // Video Output
    // ========================================================================

    FrameBufferInfo getFramebufferInfo() const override {
        FrameBufferInfo info;
        info.width = m_framebuffer.width();
        info.height = m_framebuffer.height();
        info.pitch = m_framebuffer.pitch();
        return info;
    }

    std::pair<const uint8_t*, size_t> getFramebuffer() const override {
        return std::make_pair(
            m_framebuffer.data(),
            m_framebuffer.sizeBytes()
        );
    }

    // ========================================================================
    // Audio Output
    // ========================================================================

    AudioInfo getAudioInfo() const override {
        AudioInfo info;
        info.sampleRate = m_config.audioSampleRate;
        info.channels = 1;  // CoCo is mono
        info.bitsPerSample = 16;
        return info;
    }

    std::pair<const int16_t*, size_t> getAudioSamples() const override {
        if (m_audioSamples.empty()) {
            return std::make_pair(nullptr, 0);
        }
        return std::make_pair(m_audioSamples.data(), m_audioSamples.size());
    }

    // ========================================================================
    // Cartridge
    // ========================================================================

    bool loadCartridge(const std::filesystem::path& path) override {
        auto& cart = getCartridgeManager();
        if (!cart.load(path)) {
            m_lastError = cart.getLastError();
            return false;
        }
        // Reset after loading cartridge (many games expect this)
        reset();
        return true;
    }

    void ejectCartridge() override {
        auto& cart = getCartridgeManager();
        cart.eject();
    }

    bool hasCartridge() const override {
        return getCartridgeManager().hasCartridge();
    }

    std::string getCartridgeName() const override {
        return getCartridgeManager().getName();
    }

    // ========================================================================
    // Configuration & State
    // ========================================================================

    CpuType getCpuType() const override {
        return m_cpuType;
    }

    void setCpuType(CpuType type) override {
        if (type == m_cpuType) {
            return;
        }

        m_cpuType = type;

        // If running, switch CPU execution function
        if (m_ready) {
            if (type == CpuType::HD6309) {
                HD6309Init();
                ::CPUExec = HD6309Exec;
            } else {
                MC6809Init();
                ::CPUExec = MC6809Exec;
            }
        }
    }

    MemorySize getMemorySize() const override {
        return m_config.memorySize;
    }

    bool isReady() const override {
        return m_ready;
    }

    std::string getLastError() const override {
        return m_lastError;
    }

private:
    EmulatorConfig m_config;
    FrameBuffer m_framebuffer;
    unsigned char* m_memory = nullptr;
    CpuType m_cpuType = CpuType::MC6809;
    bool m_ready = false;
    std::string m_lastError;

    // Audio samples converted from legacy buffer (16-bit mono)
    std::vector<int16_t> m_audioSamples;
};

// Factory function
std::unique_ptr<CocoEmulator> CocoEmulator::create(const EmulatorConfig& config) {
    return std::make_unique<CocoEmulatorImpl>(config);
}

} // namespace cutie
