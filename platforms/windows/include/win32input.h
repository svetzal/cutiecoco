#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "cutie/keyboard.h"
#include "cutie/keymapping.h"
#include "cutie/joystick.h"

#include <optional>

namespace cutie {

class CocoEmulator;

namespace win32 {

// Map a Windows virtual key code to CoCo key for non-printable keys
std::optional<CocoKey> mapVKToCoco(WPARAM vkCode);

// Joystick key state tracking (numpad emulation)
struct JoystickKeyState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool button1 = false;
    bool button2 = false;
};

// Input handler for Windows platform
class Win32Input {
public:
    Win32Input();

    // Set emulator for joystick updates
    void setEmulator(CocoEmulator* emulator) { m_emulator = emulator; }

    // Handle WM_KEYDOWN - returns true if key was handled
    bool handleKeyDown(WPARAM vkCode, LPARAM flags);

    // Handle WM_KEYUP - returns true if key was handled
    bool handleKeyUp(WPARAM vkCode, LPARAM flags);

    // Handle WM_CHAR for character input
    void handleChar(wchar_t ch);

    // Reset all input state (e.g., when window loses focus)
    void reset();

private:
    // Update joystick from numpad key state
    void updateJoystickFromKeys();

    // Handle joystick numpad key - returns true if handled
    bool handleJoystickKey(WPARAM vkCode, bool pressed);

private:
    CocoEmulator* m_emulator = nullptr;
    JoystickKeyState m_leftJoyKeys;

    // Track keys that we added shift for (character-based mapping)
    // Key = virtual key code, Value = true if we added shift
    static constexpr int MAX_VK = 256;
    bool m_addedShift[MAX_VK] = {};
};

}  // namespace win32
}  // namespace cutie
