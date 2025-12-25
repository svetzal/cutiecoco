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

#include "cutie/cartridge.h"
#include "mc6821.h"
#include <fstream>
#include <cstdio>

namespace cutie {

CartridgeManager::CartridgeManager()
{
}

bool CartridgeManager::load(const std::filesystem::path& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        m_lastError = "File not found: " + path.string();
        return false;
    }

    // Get file size
    auto fileSize = std::filesystem::file_size(path);
    if (fileSize == 0) {
        m_lastError = "Empty file: " + path.string();
        return false;
    }

    // CoCo ROM paks are typically 8KB, 16KB, or multiples for bank-switched
    // Maximum reasonable size is 512KB (32 banks of 16KB)
    constexpr size_t MAX_ROM_SIZE = 512 * 1024;
    if (fileSize > MAX_ROM_SIZE) {
        m_lastError = "ROM file too large (max 512KB)";
        return false;
    }

    // Open and read the file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        m_lastError = "Failed to open file: " + path.string();
        return false;
    }

    // Read ROM data
    std::vector<uint8_t> romData(fileSize);
    file.read(reinterpret_cast<char*>(romData.data()), fileSize);
    if (!file) {
        m_lastError = "Failed to read file: " + path.string();
        return false;
    }

    // Success - store the ROM
    m_rom = std::move(romData);
    m_name = path.filename().string();
    m_bankSelect = 0;
    m_lastError.clear();

    fprintf(stderr, "Loaded cartridge: %s (%zu bytes)\n",
            m_name.c_str(), m_rom.size());

    // Notify PIA that a cartridge is inserted (for auto-start)
    SetCart(true);

    return true;
}

void CartridgeManager::eject()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rom.clear();
    m_name.clear();
    m_bankSelect = 0;
    m_lastError.clear();

    // Notify PIA that cartridge is removed
    SetCart(false);
}

bool CartridgeManager::hasCartridge() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_rom.empty();
}

std::string CartridgeManager::getName() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_name;
}

size_t CartridgeManager::calculateOffset(uint16_t address) const
{
    // Address is 0x0000-0x3FFF (16KB window from $C000-$FEFF)
    // For ROMs <= 16KB, just use the address directly
    // For larger ROMs, add bank offset

    if (m_rom.size() <= 0x4000) {
        // Small ROM - no banking
        return address % m_rom.size();
    }

    // Bank-switched ROM
    // First 16KB (bank 0) is always at the beginning
    // Additional banks are selected by m_bankSelect
    if (address < 0x2000) {
        // Lower 8KB - always from bank 0 (some ROMs use this pattern)
        return address;
    } else {
        // Upper 8KB - from selected bank
        size_t bankOffset = static_cast<size_t>(m_bankSelect) * 0x4000;
        return bankOffset + address;
    }
}

uint8_t CartridgeManager::read(uint16_t address) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_rom.empty()) {
        return 0xFF;  // No cartridge - open bus
    }

    // The MMU passes addresses in the range 0x0000-0x7FFF for cartridge reads
    // (offset + bank within 32KB cartridge space)
    // Mask to 32KB as the original VCC code does
    address &= 0x7FFF;

    // For simple ROMs, just read directly
    // The address already incorporates the bank offset from the MMU
    if (address >= m_rom.size()) {
        // For ROMs smaller than 32KB, mirror the content
        if (m_rom.size() > 0) {
            return m_rom[address % m_rom.size()];
        }
        return 0xFF;
    }

    return m_rom[address];
}

void CartridgeManager::writePort(uint8_t port, uint8_t value)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Bank switching is typically done via port $FF40
    // Different ROMs use different bank switching schemes
    // Common scheme: write bank number to port 0 ($FF40)
    if (port == 0) {
        m_bankSelect = value;
    }
}

uint8_t CartridgeManager::readPort(uint8_t port) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Most cartridges return 0xFF for port reads
    (void)port;
    return 0xFF;
}

void CartridgeManager::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bankSelect = 0;
}

std::string CartridgeManager::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

CartridgeManager& getCartridgeManager()
{
    static CartridgeManager manager;
    return manager;
}

} // namespace cutie

// C-compatible functions for legacy code

unsigned char vccCartridgeRead(unsigned short address)
{
    return cutie::getCartridgeManager().read(address);
}

void vccCartridgeWritePort(unsigned char port, unsigned char value)
{
    cutie::getCartridgeManager().writePort(port, value);
}

unsigned char vccCartridgeReadPort(unsigned char port)
{
    return cutie::getCartridgeManager().readPort(port);
}

unsigned char vccCartridgeIsInserted()
{
    return cutie::getCartridgeManager().hasCartridge() ? 1 : 0;
}
