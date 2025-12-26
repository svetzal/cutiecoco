#ifndef CUTIE_JOYSTICK_H
#define CUTIE_JOYSTICK_H
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
#include <array>
#include <mutex>

namespace cutie {

/**
 * @brief Joystick index constants
 */
constexpr int JOYSTICK_LEFT = 0;
constexpr int JOYSTICK_RIGHT = 1;
constexpr int JOYSTICK_COUNT = 2;

/**
 * @brief Joystick axis constants
 */
constexpr int AXIS_X = 0;
constexpr int AXIS_Y = 1;
constexpr int AXIS_COUNT = 2;

/**
 * @brief Joystick axis value constants
 * The CoCo joystick uses 6-bit resolution (0-63)
 */
constexpr int AXIS_MIN = 0;
constexpr int AXIS_CENTER = 32;
constexpr int AXIS_MAX = 63;

/**
 * @brief Joystick button constants
 */
constexpr int BUTTON_1 = 0;  // Primary fire button
constexpr int BUTTON_2 = 1;  // Secondary button
constexpr int BUTTON_COUNT = 2;

/**
 * @brief CoCo 3 joystick input handler
 *
 * The CoCo joystick system uses an analog ramp-compare circuit:
 * 1. Software writes a 6-bit DAC value to $FF20 bits 7-2
 * 2. The hardware compares this to the joystick potentiometer
 * 3. Bit 7 of $FF00 returns the comparison result
 * 4. By binary searching, software determines the exact pot position
 *
 * Joystick buttons are wired to specific bits of $FF00:
 * - Bit 0: Right joystick button 1 (shared with keyboard row 1)
 * - Bit 1: Left joystick button 1 (shared with keyboard row 2)
 * - Bit 2: Right joystick button 2 (shared with keyboard row 3)
 * - Bit 3: Left joystick button 2 (shared with keyboard row 4)
 *
 * Thread-safe for use between Qt UI thread and emulation.
 */
class Joystick {
public:
    Joystick();

    /**
     * @brief Set a joystick axis value
     * @param joystick Joystick index (0=left, 1=right)
     * @param axis Axis index (0=X, 1=Y)
     * @param value Axis value (0-63, 32=center)
     */
    void setAxis(int joystick, int axis, int value);

    /**
     * @brief Get a joystick axis value
     * @param joystick Joystick index (0=left, 1=right)
     * @param axis Axis index (0=X, 1=Y)
     * @return Axis value (0-63, 32=center)
     */
    int getAxis(int joystick, int axis) const;

    /**
     * @brief Set a joystick button state
     * @param joystick Joystick index (0=left, 1=right)
     * @param button Button index (0=primary, 1=secondary)
     * @param pressed True if button is pressed
     */
    void setButton(int joystick, int button, bool pressed);

    /**
     * @brief Get a joystick button state
     * @param joystick Joystick index (0=left, 1=right)
     * @param button Button index (0=primary, 1=secondary)
     * @return True if button is pressed
     */
    bool getButton(int joystick, int button) const;

    /**
     * @brief Get joystick button bits for PIA $FF00 scan
     *
     * Returns the button states in the format expected by the PIA:
     * - Bit 0: Right joystick button 1
     * - Bit 1: Left joystick button 1
     * - Bit 2: Right joystick button 2
     * - Bit 3: Left joystick button 2
     *
     * Bits are active-low (0 = pressed).
     *
     * @return Button bits for PIA $FF00 bits 3-0
     */
    uint8_t getButtonBits() const;

    /**
     * @brief Start the analog ramp (Tandy mode)
     *
     * Called when software writes to $FF20. The DAC value determines
     * the ramp threshold for joystick comparison.
     *
     * @param dacValue The 6-bit DAC value (0-63)
     */
    void startRamp(uint8_t dacValue);

    /**
     * @brief Get the analog comparison result
     *
     * Returns the comparison between the DAC ramp and the current
     * joystick position based on the MUX state.
     *
     * @param muxState The current MUX state (0-3)
     * @return True if ramp exceeds joystick position (bit 7 of $FF00)
     */
    bool getComparisonResult(int muxState) const;

    /**
     * @brief Center all joysticks
     */
    void centerAll();

private:
    // Axis values for each joystick [joystick][axis]
    std::array<std::array<int, AXIS_COUNT>, JOYSTICK_COUNT> m_axes;

    // Button states for each joystick [joystick][button]
    std::array<std::array<bool, BUTTON_COUNT>, JOYSTICK_COUNT> m_buttons;

    // Current DAC ramp value (6-bit, 0-63)
    uint8_t m_dacValue = 0;

    mutable std::mutex m_mutex;
};

/**
 * @brief Global joystick instance
 *
 * Used by both the Qt UI thread (for input events) and the
 * emulation thread (for reading joystick state).
 */
Joystick& getJoystick();

} // namespace cutie

// C-compatible functions for legacy code (mc6821.cpp)
extern "C" {
    /**
     * @brief Get joystick button bits for PIA scan
     * @return Button bits (active-low) for bits 3-0 of $FF00
     */
    unsigned char vccJoystickGetButtonBits();

    /**
     * @brief Start joystick ramp (Tandy mode)
     * @param dacValue The DAC value from $FF20
     */
    void vccJoystickStartRamp(unsigned char dacValue);

    /**
     * @brief Get joystick analog comparison result
     * @param muxState The current MUX state
     * @return Non-zero if ramp exceeds joystick position
     */
    unsigned char vccJoystickGetComparison(unsigned char muxState);
}

#endif // CUTIE_JOYSTICK_H
