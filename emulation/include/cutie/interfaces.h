#ifndef CUTIE_INTERFACES_H
#define CUTIE_INTERFACES_H
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
#include <span>

namespace cutie {

// ============================================================================
// IVideoOutput - Abstract video output interface
// ============================================================================

/**
 * @brief Interface for video output
 *
 * The emulation library renders to an internal buffer and notifies the
 * platform via this interface when a frame is ready. Platforms implement
 * this to receive video output.
 */
class IVideoOutput {
public:
    virtual ~IVideoOutput() = default;

    /**
     * @brief Called when a new frame is ready
     *
     * @param pixels RGBA pixel data (8 bits per component)
     * @param width Frame width in pixels
     * @param height Frame height in pixels
     * @param pitch Row pitch in pixels (may include padding)
     */
    virtual void onFrame(const uint8_t* pixels, int width, int height, int pitch) = 0;

    /**
     * @brief Called when video mode changes
     *
     * The CoCo 3 GIME can change video modes. This notifies the platform
     * that the output dimensions or format have changed.
     *
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void onModeChange(int width, int height) = 0;
};

// ============================================================================
// IAudioOutput - Abstract audio output interface
// ============================================================================

/**
 * @brief Interface for audio output
 *
 * The emulation library generates audio samples and pushes them to the
 * platform via this interface. Platforms implement this to play audio.
 */
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    /**
     * @brief Initialize audio output
     *
     * @param sampleRate Desired sample rate in Hz (e.g., 44100)
     * @return true if initialization succeeded
     */
    virtual bool init(uint32_t sampleRate) = 0;

    /**
     * @brief Shutdown audio output
     */
    virtual void shutdown() = 0;

    /**
     * @brief Submit audio samples for playback
     *
     * @param samples 16-bit signed mono samples
     * @param count Number of samples
     */
    virtual void submitSamples(const int16_t* samples, size_t count) = 0;

    /**
     * @brief Get the number of samples that can be queued
     *
     * Returns how many samples the audio backend can accept without blocking.
     * Used for throttling the emulation to prevent buffer overflow.
     *
     * @return Number of samples that can be queued
     */
    virtual size_t getQueuedSampleCount() const = 0;

    /**
     * @brief Get the current sample rate
     */
    virtual uint32_t getSampleRate() const = 0;
};

// ============================================================================
// IInputProvider - Abstract input provider interface
// ============================================================================

/**
 * @brief Interface for input (keyboard and joystick)
 *
 * Platforms implement this to provide input to the emulator.
 * The emulator polls this interface during each frame.
 */
class IInputProvider {
public:
    virtual ~IInputProvider() = default;

    /**
     * @brief Scan the keyboard matrix
     *
     * Returns the state of the keyboard matrix for the specified column mask.
     * This matches the CoCo keyboard hardware behavior:
     * - colMask: bits that are 0 indicate selected columns
     * - Returns: row bits where 0 indicates a key is pressed
     *
     * @param colMask Column selection mask (active low)
     * @return Row bits for pressed keys (active low)
     */
    virtual uint8_t scanKeyboard(uint8_t colMask) const = 0;

    /**
     * @brief Get joystick axis value
     *
     * @param joystick Joystick number (0 or 1)
     * @param axis 0 for X axis, 1 for Y axis
     * @return Axis value (0-63, where 32 is center)
     */
    virtual int getJoystickAxis(int joystick, int axis) const = 0;

    /**
     * @brief Get joystick button state
     *
     * @param joystick Joystick number (0 or 1)
     * @param button Button number (0 or 1)
     * @return true if button is pressed
     */
    virtual bool getJoystickButton(int joystick, int button) const = 0;
};

// ============================================================================
// ICartridge - Abstract cartridge/pak interface
// ============================================================================

/**
 * @brief Interface for cartridge (ROM pak) support
 *
 * The CoCo supports plug-in ROM cartridges at $C000-$FEFF.
 * This interface allows platforms to provide cartridge implementations.
 */
class ICartridge {
public:
    virtual ~ICartridge() = default;

    /**
     * @brief Read a byte from cartridge memory
     *
     * @param address Address in CoCo address space ($C000-$FEFF)
     * @return Byte value, or 0xFF if no cartridge
     */
    virtual uint8_t read(uint16_t address) const = 0;

    /**
     * @brief Write a byte to cartridge memory
     *
     * Some cartridges have writable memory or registers.
     *
     * @param address Address in CoCo address space
     * @param value Byte value to write
     */
    virtual void write(uint16_t address, uint8_t value) = 0;

    /**
     * @brief Read from cartridge I/O port
     *
     * Some cartridges have I/O ports at $FF40-$FF5F.
     *
     * @param port Port number (0x00-0x1F, offset from $FF40)
     * @return Port value
     */
    virtual uint8_t readPort(uint8_t port) const = 0;

    /**
     * @brief Write to cartridge I/O port
     *
     * @param port Port number (0x00-0x1F, offset from $FF40)
     * @param value Value to write
     */
    virtual void writePort(uint8_t port, uint8_t value) = 0;

    /**
     * @brief Called once per frame for timing-based operations
     *
     * Some cartridges need periodic updates (e.g., real-time clock).
     */
    virtual void tick() = 0;

    /**
     * @brief Get cartridge name/description
     * @return Human-readable name
     */
    virtual const char* getName() const = 0;

    /**
     * @brief Check if cartridge has auto-start ROM
     * @return true if CART interrupt should be generated on reset
     */
    virtual bool hasAutoStart() const = 0;
};

// ============================================================================
// Null implementations for testing/default behavior
// ============================================================================

/**
 * @brief Null video output - discards all frames
 */
class NullVideoOutput : public IVideoOutput {
public:
    void onFrame(const uint8_t*, int, int, int) override {}
    void onModeChange(int, int) override {}
};

/**
 * @brief Null audio output - discards all samples
 */
class NullAudioOutput : public IAudioOutput {
public:
    bool init(uint32_t sampleRate) override {
        m_sampleRate = sampleRate;
        return true;
    }
    void shutdown() override {}
    void submitSamples(const int16_t*, size_t) override {}
    size_t getQueuedSampleCount() const override { return 0; }
    uint32_t getSampleRate() const override { return m_sampleRate; }

private:
    uint32_t m_sampleRate = 44100;
};

/**
 * @brief Null input provider - no input
 */
class NullInputProvider : public IInputProvider {
public:
    uint8_t scanKeyboard(uint8_t) const override { return 0xFF; }
    int getJoystickAxis(int, int) const override { return 32; }
    bool getJoystickButton(int, int) const override { return false; }
};

/**
 * @brief Null cartridge - empty slot
 */
class NullCartridge : public ICartridge {
public:
    uint8_t read(uint16_t) const override { return 0xFF; }
    void write(uint16_t, uint8_t) override {}
    uint8_t readPort(uint8_t) const override { return 0xFF; }
    void writePort(uint8_t, uint8_t) override {}
    void tick() override {}
    const char* getName() const override { return "Empty"; }
    bool hasAutoStart() const override { return false; }
};

} // namespace cutie

#endif // CUTIE_INTERFACES_H
