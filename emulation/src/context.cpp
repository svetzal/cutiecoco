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

#include "cutie/context.h"
#include <cstdio>

namespace cutie {

// ============================================================================
// Singleton
// ============================================================================

EmulationContext& EmulationContext::instance() {
    static EmulationContext ctx;
    return ctx;
}

EmulationContext::EmulationContext() {
    // Default message handler prints to stderr
    m_messageHandler = [](const char* message, const char* title) {
        if (title && title[0]) {
            fprintf(stderr, "[%s] %s\n", title, message);
        } else {
            fprintf(stderr, "%s\n", message);
        }
    };
}

// ============================================================================
// Interface Accessors
// ============================================================================

IVideoOutput* EmulationContext::videoOutput() {
    return m_videoOutput ? m_videoOutput : &m_nullVideo;
}

IAudioOutput* EmulationContext::audioOutput() {
    return m_audioOutput ? m_audioOutput : &m_nullAudio;
}

IInputProvider* EmulationContext::inputProvider() {
    return m_inputProvider ? m_inputProvider : &m_nullInput;
}

ICartridge* EmulationContext::cartridge() {
    return m_cartridge ? m_cartridge : &m_nullCartridge;
}

// ============================================================================
// Interface Setters
// ============================================================================

void EmulationContext::setVideoOutput(IVideoOutput* output) {
    m_videoOutput = output;
}

void EmulationContext::setAudioOutput(IAudioOutput* output) {
    m_audioOutput = output;
}

void EmulationContext::setInputProvider(IInputProvider* input) {
    m_inputProvider = input;
}

void EmulationContext::setCartridge(ICartridge* cart) {
    m_cartridge = cart;
}

// ============================================================================
// Configuration
// ============================================================================

void EmulationContext::setSystemRomPath(const std::filesystem::path& path) {
    m_systemRomPath = path;
}

std::filesystem::path EmulationContext::systemRomPath() const {
    if (!m_systemRomPath.empty()) {
        return m_systemRomPath;
    }
    // Fallback to current working directory
    return std::filesystem::current_path() / "system-roms";
}

void EmulationContext::setUseCustomSystemRom(bool use) {
    m_useCustomSystemRom = use;
}

bool EmulationContext::useCustomSystemRom() const {
    return m_useCustomSystemRom;
}

void EmulationContext::setCustomSystemRomPath(const std::filesystem::path& path) {
    m_customSystemRomPath = path;
}

std::filesystem::path EmulationContext::customSystemRomPath() const {
    return m_customSystemRomPath;
}

void EmulationContext::setMessageHandler(std::function<void(const char*, const char*)> handler) {
    if (handler) {
        m_messageHandler = handler;
    } else {
        // Reset to default
        m_messageHandler = [](const char* message, const char* title) {
            if (title && title[0]) {
                fprintf(stderr, "[%s] %s\n", title, message);
            } else {
                fprintf(stderr, "%s\n", message);
            }
        };
    }
}

void EmulationContext::showMessage(const char* message, const char* title) {
    if (m_messageHandler) {
        m_messageHandler(message, title ? title : "");
    }
}

// ============================================================================
// Reset
// ============================================================================

void EmulationContext::reset() {
    m_videoOutput = nullptr;
    m_audioOutput = nullptr;
    m_inputProvider = nullptr;
    m_cartridge = nullptr;
}

} // namespace cutie

// ============================================================================
// C-compatible wrapper functions
// ============================================================================

extern "C" {

int vccContextGetAudioFreeBlocks() {
    // The audio stretching code in coco3.cpp checks if there's buffer space
    // to slightly expand audio samples when running low. We return a value > 1
    // to allow this behavior, but the actual audio output is handled by the
    // platform through getAudioSamples().
    //
    // If a real audio output is set, we could query it, but for the pull-based
    // model we use, this isn't necessary.
    auto* audio = cutie::EmulationContext::instance().audioOutput();
    if (audio) {
        // Return based on queued samples - fewer queued = more free
        size_t queued = audio->getQueuedSampleCount();
        // Assume ~4000 samples per "block" (about 90ms at 44100Hz)
        // If less than 50% queued, return > 1 to enable stretching
        return queued < 2000 ? 4 : 1;
    }
    // Default: pretend we have buffer space
    return 4;
}

void vccContextShowMessage(const char* message, const char* title) {
    cutie::EmulationContext::instance().showMessage(message, title);
}

// Static buffer for C-string return
static std::string s_systemRomPathBuffer;

const char* vccContextGetSystemRomPath() {
    s_systemRomPathBuffer = cutie::EmulationContext::instance().systemRomPath().string();
    return s_systemRomPathBuffer.c_str();
}

} // extern "C"
