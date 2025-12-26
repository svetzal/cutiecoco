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

#ifndef CUTIE_CONTEXT_H
#define CUTIE_CONTEXT_H

#include "cutie/interfaces.h"
#include <cstdint>
#include <filesystem>
#include <functional>

namespace cutie {

/**
 * @brief Central context for emulation platform services
 *
 * EmulationContext provides a way for platform code to inject implementations
 * of the abstract interfaces (IVideoOutput, IAudioOutput, etc.). The emulation
 * code accesses these services through the context singleton.
 *
 * If no implementation is set, the context provides null/default implementations
 * that match the behavior of the inline stubs in stubs.h.
 *
 * Usage (platform code):
 * @code
 * // Set platform-specific audio implementation
 * EmulationContext::instance().setAudioOutput(&myAudioOutput);
 * @endcode
 *
 * Usage (emulation code):
 * @code
 * // Get audio output (never null - returns NullAudioOutput if none set)
 * auto* audio = EmulationContext::instance().audioOutput();
 * audio->submitSamples(samples, count);
 * @endcode
 */
class EmulationContext {
public:
    /**
     * @brief Get the singleton instance
     */
    static EmulationContext& instance();

    // ========================================================================
    // Interface Accessors (never return null - return Null* implementation if not set)
    // ========================================================================

    /**
     * @brief Get video output interface
     * @return Video output (NullVideoOutput if none set)
     */
    IVideoOutput* videoOutput();

    /**
     * @brief Get audio output interface
     * @return Audio output (NullAudioOutput if none set)
     */
    IAudioOutput* audioOutput();

    /**
     * @brief Get input provider interface
     * @return Input provider (NullInputProvider if none set)
     */
    IInputProvider* inputProvider();

    /**
     * @brief Get cartridge interface
     * @return Cartridge (NullCartridge if none set)
     */
    ICartridge* cartridge();

    // ========================================================================
    // Interface Setters (platform code uses these to inject implementations)
    // ========================================================================

    /**
     * @brief Set video output implementation
     * @param output Pointer to video output (nullptr to use null implementation)
     */
    void setVideoOutput(IVideoOutput* output);

    /**
     * @brief Set audio output implementation
     * @param output Pointer to audio output (nullptr to use null implementation)
     */
    void setAudioOutput(IAudioOutput* output);

    /**
     * @brief Set input provider implementation
     * @param input Pointer to input provider (nullptr to use null implementation)
     */
    void setInputProvider(IInputProvider* input);

    /**
     * @brief Set cartridge implementation
     * @param cart Pointer to cartridge (nullptr to use null implementation)
     */
    void setCartridge(ICartridge* cart);

    // ========================================================================
    // Configuration (simple values that don't need full interfaces)
    // ========================================================================

    /**
     * @brief Set path to system ROMs directory
     */
    void setSystemRomPath(const std::filesystem::path& path);

    /**
     * @brief Get path to system ROMs directory
     * @return Path to system-roms directory
     */
    std::filesystem::path systemRomPath() const;

    /**
     * @brief Set whether to use custom system ROM
     */
    void setUseCustomSystemRom(bool use);

    /**
     * @brief Check if using custom system ROM
     */
    bool useCustomSystemRom() const;

    /**
     * @brief Set custom system ROM path
     */
    void setCustomSystemRomPath(const std::filesystem::path& path);

    /**
     * @brief Get custom system ROM path
     */
    std::filesystem::path customSystemRomPath() const;

    /**
     * @brief Set message handler for MessageBox-style messages
     * @param handler Function that receives (message, title) and displays to user
     */
    void setMessageHandler(std::function<void(const char*, const char*)> handler);

    /**
     * @brief Display a message to the user
     * @param message Message text
     * @param title Window title (optional)
     */
    void showMessage(const char* message, const char* title = nullptr);

    // ========================================================================
    // Reset
    // ========================================================================

    /**
     * @brief Reset all interfaces to null implementations
     *
     * Call this when shutting down or reinitializing the emulator.
     */
    void reset();

private:
    EmulationContext();
    ~EmulationContext() = default;

    // Non-copyable
    EmulationContext(const EmulationContext&) = delete;
    EmulationContext& operator=(const EmulationContext&) = delete;

    // Interface pointers (null means use default Null* implementation)
    IVideoOutput* m_videoOutput = nullptr;
    IAudioOutput* m_audioOutput = nullptr;
    IInputProvider* m_inputProvider = nullptr;
    ICartridge* m_cartridge = nullptr;

    // Default null implementations
    NullVideoOutput m_nullVideo;
    NullAudioOutput m_nullAudio;
    NullInputProvider m_nullInput;
    NullCartridge m_nullCartridge;

    // Configuration
    std::filesystem::path m_systemRomPath;
    bool m_useCustomSystemRom = false;
    std::filesystem::path m_customSystemRomPath;

    // Message handler (default: prints to stderr)
    std::function<void(const char*, const char*)> m_messageHandler;
};

} // namespace cutie

// ============================================================================
// C-compatible wrapper functions for legacy code
// ============================================================================

extern "C" {

/**
 * @brief Get audio buffer free block count
 *
 * This is used by the audio stretching code in coco3.cpp to slightly
 * expand audio when the buffer is running low. Returns the number of
 * "blocks" that can be queued (each block is ~1ms of audio).
 */
int vccContextGetAudioFreeBlocks();

/**
 * @brief Display a message box to the user
 *
 * Platform should set a message handler via EmulationContext::setMessageHandler()
 * to display messages in a platform-appropriate way.
 */
void vccContextShowMessage(const char* message, const char* title);

/**
 * @brief Get system ROM path
 *
 * Returns the path to the system-roms directory. Platform should call
 * EmulationContext::setSystemRomPath() during initialization.
 */
const char* vccContextGetSystemRomPath();

} // extern "C"

#endif // CUTIE_CONTEXT_H
