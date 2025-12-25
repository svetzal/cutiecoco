/*
Copyright 2024 DREAM-VCC Contributors
This file is part of DREAM-VCC.

    DREAM-VCC is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DREAM-VCC is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DREAM-VCC.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dream/keyboard.h"
#include <algorithm>

namespace dream {

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

uint8_t Keyboard::scan(uint8_t rowMask) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // rowMask is active-low: bits that are 0 indicate selected rows
    // We OR together the columns for all selected rows
    uint8_t result = 0;

    for (uint8_t row = 0; row < 7; ++row) {
        // Check if this row is selected (bit is 0 in mask)
        if ((rowMask & (1 << row)) == 0) {
            result |= m_matrix[row];
        }
    }

    return result;
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

} // namespace dream

// C-compatible function for mc6821.cpp
unsigned char vccKeyboardGetScan(unsigned char rowMask)
{
    return dream::getKeyboard().scan(rowMask);
}
