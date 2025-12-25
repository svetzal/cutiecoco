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

#include "cutie/joystick.h"
#include <algorithm>

namespace cutie {

Joystick::Joystick()
{
    // Initialize all axes to center (32)
    for (auto& stick : m_axes) {
        for (auto& axis : stick) {
            axis = AXIS_CENTER;
        }
    }

    // Initialize all buttons to not pressed
    for (auto& stick : m_buttons) {
        for (auto& button : stick) {
            button = false;
        }
    }
}

void Joystick::setAxis(int joystick, int axis, int value)
{
    if (joystick < 0 || joystick >= JOYSTICK_COUNT) return;
    if (axis < 0 || axis >= AXIS_COUNT) return;

    // Clamp value to valid range
    value = std::clamp(value, AXIS_MIN, AXIS_MAX);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_axes[joystick][axis] = value;
}

int Joystick::getAxis(int joystick, int axis) const
{
    if (joystick < 0 || joystick >= JOYSTICK_COUNT) return AXIS_CENTER;
    if (axis < 0 || axis >= AXIS_COUNT) return AXIS_CENTER;

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_axes[joystick][axis];
}

void Joystick::setButton(int joystick, int button, bool pressed)
{
    if (joystick < 0 || joystick >= JOYSTICK_COUNT) return;
    if (button < 0 || button >= BUTTON_COUNT) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_buttons[joystick][button] = pressed;
}

bool Joystick::getButton(int joystick, int button) const
{
    if (joystick < 0 || joystick >= JOYSTICK_COUNT) return false;
    if (button < 0 || button >= BUTTON_COUNT) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_buttons[joystick][button];
}

uint8_t Joystick::getButtonBits() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Build the button bits for PIA $FF00:
    // Bit 0: Right joystick button 1
    // Bit 1: Left joystick button 1
    // Bit 2: Right joystick button 2
    // Bit 3: Left joystick button 2
    // Bits are active-low (0 = pressed)

    uint8_t bits = 0x0F;  // All buttons released (high)

    if (m_buttons[JOYSTICK_RIGHT][BUTTON_1]) bits &= ~0x01;  // Clear bit 0
    if (m_buttons[JOYSTICK_LEFT][BUTTON_1])  bits &= ~0x02;  // Clear bit 1
    if (m_buttons[JOYSTICK_RIGHT][BUTTON_2]) bits &= ~0x04;  // Clear bit 2
    if (m_buttons[JOYSTICK_LEFT][BUTTON_2])  bits &= ~0x08;  // Clear bit 3

    return bits;
}

void Joystick::startRamp(uint8_t dacValue)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // The DAC value from $FF20 is in bits 7-2, giving us 6 bits
    // We shift right by 2 to get the 0-63 range
    m_dacValue = dacValue >> 2;
}

bool Joystick::getComparisonResult(int muxState) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // MUX state determines which analog input is being read:
    // 0 = Right joystick X
    // 1 = Right joystick Y
    // 2 = Left joystick X
    // 3 = Left joystick Y

    int joystick, axis;
    switch (muxState) {
        case 0:
            joystick = JOYSTICK_RIGHT;
            axis = AXIS_X;
            break;
        case 1:
            joystick = JOYSTICK_RIGHT;
            axis = AXIS_Y;
            break;
        case 2:
            joystick = JOYSTICK_LEFT;
            axis = AXIS_X;
            break;
        case 3:
            joystick = JOYSTICK_LEFT;
            axis = AXIS_Y;
            break;
        default:
            return false;  // Invalid mux state
    }

    int potValue = m_axes[joystick][axis];

    // The comparison works like this:
    // - The DAC outputs a voltage based on m_dacValue
    // - The joystick potentiometer outputs a voltage based on position
    // - The comparator returns 1 if DAC voltage > pot voltage
    // - Since higher DAC value = higher voltage, and higher pot = higher voltage:
    //   Return true (bit 7 high) when m_dacValue > potValue
    return m_dacValue > potValue;
}

void Joystick::centerAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& stick : m_axes) {
        for (auto& axis : stick) {
            axis = AXIS_CENTER;
        }
    }
}

Joystick& getJoystick()
{
    static Joystick joystick;
    return joystick;
}

} // namespace cutie

// C-compatible functions for mc6821.cpp

unsigned char vccJoystickGetButtonBits()
{
    return cutie::getJoystick().getButtonBits();
}

void vccJoystickStartRamp(unsigned char dacValue)
{
    cutie::getJoystick().startRamp(dacValue);
}

unsigned char vccJoystickGetComparison(unsigned char muxState)
{
    return cutie::getJoystick().getComparisonResult(muxState) ? 0x80 : 0x00;
}
