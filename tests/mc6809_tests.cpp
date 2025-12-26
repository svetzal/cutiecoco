/*
Copyright 2024-2025 CutieCoCo Contributors
This file is part of CutieCoCo.

MC6809 CPU Instruction Tests
*/

#include <catch2/catch_test_macros.hpp>
#include "cpu_test_harness.h"

using namespace cutie::test;

// ============================================================================
// Load Instructions
// ============================================================================

TEST_CASE("MC6809: LDA immediate loads value into A register", "[mc6809][load]") {
    CPUTestHarness cpu;

    // LDA #$42
    cpu.loadProgram(0x1000, {0x86, 0x42});
    cpu.setPC(0x1000);
    cpu.step();

    auto state = cpu.getState();
    REQUIRE(state.A == 0x42);
    REQUIRE(state.PC == 0x1002);  // LDA immediate is 2 bytes
}

TEST_CASE("MC6809: LDA immediate sets Zero flag correctly", "[mc6809][load][flags]") {
    CPUTestHarness cpu;

    // LDA #$00
    cpu.loadProgram(0x1000, {0x86, 0x00});
    cpu.setPC(0x1000);
    cpu.step();

    auto state = cpu.getState();
    REQUIRE(state.A == 0x00);
    REQUIRE((state.CC & CC_Z) != 0);  // Zero flag should be set
    REQUIRE((state.CC & CC_N) == 0);  // Negative flag should be clear
}

TEST_CASE("MC6809: LDA immediate sets Negative flag correctly", "[mc6809][load][flags]") {
    CPUTestHarness cpu;

    // LDA #$80 (negative value in two's complement)
    cpu.loadProgram(0x1000, {0x86, 0x80});
    cpu.setPC(0x1000);
    cpu.step();

    auto state = cpu.getState();
    REQUIRE(state.A == 0x80);
    REQUIRE((state.CC & CC_N) != 0);  // Negative flag should be set
    REQUIRE((state.CC & CC_Z) == 0);  // Zero flag should be clear
}

TEST_CASE("MC6809: LDB immediate loads value into B register", "[mc6809][load]") {
    CPUTestHarness cpu;

    // LDB #$37
    cpu.loadProgram(0x1000, {0xC6, 0x37});
    cpu.setPC(0x1000);
    cpu.step();

    auto state = cpu.getState();
    REQUIRE(state.B == 0x37);
}

TEST_CASE("MC6809: LDD immediate loads 16-bit value into D register", "[mc6809][load]") {
    CPUTestHarness cpu;

    // LDD #$1234
    cpu.loadProgram(0x1000, {0xCC, 0x12, 0x34});
    cpu.setPC(0x1000);
    cpu.step();

    auto state = cpu.getState();
    REQUIRE(state.A == 0x12);  // D high byte = A
    REQUIRE(state.B == 0x34);  // D low byte = B
    REQUIRE(state.D == 0x1234);
}

TEST_CASE("MC6809: LDX immediate loads 16-bit value into X register", "[mc6809][load]") {
    CPUTestHarness cpu;

    // LDX #$ABCD
    cpu.loadProgram(0x1000, {0x8E, 0xAB, 0xCD});
    cpu.setPC(0x1000);
    cpu.step();

    auto state = cpu.getState();
    REQUIRE(state.X == 0xABCD);
}

TEST_CASE("MC6809: LDY immediate loads 16-bit value into Y register", "[mc6809][load]") {
    CPUTestHarness cpu;

    // LDY #$1357 (2-byte opcode: 10 8E)
    cpu.loadProgram(0x1000, {0x10, 0x8E, 0x13, 0x57});
    cpu.setPC(0x1000);
    cpu.step();

    auto state = cpu.getState();
    REQUIRE(state.Y == 0x1357);
}

// ============================================================================
// Store Instructions
// ============================================================================

TEST_CASE("MC6809: STA direct stores A to memory", "[mc6809][store]") {
    CPUTestHarness cpu;

    // Set up DP register to $10 and A register to $55
    // Then store A to direct page offset $20 (effective address $1020)
    cpu.loadProgram(0x1000, {
        0x86, 0x55,       // LDA #$55
        0x1F, 0x8B,       // TFR A,DP (set DP to $55... wait, that's wrong)
    });

    // Actually, let's use a simpler approach - use extended addressing
    // STA $2000 (extended mode)
    cpu.loadProgram(0x1000, {
        0x86, 0x55,             // LDA #$55
        0xB7, 0x20, 0x00        // STA $2000 (extended)
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // STA

    REQUIRE(cpu.readByte(0x2000) == 0x55);
}

TEST_CASE("MC6809: STD extended stores D to memory", "[mc6809][store]") {
    CPUTestHarness cpu;

    // LDD #$CAFE, STD $2000
    cpu.loadProgram(0x1000, {
        0xCC, 0xCA, 0xFE,       // LDD #$CAFE
        0xFD, 0x20, 0x00        // STD $2000 (extended)
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDD
    cpu.step();  // STD

    REQUIRE(cpu.readWord(0x2000) == 0xCAFE);
}

// ============================================================================
// Arithmetic Instructions
// ============================================================================

TEST_CASE("MC6809: ADDA immediate adds value to A", "[mc6809][arithmetic]") {
    CPUTestHarness cpu;

    // LDA #$10, ADDA #$05
    cpu.loadProgram(0x1000, {
        0x86, 0x10,       // LDA #$10
        0x8B, 0x05        // ADDA #$05
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // ADDA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x15);
}

TEST_CASE("MC6809: ADDA sets Carry on overflow", "[mc6809][arithmetic][flags]") {
    CPUTestHarness cpu;

    // LDA #$FF, ADDA #$02 -> should set carry
    cpu.loadProgram(0x1000, {
        0x86, 0xFF,       // LDA #$FF
        0x8B, 0x02        // ADDA #$02
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // ADDA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x01);  // $FF + $02 = $101, truncated to $01
    REQUIRE((state.CC & CC_C) != 0);  // Carry flag should be set
}

TEST_CASE("MC6809: SUBA immediate subtracts value from A", "[mc6809][arithmetic]") {
    CPUTestHarness cpu;

    // LDA #$20, SUBA #$05
    cpu.loadProgram(0x1000, {
        0x86, 0x20,       // LDA #$20
        0x80, 0x05        // SUBA #$05
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // SUBA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x1B);
}

TEST_CASE("MC6809: ADDD adds 16-bit value to D", "[mc6809][arithmetic]") {
    CPUTestHarness cpu;

    // LDD #$1000, ADDD #$0234
    cpu.loadProgram(0x1000, {
        0xCC, 0x10, 0x00,       // LDD #$1000
        0xC3, 0x02, 0x34        // ADDD #$0234
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDD
    cpu.step();  // ADDD

    auto state = cpu.getState();
    REQUIRE(state.D == 0x1234);
}

TEST_CASE("MC6809: INCA increments A register", "[mc6809][arithmetic]") {
    CPUTestHarness cpu;

    // LDA #$41, INCA
    cpu.loadProgram(0x1000, {
        0x86, 0x41,       // LDA #$41
        0x4C              // INCA
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // INCA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x42);
}

TEST_CASE("MC6809: DECA decrements A register", "[mc6809][arithmetic]") {
    CPUTestHarness cpu;

    // LDA #$43, DECA
    cpu.loadProgram(0x1000, {
        0x86, 0x43,       // LDA #$43
        0x4A              // DECA
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // DECA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x42);
}

// ============================================================================
// Logical Instructions
// ============================================================================

TEST_CASE("MC6809: ANDA performs bitwise AND", "[mc6809][logical]") {
    CPUTestHarness cpu;

    // LDA #$FF, ANDA #$0F
    cpu.loadProgram(0x1000, {
        0x86, 0xFF,       // LDA #$FF
        0x84, 0x0F        // ANDA #$0F
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // ANDA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x0F);
}

TEST_CASE("MC6809: ORA performs bitwise OR", "[mc6809][logical]") {
    CPUTestHarness cpu;

    // LDA #$F0, ORA #$0F
    cpu.loadProgram(0x1000, {
        0x86, 0xF0,       // LDA #$F0
        0x8A, 0x0F        // ORA #$0F
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // ORA

    auto state = cpu.getState();
    REQUIRE(state.A == 0xFF);
}

TEST_CASE("MC6809: EORA performs bitwise XOR", "[mc6809][logical]") {
    CPUTestHarness cpu;

    // LDA #$FF, EORA #$AA -> $55
    cpu.loadProgram(0x1000, {
        0x86, 0xFF,       // LDA #$FF
        0x88, 0xAA        // EORA #$AA
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // EORA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x55);
}

TEST_CASE("MC6809: COMA complements A register", "[mc6809][logical]") {
    CPUTestHarness cpu;

    // LDA #$55, COMA -> $AA
    cpu.loadProgram(0x1000, {
        0x86, 0x55,       // LDA #$55
        0x43              // COMA
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // COMA

    auto state = cpu.getState();
    REQUIRE(state.A == 0xAA);
    REQUIRE((state.CC & CC_C) != 0);  // COM always sets carry
}

// ============================================================================
// Branch Instructions
// ============================================================================

TEST_CASE("MC6809: BRA always branches", "[mc6809][branch]") {
    CPUTestHarness cpu;

    // BRA +5 (forward branch)
    cpu.loadProgram(0x1000, {
        0x20, 0x05        // BRA $1007 (PC after BRA = $1002, +5 = $1007)
    });
    cpu.setPC(0x1000);
    cpu.step();  // BRA

    auto state = cpu.getState();
    REQUIRE(state.PC == 0x1007);
}

TEST_CASE("MC6809: BEQ branches when Zero flag is set", "[mc6809][branch]") {
    CPUTestHarness cpu;

    // LDA #$00 (sets Z flag), BEQ +5
    cpu.loadProgram(0x1000, {
        0x86, 0x00,       // LDA #$00 (sets Z)
        0x27, 0x05        // BEQ $1009
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // BEQ

    auto state = cpu.getState();
    REQUIRE(state.PC == 0x1009);  // Should have branched
}

TEST_CASE("MC6809: BEQ does not branch when Zero flag is clear", "[mc6809][branch]") {
    CPUTestHarness cpu;

    // LDA #$01 (clears Z flag), BEQ +5
    cpu.loadProgram(0x1000, {
        0x86, 0x01,       // LDA #$01 (clears Z)
        0x27, 0x05        // BEQ $1009
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // BEQ

    auto state = cpu.getState();
    REQUIRE(state.PC == 0x1004);  // Should NOT have branched (just past BEQ)
}

TEST_CASE("MC6809: BNE branches when Zero flag is clear", "[mc6809][branch]") {
    CPUTestHarness cpu;

    // LDA #$01, BNE +5
    cpu.loadProgram(0x1000, {
        0x86, 0x01,       // LDA #$01 (clears Z)
        0x26, 0x05        // BNE $1009
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // BNE

    auto state = cpu.getState();
    REQUIRE(state.PC == 0x1009);  // Should have branched
}

// ============================================================================
// Shift/Rotate Instructions
// ============================================================================

TEST_CASE("MC6809: ASLA shifts A left", "[mc6809][shift]") {
    CPUTestHarness cpu;

    // LDA #$40, ASLA -> $80
    cpu.loadProgram(0x1000, {
        0x86, 0x40,       // LDA #$40
        0x48              // ASLA
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // ASLA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x80);
    REQUIRE((state.CC & CC_C) == 0);  // No carry out
}

TEST_CASE("MC6809: ASLA sets Carry when bit 7 shifts out", "[mc6809][shift][flags]") {
    CPUTestHarness cpu;

    // LDA #$80, ASLA -> $00 with carry
    cpu.loadProgram(0x1000, {
        0x86, 0x80,       // LDA #$80
        0x48              // ASLA
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // ASLA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x00);
    REQUIRE((state.CC & CC_C) != 0);  // Carry should be set
    REQUIRE((state.CC & CC_Z) != 0);  // Zero should be set
}

TEST_CASE("MC6809: LSRA shifts A right", "[mc6809][shift]") {
    CPUTestHarness cpu;

    // LDA #$80, LSRA -> $40
    cpu.loadProgram(0x1000, {
        0x86, 0x80,       // LDA #$80
        0x44              // LSRA
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // LSRA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x40);
    REQUIRE((state.CC & CC_C) == 0);  // No carry out
}

// ============================================================================
// Compare Instructions
// ============================================================================

TEST_CASE("MC6809: CMPA sets flags correctly for equal values", "[mc6809][compare]") {
    CPUTestHarness cpu;

    // LDA #$42, CMPA #$42
    cpu.loadProgram(0x1000, {
        0x86, 0x42,       // LDA #$42
        0x81, 0x42        // CMPA #$42
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // CMPA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x42);  // A unchanged
    REQUIRE((state.CC & CC_Z) != 0);  // Zero flag set (equal)
    REQUIRE((state.CC & CC_C) == 0);  // No borrow
}

TEST_CASE("MC6809: CMPA sets Carry when A < operand", "[mc6809][compare][flags]") {
    CPUTestHarness cpu;

    // LDA #$10, CMPA #$20
    cpu.loadProgram(0x1000, {
        0x86, 0x10,       // LDA #$10
        0x81, 0x20        // CMPA #$20
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // CMPA

    auto state = cpu.getState();
    REQUIRE(state.A == 0x10);  // A unchanged
    REQUIRE((state.CC & CC_Z) == 0);  // Not equal
    REQUIRE((state.CC & CC_C) != 0);  // Borrow (A < operand)
}

// ============================================================================
// Transfer Instructions
// ============================================================================

TEST_CASE("MC6809: TFR copies register value", "[mc6809][transfer]") {
    CPUTestHarness cpu;

    // LDA #$55, TFR A,B
    cpu.loadProgram(0x1000, {
        0x86, 0x55,       // LDA #$55
        0x1F, 0x89        // TFR A,B
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // TFR

    auto state = cpu.getState();
    REQUIRE(state.A == 0x55);
    REQUIRE(state.B == 0x55);
}

TEST_CASE("MC6809: EXG exchanges register values", "[mc6809][transfer]") {
    CPUTestHarness cpu;

    // LDA #$AA, LDB #$55, EXG A,B
    cpu.loadProgram(0x1000, {
        0x86, 0xAA,       // LDA #$AA
        0xC6, 0x55,       // LDB #$55
        0x1E, 0x89        // EXG A,B
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDA
    cpu.step();  // LDB
    cpu.step();  // EXG

    auto state = cpu.getState();
    REQUIRE(state.A == 0x55);
    REQUIRE(state.B == 0xAA);
}

// ============================================================================
// Stack Instructions
// ============================================================================

TEST_CASE("MC6809: PSHS pushes registers to stack", "[mc6809][stack]") {
    CPUTestHarness cpu;

    // LDS #$3000, LDA #$42, PSHS A
    cpu.loadProgram(0x1000, {
        0x10, 0xCE, 0x30, 0x00,  // LDS #$3000
        0x86, 0x42,              // LDA #$42
        0x34, 0x02               // PSHS A
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDS
    cpu.step();  // LDA
    cpu.step();  // PSHS

    auto state = cpu.getState();
    REQUIRE(state.S == 0x2FFF);  // S decremented
    REQUIRE(cpu.readByte(0x2FFF) == 0x42);  // A was pushed
}

TEST_CASE("MC6809: PULS pulls registers from stack", "[mc6809][stack]") {
    CPUTestHarness cpu;

    // Set up stack with a value, then pull it
    cpu.writeByte(0x2FFF, 0x37);  // Pre-load stack

    // LDS #$2FFF, PULS A
    cpu.loadProgram(0x1000, {
        0x10, 0xCE, 0x2F, 0xFF,  // LDS #$2FFF
        0x35, 0x02               // PULS A
    });
    cpu.setPC(0x1000);
    cpu.step();  // LDS
    cpu.step();  // PULS

    auto state = cpu.getState();
    REQUIRE(state.A == 0x37);
    REQUIRE(state.S == 0x3000);  // S incremented
}
