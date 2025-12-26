/*
Copyright 2024-2025 CutieCoCo Contributors
This file is part of CutieCoCo.

CPU Test Harness - Sets up the emulation environment for CPU instruction testing.
*/

#include "cpu_test_harness.h"
#include "tcc1014mmu.h"
#include "tcc1014registers.h"
#include "tcc1014graphics.h"
#include "mc6809.h"
#include "cutie/stubs.h"
#include <cstring>

namespace cutie {
namespace test {

// Static memory buffer for tests (64KB flat memory model)
static uint8_t* s_testMemory = nullptr;
static constexpr size_t TEST_MEMORY_SIZE = 65536;

// Flag to track if we're in test mode (bypasses normal MMU mapping)
static bool s_testMode = false;

CPUTestHarness::CPUTestHarness() {
    setup();
}

CPUTestHarness::~CPUTestHarness() {
    teardown();
}

void CPUTestHarness::setup() {
    // Initialize MMU with 512K (provides flat memory access)
    s_testMemory = MmuInit(1);  // 1 = 512K
    s_testMode = true;

    // Initialize GIME/SAM for proper memory access
    GimeInit();
    GimeReset();
    mc6883_reset();

    // Initialize CPU (does not reset - that would read from ROM vector)
    MC6809Init();
}

void CPUTestHarness::teardown() {
    s_testMode = false;
    // Memory is managed by the MMU, don't free it here
}

void CPUTestHarness::reset() {
    // Clear memory except for test program
    // Reset CPU to use reset vector
    MC6809Reset();
}

void CPUTestHarness::loadProgram(uint16_t address, const std::vector<uint8_t>& program) {
    for (size_t i = 0; i < program.size() && (address + i) < TEST_MEMORY_SIZE; ++i) {
        writeByte(address + static_cast<uint16_t>(i), program[i]);
    }
}

void CPUTestHarness::setPC(uint16_t address) {
    // Use MC6809ForcePC to directly set the program counter
    // This avoids the complexity of the reset vector in ROM space
    MC6809ForcePC(address);
}

int CPUTestHarness::execute(int cycles) {
    return MC6809Exec(cycles);
}

int CPUTestHarness::step() {
    // Execute one instruction
    // Use minimal cycles - MC6809Exec will execute at least one full instruction
    // if cycles > 0, even if it uses more cycles than requested
    return MC6809Exec(2);
}

VCC::CPUState CPUTestHarness::getState() const {
    return MC6809GetState();
}

uint8_t CPUTestHarness::readByte(uint16_t address) const {
    return MemRead8(address);
}

uint16_t CPUTestHarness::readWord(uint16_t address) const {
    return MemRead16(address);
}

void CPUTestHarness::writeByte(uint16_t address, uint8_t value) {
    MemWrite8(value, address);
}

void CPUTestHarness::writeWord(uint16_t address, uint16_t value) {
    MemWrite16(value, address);
}

void CPUTestHarness::setA(uint8_t value) {
    // Load A register using LDA immediate
    // We can't directly set registers, so we'll use a small program
    uint16_t tempAddr = 0x0000;  // Use zero page for temp code
    std::vector<uint8_t> code = {
        0x86, value,  // LDA #value
        0x12          // NOP (marker to stop)
    };
    loadProgram(tempAddr, code);
    setPC(tempAddr);
    step();  // Execute LDA
}

void CPUTestHarness::setB(uint8_t value) {
    uint16_t tempAddr = 0x0000;
    std::vector<uint8_t> code = {
        0xC6, value,  // LDB #value
        0x12          // NOP
    };
    loadProgram(tempAddr, code);
    setPC(tempAddr);
    step();
}

void CPUTestHarness::setD(uint16_t value) {
    uint16_t tempAddr = 0x0000;
    std::vector<uint8_t> code = {
        0xCC, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF),  // LDD #value
        0x12  // NOP
    };
    loadProgram(tempAddr, code);
    setPC(tempAddr);
    step();
}

void CPUTestHarness::setX(uint16_t value) {
    uint16_t tempAddr = 0x0000;
    std::vector<uint8_t> code = {
        0x8E, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF),  // LDX #value
        0x12  // NOP
    };
    loadProgram(tempAddr, code);
    setPC(tempAddr);
    step();
}

void CPUTestHarness::setY(uint16_t value) {
    uint16_t tempAddr = 0x0000;
    std::vector<uint8_t> code = {
        0x10, 0x8E, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF),  // LDY #value
        0x12  // NOP
    };
    loadProgram(tempAddr, code);
    setPC(tempAddr);
    step();
}

void CPUTestHarness::setU(uint16_t value) {
    uint16_t tempAddr = 0x0000;
    std::vector<uint8_t> code = {
        0xCE, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF),  // LDU #value
        0x12  // NOP
    };
    loadProgram(tempAddr, code);
    setPC(tempAddr);
    step();
}

void CPUTestHarness::setS(uint16_t value) {
    uint16_t tempAddr = 0x0000;
    std::vector<uint8_t> code = {
        0x10, 0xCE, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF),  // LDS #value
        0x12  // NOP
    };
    loadProgram(tempAddr, code);
    setPC(tempAddr);
    step();
}

} // namespace test
} // namespace cutie
