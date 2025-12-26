#ifndef CUTIE_KEYBOARD_H
#define CUTIE_KEYBOARD_H
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
 * @brief CoCo keyboard matrix positions
 *
 * The CoCo 3 keyboard is a 7x8 matrix. Each key has a row (0-6) and
 * column (0-7) position. When the PIA reads the keyboard, it sets
 * output bits on port B to select rows, and reads input bits on port A
 * to detect which columns are pressed in those rows.
 */
enum class CocoKey : uint8_t {
    // Row 0
    At = 0,      // @
    A, B, C, D, E, F, G,

    // Row 1
    H, I, J, K, L, M, N, O,

    // Row 2
    P, Q, R, S, T, U, V, W,

    // Row 3
    X, Y, Z,
    Up, Down, Left, Right, Space,

    // Row 4
    Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7,

    // Row 5
    Key8, Key9, Colon, Semicolon, Comma, Minus, Period, Slash,

    // Row 6
    Enter, Clear, Break, Alt, Ctrl, F1, F2, Shift,

    Count  // Total number of keys
};

/**
 * @brief CoCo 3 keyboard matrix handler
 *
 * Maintains the state of all keys in the 7x8 keyboard matrix.
 * Thread-safe for use with emulation thread.
 */
class Keyboard {
public:
    Keyboard();

    /**
     * @brief Press a key
     * @param key The key to press
     */
    void keyDown(CocoKey key);

    /**
     * @brief Release a key
     * @param key The key to release
     */
    void keyUp(CocoKey key);

    /**
     * @brief Release all keys (e.g., when window loses focus)
     */
    void releaseAll();

    /**
     * @brief Scan the keyboard matrix
     *
     * This function is called by the PIA emulation. The CoCo keyboard is wired:
     * - PB0-PB7 ($FF02) = Column strobe outputs (active low)
     * - PA0-PA6 ($FF00) = Row return inputs (active low)
     *
     * The colMask parameter indicates which columns are being strobed
     * (active low - 0 bits indicate selected columns). Returns a byte
     * with bits cleared (0) for rows that have a key pressed in the
     * selected columns.
     *
     * @param colMask Column selection mask (inverted - 0 = selected)
     * @return Row bits for pressed keys in selected columns (inverted)
     */
    uint8_t scan(uint8_t colMask) const;

    /**
     * @brief Check if a specific key is pressed
     */
    bool isPressed(CocoKey key) const;

private:
    // Matrix state: 7 rows x 8 columns
    // Each byte represents a row, with bits indicating pressed columns
    std::array<uint8_t, 7> m_matrix;

    mutable std::mutex m_mutex;

    // Get row and column for a key
    static void getRowCol(CocoKey key, uint8_t& row, uint8_t& col);
};

/**
 * @brief Global keyboard instance
 *
 * Used by both the Qt UI thread (for key events) and the
 * emulation thread (for scanning).
 */
Keyboard& getKeyboard();

} // namespace cutie

// C-compatible function for legacy code (mc6821.cpp)
extern "C" {
    unsigned char vccKeyboardGetScan(unsigned char colMask);
}

#endif // CUTIE_KEYBOARD_H
