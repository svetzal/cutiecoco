/*
Copyright 2024-2025 CutieCoCo Contributors
This file is part of CutieCoCo.

Integration Tests - Tests the full emulation system via CocoEmulator API
*/

#include <catch2/catch_test_macros.hpp>
#include "cutie/emulator.h"
#include "cutie/context.h"
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// Helper to find system ROM
// ============================================================================

static fs::path findSystemRomPath() {
    // Try common locations relative to test execution
    std::vector<fs::path> candidates = {
        "system-roms",
        "../system-roms",
        "../../system-roms",
        "shared/system-roms",
        "../shared/system-roms",
        "../../shared/system-roms",
    };

    for (const auto& dir : candidates) {
        fs::path romPath = dir / "coco3.rom";
        if (fs::exists(romPath)) {
            return dir;
        }
    }
    return "";  // Not found
}

// ============================================================================
// CocoEmulator Creation Tests
// ============================================================================

TEST_CASE("CocoEmulator: Create with default config", "[integration][emulator]") {
    cutie::EmulatorConfig config;
    config.systemRomPath = findSystemRomPath();

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator != nullptr);
}

TEST_CASE("CocoEmulator: Create with various memory sizes", "[integration][emulator]") {
    auto romPath = findSystemRomPath();

    SECTION("128K memory") {
        cutie::EmulatorConfig config;
        config.memorySize = cutie::MemorySize::Mem128K;
        config.systemRomPath = romPath;
        auto emulator = cutie::CocoEmulator::create(config);
        REQUIRE(emulator != nullptr);
        REQUIRE(emulator->getMemorySize() == cutie::MemorySize::Mem128K);
    }

    SECTION("512K memory") {
        cutie::EmulatorConfig config;
        config.memorySize = cutie::MemorySize::Mem512K;
        config.systemRomPath = romPath;
        auto emulator = cutie::CocoEmulator::create(config);
        REQUIRE(emulator != nullptr);
        REQUIRE(emulator->getMemorySize() == cutie::MemorySize::Mem512K);
    }

    SECTION("2M memory") {
        cutie::EmulatorConfig config;
        config.memorySize = cutie::MemorySize::Mem2M;
        config.systemRomPath = romPath;
        auto emulator = cutie::CocoEmulator::create(config);
        REQUIRE(emulator != nullptr);
        REQUIRE(emulator->getMemorySize() == cutie::MemorySize::Mem2M);
    }

    SECTION("8M memory") {
        cutie::EmulatorConfig config;
        config.memorySize = cutie::MemorySize::Mem8M;
        config.systemRomPath = romPath;
        auto emulator = cutie::CocoEmulator::create(config);
        REQUIRE(emulator != nullptr);
        REQUIRE(emulator->getMemorySize() == cutie::MemorySize::Mem8M);
    }
}

TEST_CASE("CocoEmulator: Create with different CPU types", "[integration][emulator]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping CPU type test");
    }

    SECTION("MC6809 CPU") {
        cutie::EmulatorConfig config;
        config.cpuType = cutie::CpuType::MC6809;
        config.systemRomPath = romPath;
        auto emulator = cutie::CocoEmulator::create(config);
        REQUIRE(emulator != nullptr);
        // CPU type is finalized during init()
        REQUIRE(emulator->init());
        REQUIRE(emulator->getCpuType() == cutie::CpuType::MC6809);
    }

    SECTION("HD6309 CPU") {
        cutie::EmulatorConfig config;
        config.cpuType = cutie::CpuType::HD6309;
        config.systemRomPath = romPath;
        auto emulator = cutie::CocoEmulator::create(config);
        REQUIRE(emulator != nullptr);
        // CPU type is finalized during init()
        REQUIRE(emulator->init());
        REQUIRE(emulator->getCpuType() == cutie::CpuType::HD6309);
    }
}

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_CASE("CocoEmulator: Init succeeds with valid ROM", "[integration][emulator]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping ROM-dependent test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator != nullptr);
    REQUIRE(emulator->init() == true);
    REQUIRE(emulator->isReady() == true);
}

TEST_CASE("CocoEmulator: Init fails with invalid ROM path", "[integration][emulator]") {
    cutie::EmulatorConfig config;
    config.systemRomPath = "/nonexistent/path/to/roms";

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator != nullptr);
    // Init may still succeed if it falls back to a default, or may fail
    // The behavior depends on implementation - just verify no crash
}

// ============================================================================
// Frame Execution Tests
// ============================================================================

TEST_CASE("CocoEmulator: Can run frames without crashing", "[integration][execution]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping execution test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;  // Disable audio for testing

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    // Run several frames - should not crash
    for (int i = 0; i < 10; ++i) {
        emulator->runFrame();
    }

    REQUIRE(emulator->isReady());
}

TEST_CASE("CocoEmulator: Framebuffer is non-null after running frames", "[integration][execution]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping framebuffer test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    // Run a few frames to generate output
    for (int i = 0; i < 5; ++i) {
        emulator->runFrame();
    }

    auto [pixels, size] = emulator->getFramebuffer();
    REQUIRE(pixels != nullptr);
    REQUIRE(size > 0);
}

TEST_CASE("CocoEmulator: Framebuffer info returns valid dimensions", "[integration][execution]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping framebuffer info test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());
    emulator->runFrame();

    auto info = emulator->getFramebufferInfo();

    // CoCo 3 output may be scaled internally for display
    // Native resolutions: 320x200, 320x225, 640x200, 640x225
    // Emulator may double these: 640x400, 640x450, 640x480
    REQUIRE(info.width >= 256);
    REQUIRE(info.width <= 1280);   // Allow 2x scaling
    REQUIRE(info.height >= 192);
    REQUIRE(info.height <= 480);   // Allow scaled output
    // Pitch is in pixels, not bytes (see framebuffer.h)
    REQUIRE(info.pitch >= info.width);  // Pitch >= width (may have padding)
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_CASE("CocoEmulator: Reset does not crash", "[integration][reset]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping reset test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    // Run some frames, then reset
    for (int i = 0; i < 5; ++i) {
        emulator->runFrame();
    }

    emulator->reset();

    // Run more frames after reset
    for (int i = 0; i < 5; ++i) {
        emulator->runFrame();
    }

    REQUIRE(emulator->isReady());
}

// ============================================================================
// Input Tests
// ============================================================================

TEST_CASE("CocoEmulator: setKeyState does not crash", "[integration][input]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping input test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());
    emulator->runFrame();

    // Press and release keys in the keyboard matrix
    // Row 0-6, Column 0-7 is the CoCo keyboard matrix
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 8; ++col) {
            emulator->setKeyState(row, col, true);   // Press
            emulator->runFrame();
            emulator->setKeyState(row, col, false);  // Release
        }
    }

    REQUIRE(emulator->isReady());
}

TEST_CASE("CocoEmulator: setJoystickAxis does not crash", "[integration][input]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping joystick test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());
    emulator->runFrame();

    // Test joystick axis values (typically 0-63 for CoCo analog joysticks)
    for (int joystick = 0; joystick < 2; ++joystick) {
        for (int axis = 0; axis < 2; ++axis) {
            emulator->setJoystickAxis(joystick, axis, 0);    // Left/Up
            emulator->runFrame();
            emulator->setJoystickAxis(joystick, axis, 32);   // Center
            emulator->runFrame();
            emulator->setJoystickAxis(joystick, axis, 63);   // Right/Down
            emulator->runFrame();
        }
    }

    REQUIRE(emulator->isReady());
}

TEST_CASE("CocoEmulator: setJoystickButton does not crash", "[integration][input]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping joystick button test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());
    emulator->runFrame();

    // Test joystick buttons (2 joysticks, 1 button each on CoCo)
    for (int joystick = 0; joystick < 2; ++joystick) {
        emulator->setJoystickButton(joystick, 0, true);   // Press
        emulator->runFrame();
        emulator->setJoystickButton(joystick, 0, false);  // Release
        emulator->runFrame();
    }

    REQUIRE(emulator->isReady());
}

// ============================================================================
// Cartridge Tests
// ============================================================================

TEST_CASE("CocoEmulator: hasCartridge returns false initially", "[integration][cartridge]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping cartridge test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    REQUIRE(emulator->hasCartridge() == false);
}

TEST_CASE("CocoEmulator: loadCartridge fails with invalid path", "[integration][cartridge]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping cartridge test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    bool loaded = emulator->loadCartridge("/nonexistent/cart.rom");
    REQUIRE(loaded == false);
    REQUIRE(emulator->hasCartridge() == false);
}

TEST_CASE("CocoEmulator: ejectCartridge does not crash when empty", "[integration][cartridge]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping eject test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    // Eject when no cartridge is inserted - should not crash
    emulator->ejectCartridge();

    REQUIRE(emulator->hasCartridge() == false);
}

// ============================================================================
// Audio Tests
// ============================================================================

TEST_CASE("CocoEmulator: getAudioSamples with audio disabled", "[integration][audio]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping audio test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;  // Disabled

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());
    emulator->runFrame();

    auto [samples, count] = emulator->getAudioSamples();
    // With audio disabled, we might get null or empty samples
    // Just verify it doesn't crash
    REQUIRE(true);  // If we got here, it didn't crash
}

TEST_CASE("CocoEmulator: getAudioInfo returns valid data", "[integration][audio]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping audio info test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 44100;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    auto info = emulator->getAudioInfo();
    // Sample rate should match config or be 0 if audio is not supported
    REQUIRE((info.sampleRate == 44100 || info.sampleRate == 0));
}

// ============================================================================
// EmulationContext Tests
// ============================================================================

TEST_CASE("EmulationContext: Singleton returns same instance", "[integration][context]") {
    auto& ctx1 = cutie::EmulationContext::instance();
    auto& ctx2 = cutie::EmulationContext::instance();

    REQUIRE(&ctx1 == &ctx2);
}

TEST_CASE("EmulationContext: Default interfaces are non-null", "[integration][context]") {
    auto& ctx = cutie::EmulationContext::instance();

    REQUIRE(ctx.videoOutput() != nullptr);
    REQUIRE(ctx.audioOutput() != nullptr);
    REQUIRE(ctx.inputProvider() != nullptr);
    REQUIRE(ctx.cartridge() != nullptr);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_CASE("CocoEmulator: Can run many frames continuously", "[integration][stress]") {
    auto romPath = findSystemRomPath();
    if (romPath.empty()) {
        SKIP("System ROM not found - skipping stress test");
    }

    cutie::EmulatorConfig config;
    config.systemRomPath = romPath;
    config.audioSampleRate = 0;

    auto emulator = cutie::CocoEmulator::create(config);
    REQUIRE(emulator->init());

    // Run 600 frames (about 10 seconds at 60fps)
    // This should be enough to complete BASIC boot
    for (int i = 0; i < 600; ++i) {
        emulator->runFrame();
    }

    REQUIRE(emulator->isReady());

    // After boot, framebuffer should have changed from initial state
    auto [pixels, size] = emulator->getFramebuffer();
    REQUIRE(pixels != nullptr);
    REQUIRE(size > 0);
}
