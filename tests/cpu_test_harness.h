/*
Copyright 2024-2025 CutieCoCo Contributors
This file is part of CutieCoCo.

CPU Test Harness - Provides a controlled environment for CPU instruction testing.
*/

#ifndef CUTIE_CPU_TEST_HARNESS_H
#define CUTIE_CPU_TEST_HARNESS_H

#include <cstdint>
#include <vector>
#include "cutie/compat.h"  // For VCC::CPUState

namespace cutie {
namespace test {

/**
 * @brief Test harness for 6809/6309 CPU instruction testing
 *
 * This class sets up a minimal emulation environment for testing
 * individual CPU instructions. It provides:
 * - 64KB flat memory model
 * - Direct memory read/write access
 * - CPU state inspection
 * - Single-step execution
 */
class CPUTestHarness {
public:
    CPUTestHarness();
    ~CPUTestHarness();

    /**
     * @brief Reset the CPU to initial state
     *
     * Resets the CPU and sets PC from reset vector ($FFFE-$FFFF)
     */
    void reset();

    /**
     * @brief Load a program into memory
     * @param address Starting address
     * @param program Byte vector containing the program
     */
    void loadProgram(uint16_t address, const std::vector<uint8_t>& program);

    /**
     * @brief Set the program counter
     *
     * Updates the reset vector and resets the CPU so PC starts at address.
     * Note: This clears flags but allows testing specific instructions.
     */
    void setPC(uint16_t address);

    /**
     * @brief Execute a number of CPU cycles
     * @param cycles Number of cycles to execute
     * @return Actual cycles executed
     */
    int execute(int cycles);

    /**
     * @brief Execute a single instruction
     * @return Cycles used by the instruction
     */
    int step();

    /**
     * @brief Get current CPU state
     * @return CPUState structure with register values
     */
    VCC::CPUState getState() const;

    /**
     * @brief Read a byte from memory
     */
    uint8_t readByte(uint16_t address) const;

    /**
     * @brief Read a 16-bit word from memory (big-endian)
     */
    uint16_t readWord(uint16_t address) const;

    /**
     * @brief Write a byte to memory
     */
    void writeByte(uint16_t address, uint8_t value);

    /**
     * @brief Write a 16-bit word to memory (big-endian)
     */
    void writeWord(uint16_t address, uint16_t value);

    // Register helpers - set registers before running test
    void setA(uint8_t value);
    void setB(uint8_t value);
    void setD(uint16_t value);
    void setX(uint16_t value);
    void setY(uint16_t value);
    void setU(uint16_t value);
    void setS(uint16_t value);

private:
    void setup();
    void teardown();
};

// Condition code bit positions
enum CCFlag {
    CC_C = 0x01,  // Carry
    CC_V = 0x02,  // Overflow
    CC_Z = 0x04,  // Zero
    CC_N = 0x08,  // Negative
    CC_I = 0x10,  // IRQ mask
    CC_H = 0x20,  // Half-carry
    CC_F = 0x40,  // FIRQ mask
    CC_E = 0x80   // Entire state saved
};

} // namespace test
} // namespace cutie

#endif // CUTIE_CPU_TEST_HARNESS_H
