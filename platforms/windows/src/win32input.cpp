// CutieCoCo - Windows Input Handling Implementation

#include "win32input.h"
#include "cutie/emulator.h"

namespace cutie {
namespace win32 {

std::optional<CocoKeyCombo> mapCharToCoco(wchar_t ch)
{
    using K = CocoKey;

    // Lowercase letters -> just the letter key
    if (ch >= L'a' && ch <= L'z') {
        return CocoKeyCombo{static_cast<K>(static_cast<int>(K::A) + (ch - L'a')), false};
    }

    // Uppercase letters -> letter key + shift
    if (ch >= L'A' && ch <= L'Z') {
        return CocoKeyCombo{static_cast<K>(static_cast<int>(K::A) + (ch - L'A')), true};
    }

    // Numbers
    if (ch >= L'0' && ch <= L'9') {
        return CocoKeyCombo{static_cast<K>(static_cast<int>(K::Key0) + (ch - L'0')), false};
    }

    // CoCo shifted number keys produce different symbols than PC
    switch (ch) {
        // Basic punctuation (unshifted on CoCo)
        case L'@': return CocoKeyCombo{K::At, false};
        case L':': return CocoKeyCombo{K::Colon, false};
        case L';': return CocoKeyCombo{K::Semicolon, false};
        case L',': return CocoKeyCombo{K::Comma, false};
        case L'-': return CocoKeyCombo{K::Minus, false};
        case L'.': return CocoKeyCombo{K::Period, false};
        case L'/': return CocoKeyCombo{K::Slash, false};
        case L' ': return CocoKeyCombo{K::Space, false};

        // Shifted punctuation on CoCo
        case L'!': return CocoKeyCombo{K::Key1, true};   // Shift+1
        case L'"': return CocoKeyCombo{K::Key2, true};   // Shift+2
        case L'#': return CocoKeyCombo{K::Key3, true};   // Shift+3
        case L'$': return CocoKeyCombo{K::Key4, true};   // Shift+4
        case L'%': return CocoKeyCombo{K::Key5, true};   // Shift+5
        case L'&': return CocoKeyCombo{K::Key6, true};   // Shift+6
        case L'\'': return CocoKeyCombo{K::Key7, true};  // Shift+7 (apostrophe)
        case L'(': return CocoKeyCombo{K::Key8, true};   // Shift+8
        case L')': return CocoKeyCombo{K::Key9, true};   // Shift+9
        case L'*': return CocoKeyCombo{K::Colon, true};  // Shift+:
        case L'+': return CocoKeyCombo{K::Semicolon, true}; // Shift+;
        case L'<': return CocoKeyCombo{K::Comma, true};  // Shift+,
        case L'=': return CocoKeyCombo{K::Minus, true};  // Shift+-
        case L'>': return CocoKeyCombo{K::Period, true}; // Shift+.
        case L'?': return CocoKeyCombo{K::Slash, true};  // Shift+/

        default: return std::nullopt;
    }
}

std::optional<CocoKey> mapVKToCoco(WPARAM vkCode)
{
    using K = CocoKey;

    switch (vkCode) {
        // Arrow keys
        case VK_UP: return K::Up;
        case VK_DOWN: return K::Down;
        case VK_LEFT: return K::Left;
        case VK_RIGHT: return K::Right;

        // Special keys
        case VK_RETURN: return K::Enter;
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT: return K::Shift;
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL: return K::Ctrl;
        case VK_MENU:  // Alt key
        case VK_LMENU:
        case VK_RMENU: return K::Alt;
        case VK_ESCAPE: return K::Break;
        case VK_BACK: return K::Left;  // Backspace acts as left arrow
        case VK_HOME: return K::Clear;

        // Function keys
        case VK_F1: return K::F1;
        case VK_F2: return K::F2;

        // Space bar (also handled via WM_CHAR)
        case VK_SPACE: return K::Space;

        default: return std::nullopt;
    }
}

Win32Input::Win32Input() = default;

bool Win32Input::handleKeyDown(WPARAM vkCode, LPARAM /*flags*/)
{
    // First check if this is a joystick key (numpad)
    if (handleJoystickKey(vkCode, true)) {
        return true;
    }

    // Try to map the virtual key to a CoCo key for non-printable keys
    auto cocoKey = mapVKToCoco(vkCode);
    if (cocoKey) {
        getKeyboard().keyDown(*cocoKey);
        return true;
    }

    // For printable characters, we'll handle them in handleChar()
    // via WM_CHAR messages which give us the actual character
    return false;
}

bool Win32Input::handleKeyUp(WPARAM vkCode, LPARAM /*flags*/)
{
    // First check if this is a joystick key (numpad)
    if (handleJoystickKey(vkCode, false)) {
        return true;
    }

    // Release any shift we added for this key
    if (vkCode < MAX_VK && m_addedShift[vkCode]) {
        getKeyboard().keyUp(CocoKey::Shift);
        m_addedShift[vkCode] = false;
    }

    // Try to map the virtual key to a CoCo key
    auto cocoKey = mapVKToCoco(vkCode);
    if (cocoKey) {
        getKeyboard().keyUp(*cocoKey);
        return true;
    }

    // For printable characters, we need to map the character back
    // Since WM_KEYUP doesn't give us the character, we handle common cases
    // based on virtual key code

    // Letters A-Z
    if (vkCode >= 'A' && vkCode <= 'Z') {
        auto key = static_cast<CocoKey>(static_cast<int>(CocoKey::A) + (vkCode - 'A'));
        getKeyboard().keyUp(key);
        return true;
    }

    // Numbers 0-9
    if (vkCode >= '0' && vkCode <= '9') {
        auto key = static_cast<CocoKey>(static_cast<int>(CocoKey::Key0) + (vkCode - '0'));
        getKeyboard().keyUp(key);
        return true;
    }

    // Common punctuation keys
    switch (vkCode) {
        case VK_OEM_1:     // ;: key
            getKeyboard().keyUp(CocoKey::Semicolon);
            return true;
        case VK_OEM_PLUS:  // =+ key
            getKeyboard().keyUp(CocoKey::Minus);  // = is Shift+- on CoCo
            return true;
        case VK_OEM_COMMA: // ,< key
            getKeyboard().keyUp(CocoKey::Comma);
            return true;
        case VK_OEM_MINUS: // -_ key
            getKeyboard().keyUp(CocoKey::Minus);
            return true;
        case VK_OEM_PERIOD: // .> key
            getKeyboard().keyUp(CocoKey::Period);
            return true;
        case VK_OEM_2:     // /? key
            getKeyboard().keyUp(CocoKey::Slash);
            return true;
        case VK_OEM_3:     // `~ key (maps to @)
            getKeyboard().keyUp(CocoKey::At);
            return true;
        case VK_OEM_7:     // '" key
            getKeyboard().keyUp(CocoKey::Key7);  // ' is Shift+7 on CoCo
            return true;
    }

    return false;
}

void Win32Input::handleChar(wchar_t ch)
{
    auto combo = mapCharToCoco(ch);
    if (!combo) {
        return;  // Unmapped character
    }

    // If this character needs shift, press shift first
    if (combo->withShift) {
        getKeyboard().keyDown(CocoKey::Shift);
        // Track that we added shift for the corresponding VK code
        // (We'll release it in handleKeyUp)
    }

    // Press the key
    getKeyboard().keyDown(combo->key);
}

void Win32Input::reset()
{
    getKeyboard().releaseAll();
    m_leftJoyKeys = JoystickKeyState{};

    for (int i = 0; i < MAX_VK; ++i) {
        m_addedShift[i] = false;
    }

    if (m_emulator) {
        updateJoystickFromKeys();
    }
}

void Win32Input::updateJoystickFromKeys()
{
    if (!m_emulator) return;

    // Calculate X axis (0=left, 32=center, 63=right)
    int x = AXIS_CENTER;
    if (m_leftJoyKeys.left && !m_leftJoyKeys.right) {
        x = AXIS_MIN;
    } else if (m_leftJoyKeys.right && !m_leftJoyKeys.left) {
        x = AXIS_MAX;
    }

    // Calculate Y axis (0=up, 32=center, 63=down)
    int y = AXIS_CENTER;
    if (m_leftJoyKeys.up && !m_leftJoyKeys.down) {
        y = AXIS_MIN;
    } else if (m_leftJoyKeys.down && !m_leftJoyKeys.up) {
        y = AXIS_MAX;
    }

    // Apply to left joystick
    m_emulator->setJoystickAxis(JOYSTICK_LEFT, AXIS_X, x);
    m_emulator->setJoystickAxis(JOYSTICK_LEFT, AXIS_Y, y);
    m_emulator->setJoystickButton(JOYSTICK_LEFT, BUTTON_1, m_leftJoyKeys.button1);
    m_emulator->setJoystickButton(JOYSTICK_LEFT, BUTTON_2, m_leftJoyKeys.button2);
}

bool Win32Input::handleJoystickKey(WPARAM vkCode, bool pressed)
{
    bool handled = true;

    switch (vkCode) {
        // Numpad directions (8=up, 2=down, 4=left, 6=right)
        case VK_NUMPAD8:
            m_leftJoyKeys.up = pressed;
            break;
        case VK_NUMPAD2:
            m_leftJoyKeys.down = pressed;
            break;
        case VK_NUMPAD4:
            m_leftJoyKeys.left = pressed;
            break;
        case VK_NUMPAD6:
            m_leftJoyKeys.right = pressed;
            break;

        // Diagonals
        case VK_NUMPAD7:
            m_leftJoyKeys.up = pressed;
            m_leftJoyKeys.left = pressed;
            break;
        case VK_NUMPAD9:
            m_leftJoyKeys.up = pressed;
            m_leftJoyKeys.right = pressed;
            break;
        case VK_NUMPAD1:
            m_leftJoyKeys.down = pressed;
            m_leftJoyKeys.left = pressed;
            break;
        case VK_NUMPAD3:
            m_leftJoyKeys.down = pressed;
            m_leftJoyKeys.right = pressed;
            break;

        // Buttons (0=button1, 5=button2)
        case VK_NUMPAD0:
            m_leftJoyKeys.button1 = pressed;
            break;
        case VK_NUMPAD5:
            m_leftJoyKeys.button2 = pressed;
            break;

        default:
            handled = false;
            break;
    }

    if (handled) {
        updateJoystickFromKeys();
    }

    return handled;
}

}  // namespace win32
}  // namespace cutie
