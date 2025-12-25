#ifndef CUTIE_CARTRIDGE_H
#define CUTIE_CARTRIDGE_H
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
#include <filesystem>
#include <string>
#include <vector>
#include <mutex>

namespace cutie {

/**
 * @brief Simple ROM cartridge manager
 *
 * Manages a single ROM cartridge in the CoCo expansion slot.
 * The cartridge memory is mapped to $C000-$FEFF (16KB window).
 * Larger ROMs support bank switching via port writes.
 */
class CartridgeManager {
public:
    CartridgeManager();

    /**
     * @brief Load a ROM cartridge from file
     * @param path Path to the ROM file (.rom, .ccc, .pak)
     * @return true if loaded successfully
     */
    bool load(const std::filesystem::path& path);

    /**
     * @brief Eject the current cartridge
     */
    void eject();

    /**
     * @brief Check if a cartridge is loaded
     */
    bool hasCartridge() const;

    /**
     * @brief Get the name of the loaded cartridge
     * @return Filename without path, or empty if no cartridge
     */
    std::string getName() const;

    /**
     * @brief Read a byte from cartridge memory
     *
     * The address is in the $C000-$FEFF range (16KB window).
     * For ROMs larger than 16KB, bank switching determines which
     * portion of the ROM is visible.
     *
     * @param address Address in cartridge space (0x0000-0x3FFF mapped from $C000)
     * @return Byte value, or 0xFF if no cartridge
     */
    uint8_t read(uint16_t address) const;

    /**
     * @brief Write to cartridge port (bank switching)
     *
     * Writes to the $FF40-$FF5F port range control bank switching
     * for larger ROMs.
     *
     * @param port Port number (0-31, relative to $FF40)
     * @param value Value to write
     */
    void writePort(uint8_t port, uint8_t value);

    /**
     * @brief Read from cartridge port
     * @param port Port number (0-31, relative to $FF40)
     * @return Port value, or 0xFF if no cartridge
     */
    uint8_t readPort(uint8_t port) const;

    /**
     * @brief Reset cartridge state
     *
     * Resets bank selection to 0 but keeps ROM loaded.
     */
    void reset();

    /**
     * @brief Get last error message
     */
    std::string getLastError() const;

private:
    // ROM data
    std::vector<uint8_t> m_rom;

    // Cartridge name (filename)
    std::string m_name;

    // Bank selection for ROMs > 16KB
    // Each bank is 16KB, selected by upper bits of address
    uint8_t m_bankSelect = 0;

    // Error message
    std::string m_lastError;

    mutable std::mutex m_mutex;

    // Calculate offset into ROM based on address and bank
    size_t calculateOffset(uint16_t address) const;
};

/**
 * @brief Global cartridge manager instance
 */
CartridgeManager& getCartridgeManager();

} // namespace cutie

// C-compatible functions for legacy code (tcc1014mmu.cpp, mc6821.cpp)
extern "C" {
    /**
     * @brief Read from cartridge memory
     * @param address Address in cartridge space
     * @return Byte value
     */
    unsigned char vccCartridgeRead(unsigned short address);

    /**
     * @brief Write to cartridge port
     * @param port Port number
     * @param value Value to write
     */
    void vccCartridgeWritePort(unsigned char port, unsigned char value);

    /**
     * @brief Read from cartridge port
     * @param port Port number
     * @return Port value
     */
    unsigned char vccCartridgeReadPort(unsigned char port);

    /**
     * @brief Check if cartridge is inserted (for auto-start)
     * @return Non-zero if cartridge is loaded
     */
    unsigned char vccCartridgeIsInserted();
}

#endif // CUTIE_CARTRIDGE_H
