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

#include "cutie/keymapping.h"

namespace cutie {

std::optional<CocoKeyCombo> mapCharToCoco(char32_t ch)
{
    using K = CocoKey;

    // Lowercase letters -> just the letter key
    if (ch >= U'a' && ch <= U'z') {
        return CocoKeyCombo{static_cast<K>(static_cast<int>(K::A) + (ch - U'a')), false};
    }

    // Uppercase letters -> letter key + shift
    if (ch >= U'A' && ch <= U'Z') {
        return CocoKeyCombo{static_cast<K>(static_cast<int>(K::A) + (ch - U'A')), true};
    }

    // Numbers
    if (ch >= U'0' && ch <= U'9') {
        return CocoKeyCombo{static_cast<K>(static_cast<int>(K::Key0) + (ch - U'0')), false};
    }

    // CoCo shifted number keys produce different symbols than PC
    switch (ch) {
        // Basic punctuation (unshifted on CoCo)
        case U'@': return CocoKeyCombo{K::At, false};
        case U':': return CocoKeyCombo{K::Colon, false};
        case U';': return CocoKeyCombo{K::Semicolon, false};
        case U',': return CocoKeyCombo{K::Comma, false};
        case U'-': return CocoKeyCombo{K::Minus, false};
        case U'.': return CocoKeyCombo{K::Period, false};
        case U'/': return CocoKeyCombo{K::Slash, false};
        case U' ': return CocoKeyCombo{K::Space, false};

        // Shifted punctuation on CoCo
        case U'!': return CocoKeyCombo{K::Key1, true};   // Shift+1
        case U'"': return CocoKeyCombo{K::Key2, true};   // Shift+2
        case U'#': return CocoKeyCombo{K::Key3, true};   // Shift+3
        case U'$': return CocoKeyCombo{K::Key4, true};   // Shift+4
        case U'%': return CocoKeyCombo{K::Key5, true};   // Shift+5
        case U'&': return CocoKeyCombo{K::Key6, true};   // Shift+6
        case U'\'': return CocoKeyCombo{K::Key7, true};  // Shift+7 (apostrophe)
        case U'(': return CocoKeyCombo{K::Key8, true};   // Shift+8
        case U')': return CocoKeyCombo{K::Key9, true};   // Shift+9
        case U'*': return CocoKeyCombo{K::Colon, true};  // Shift+:
        case U'+': return CocoKeyCombo{K::Semicolon, true}; // Shift+;
        case U'<': return CocoKeyCombo{K::Comma, true};  // Shift+,
        case U'=': return CocoKeyCombo{K::Minus, true};  // Shift+-
        case U'>': return CocoKeyCombo{K::Period, true}; // Shift+.
        case U'?': return CocoKeyCombo{K::Slash, true};  // Shift+/

        default: return std::nullopt;
    }
}

} // namespace cutie
