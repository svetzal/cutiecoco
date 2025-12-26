#ifndef CUTIE_KEYMAPPING_H
#define CUTIE_KEYMAPPING_H
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
#include <optional>

namespace cutie {

/**
 * @brief A CoCo key combination (primary key + optional shift)
 *
 * The CoCo keyboard has different shift mappings than a PC keyboard.
 * For example, " is Shift+2 on CoCo, not Shift+' like on a PC.
 * This struct represents both the base key and whether shift is needed.
 */
struct CocoKeyCombo {
    CocoKey key;
    bool withShift;
};

/**
 * @brief Map a printable character to a CoCo key combination
 *
 * This function translates characters to their CoCo keyboard equivalents,
 * handling the different shift mappings between PC and CoCo keyboards.
 *
 * Examples:
 * - 'a' -> Key A, no shift
 * - 'A' -> Key A, with shift
 * - '"' -> Key 2, with shift (not ' like on PC)
 * - '*' -> Key :, with shift
 *
 * @param ch Unicode code point to map (char32_t handles all characters)
 * @return The CoCo key combination, or std::nullopt if unmapped
 */
std::optional<CocoKeyCombo> mapCharToCoco(char32_t ch);

} // namespace cutie

#endif // CUTIE_KEYMAPPING_H
