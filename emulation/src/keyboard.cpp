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

#include "cutie/keyboard.h"
#include <algorithm>

namespace cutie {

Keyboard::Keyboard()
{
    m_matrix.fill(0);
}

void Keyboard::getRowCol(CocoKey key, uint8_t& row, uint8_t& col)
{
    // The keys are arranged in order: row 0 cols 0-7, row 1 cols 0-7, etc.
    uint8_t keyIndex = static_cast<uint8_t>(key);
    row = keyIndex / 8;
    col = keyIndex % 8;
}

void Keyboard::keyDown(CocoKey key)
{
    if (key >= CocoKey::Count) return;

    uint8_t row, col;
    getRowCol(key, row, col);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrix[row] |= (1 << col);
}

void Keyboard::keyUp(CocoKey key)
{
    if (key >= CocoKey::Count) return;

    uint8_t row, col;
    getRowCol(key, row, col);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrix[row] &= ~(1 << col);
}

void Keyboard::releaseAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrix.fill(0);
}

uint8_t Keyboard::scan(uint8_t colMask) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // CoCo keyboard matrix wiring:
    // - PB0-PB7 ($FF02) = Column strobe outputs (active low)
    // - PA0-PA6 ($FF00) = Row return inputs (active low)
    //
    // colMask: bits that are 0 indicate selected columns
    // Return: row bits (0 = row has key pressed in selected column)
    //
    // We store keys as m_matrix[row] with column bits set.
    // When a column is selected, we need to return which rows
    // have that column pressed - this is a matrix transpose.

    uint8_t result = 0;

    for (uint8_t col = 0; col < 8; ++col) {
        // Check if this column is selected (bit is 0 in mask)
        if ((colMask & (1 << col)) == 0) {
            // Find all rows with this column pressed
            for (uint8_t row = 0; row < 7; ++row) {
                if (m_matrix[row] & (1 << col)) {
                    result |= (1 << row);
                }
            }
        }
    }

    // Return inverted (active-low): 0 = row has key, 1 = row empty
    return ~result;
}

bool Keyboard::isPressed(CocoKey key) const
{
    if (key >= CocoKey::Count) return false;

    uint8_t row, col;
    getRowCol(key, row, col);

    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_matrix[row] & (1 << col)) != 0;
}

Keyboard& getKeyboard()
{
    static Keyboard keyboard;
    return keyboard;
}

} // namespace cutie

// C-compatible function for mc6821.cpp
unsigned char vccKeyboardGetScan(unsigned char colMask)
{
    return cutie::getKeyboard().scan(colMask);
}
