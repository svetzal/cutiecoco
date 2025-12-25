#ifndef CUTIE_EMULATOR_H
#define CUTIE_EMULATOR_H
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

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

namespace cutie {

// Forward declarations for interface types
class IVideoOutput;
class IAudioOutput;
class IInputProvider;
class ICartridge;

/**
 * @brief Memory size options for CoCo 3 RAM
 */
enum class MemorySize {
    _128K,   // Base CoCo 3 memory
    _512K,   // Common expansion
    _2M,     // Extended memory
    _8M      // Maximum supported
};

/**
 * @brief CPU type selection
 */
enum class CpuType {
    MC6809,  // Motorola 6809 (standard)
    HD6309   // Hitachi 6309 (enhanced)
};

/**
 * @brief Emulator configuration options
 */
struct EmulatorConfig {
    MemorySize memorySize = MemorySize::_512K;
    CpuType cpuType = CpuType::MC6809;
    std::filesystem::path systemRomPath;
    uint32_t audioSampleRate = 44100;
};

/**
 * @brief Frame buffer dimensions and format info
 */
struct FrameBufferInfo {
    int width = 640;
    int height = 480;
    int pitch = 640;  // Pixels per row (may include padding)

    // Buffer size in bytes (RGBA format)
    size_t sizeBytes() const { return static_cast<size_t>(pitch) * height * 4; }
};

/**
 * @brief Audio buffer information
 */
struct AudioInfo {
    uint32_t sampleRate = 44100;
    int channels = 1;  // CoCo is mono
    int bitsPerSample = 16;

    // Samples per frame at ~60 Hz
    size_t samplesPerFrame() const { return sampleRate / 60; }
};

/**
 * @brief CoCo 3 Emulator - Public API
 *
 * This class provides a clean, platform-independent interface to the
 * CutieCoCo emulation core. It hides all internal implementation details
 * and provides methods for:
 * - Initialization and configuration
 * - Frame-by-frame execution
 * - Input handling (keyboard, joystick)
 * - Video output (framebuffer access)
 * - Audio output (sample buffer access)
 *
 * Usage:
 * @code
 * cutie::EmulatorConfig config;
 * config.systemRomPath = "/path/to/roms";
 *
 * auto emulator = cutie::CocoEmulator::create(config);
 * if (!emulator->init()) {
 *     // Handle error
 * }
 *
 * while (running) {
 *     emulator->runFrame();
 *
 *     // Get video output
 *     auto fb = emulator->getFramebuffer();
 *
 *     // Get audio samples
 *     auto audio = emulator->getAudioSamples();
 * }
 * @endcode
 */
class CocoEmulator {
public:
    virtual ~CocoEmulator() = default;

    // Non-copyable
    CocoEmulator(const CocoEmulator&) = delete;
    CocoEmulator& operator=(const CocoEmulator&) = delete;

    /**
     * @brief Create a new emulator instance
     * @param config Emulator configuration
     * @return Unique pointer to the emulator, or nullptr on failure
     */
    static std::unique_ptr<CocoEmulator> create(const EmulatorConfig& config = {});

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /**
     * @brief Initialize the emulator
     *
     * Allocates memory, loads system ROM, and prepares for execution.
     * Must be called before any other operations.
     *
     * @return true if initialization succeeded
     */
    virtual bool init() = 0;

    /**
     * @brief Reset the emulator to power-on state
     *
     * Resets CPU, clears RAM, reloads ROM vectors.
     * Does not reload ROM from disk.
     */
    virtual void reset() = 0;

    /**
     * @brief Shut down the emulator
     *
     * Releases all resources. The emulator cannot be used after this call
     * unless init() is called again.
     */
    virtual void shutdown() = 0;

    // ========================================================================
    // Execution
    // ========================================================================

    /**
     * @brief Execute one complete frame of emulation
     *
     * Runs CPU execution and video/audio generation for one video frame
     * (~16.7ms at 60 Hz). After this call, getFramebuffer() and
     * getAudioSamples() return the new frame's data.
     */
    virtual void runFrame() = 0;

    /**
     * @brief Execute a specified number of CPU cycles
     *
     * Lower-level interface for debugging or precise timing control.
     *
     * @param cycles Number of CPU cycles to execute
     * @return Actual number of cycles executed (may differ due to instruction boundaries)
     */
    virtual int runCycles(int cycles) = 0;

    // ========================================================================
    // Input
    // ========================================================================

    /**
     * @brief Set the state of a keyboard key
     *
     * @param row Keyboard matrix row (0-6)
     * @param col Keyboard matrix column (0-7)
     * @param pressed true if key is pressed, false if released
     */
    virtual void setKeyState(int row, int col, bool pressed) = 0;

    /**
     * @brief Set joystick axis value
     *
     * @param joystick Joystick number (0 or 1)
     * @param axis 0 for X axis, 1 for Y axis
     * @param value Axis value (0-63, where 32 is center)
     */
    virtual void setJoystickAxis(int joystick, int axis, int value) = 0;

    /**
     * @brief Set joystick button state
     *
     * @param joystick Joystick number (0 or 1)
     * @param button Button number (0 or 1)
     * @param pressed true if button is pressed
     */
    virtual void setJoystickButton(int joystick, int button, bool pressed) = 0;

    // ========================================================================
    // Video Output
    // ========================================================================

    /**
     * @brief Get information about the framebuffer format
     * @return FrameBufferInfo with dimensions and format
     */
    virtual FrameBufferInfo getFramebufferInfo() const = 0;

    /**
     * @brief Get the current framebuffer contents
     *
     * Returns a span of RGBA pixel data (8 bits per component).
     * The data remains valid until the next call to runFrame().
     *
     * @return Span of pixel data (width * height * 4 bytes)
     */
    virtual std::span<const uint8_t> getFramebuffer() const = 0;

    // ========================================================================
    // Audio Output
    // ========================================================================

    /**
     * @brief Get information about the audio format
     * @return AudioInfo with sample rate and format
     */
    virtual AudioInfo getAudioInfo() const = 0;

    /**
     * @brief Get audio samples from the last frame
     *
     * Returns 16-bit signed mono samples generated during the last frame.
     * The data remains valid until the next call to runFrame().
     *
     * @return Span of audio samples
     */
    virtual std::span<const int16_t> getAudioSamples() const = 0;

    // ========================================================================
    // Configuration & State
    // ========================================================================

    /**
     * @brief Get current CPU type
     */
    virtual CpuType getCpuType() const = 0;

    /**
     * @brief Set CPU type (requires reset to take effect)
     */
    virtual void setCpuType(CpuType type) = 0;

    /**
     * @brief Get current memory size
     */
    virtual MemorySize getMemorySize() const = 0;

    /**
     * @brief Check if emulator is initialized and ready
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Get last error message
     * @return Error string, or empty if no error
     */
    virtual std::string getLastError() const = 0;

protected:
    CocoEmulator() = default;
};

} // namespace cutie

#endif // CUTIE_EMULATOR_H
