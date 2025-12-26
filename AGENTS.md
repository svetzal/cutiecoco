# Agent Instructions

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CutieCoCo is a cross-platform Tandy Color Computer 3 (CoCo 3) emulator built with Qt. It is derived from the VCC (Virtual Color Computer) emulator lineage: VCC → VCCE → DREAM-VCC → CutieCoCo. The emulator supports a 128K CoCo 3 with memory expansion up to 8192K, and both Motorola 6809 and Hitachi 6309 CPUs.

## Current State

- Qt application boots to BASIC prompt with keyboard, audio, joystick (numpad), and cartridge loading
- CMake build system with monorepo structure: `emulation/`, `platforms/`, `shared/`
- Windows native platform skeleton exists but needs testing
- Original Windows VCC code preserved on `dream-vcc/main` branch

## Issue Tracking

This project uses **bd** (beads) for issue tracking. Run `bd ready` to find available work.

## Build Commands

**Requirements:** Qt 6.x, CMake 3.20+

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$HOME/Qt/6.10.1/macos"
cmake --build .
open platforms/qt/cutiecoco.app  # macOS
```

## Directory Structure

```
emulation/               # Platform-independent emulation library
  include/cutie/
    emulator.h           # Public CocoEmulator API
    interfaces.h         # Platform abstraction interfaces (IVideoOutput, IAudioOutput, etc.)
    context.h            # EmulationContext singleton for platform services
    compat.h             # VCC compatibility layer (defines.h redirects here)
    stubs.h              # Stubs for removed Windows functionality
    keyboard.h, keymapping.h  # Keyboard handling
  src/
    cocoemu.cpp          # CocoEmulator implementation
  # Legacy emulation files: mc6809.cpp, hd6309.cpp, tcc1014*.cpp, coco3.cpp, mc6821.cpp, iobus.cpp
  libcommon/             # Shared utilities (logger, bus, media)

platforms/
  qt/                    # Qt application (cross-platform)
  windows/               # Windows native application (skeleton)

shared/
  system-roms/           # CoCo 3 system ROM files (coco3.rom)

tests/                   # Catch2 unit tests
```

## Public Emulation API

```cpp
namespace cutie {
enum class MemorySize { Mem128K, Mem512K, Mem2M, Mem8M };  // Mem prefix avoids legacy macro conflicts
enum class CpuType { MC6809, HD6309 };

class CocoEmulator {
    static std::unique_ptr<CocoEmulator> create(const EmulatorConfig& config = {});
    bool init();
    void reset();
    void runFrame();                                         // Synchronous - caller controls timing
    std::pair<const uint8_t*, size_t> getFramebuffer() const;
    std::pair<const int16_t*, size_t> getAudioSamples() const;
    void setKeyState(int row, int col, bool pressed);
};
}
```

**API Gotchas:**
- `getCpuType()` returns default before `init()` - must call `init()` first
- `getFramebufferInfo().pitch` is in **pixels, not bytes**
- Framebuffer is 640x480 (doubled from native CoCo resolutions)

## Code Conventions

- Legacy code: `EmuState` global, `VCC::` namespace
- New code: `cutie::` namespace
- Core emulation has no platform dependencies

## Hardware Behavior Preservation

**CRITICAL:** This is an emulator. The behaviors in emulation modules reflect **ACTUAL HARDWARE BEHAVIORS** that software depends on.

Do NOT:
- "Optimize" emulation code that seems inefficient
- Remove seemingly redundant operations
- Simplify timing loops or cycle counting
- Change instruction behavior

What looks like inefficiency is often accurate hardware emulation.

## Key Architectural Decisions

### Execution Model

CocoEmulator uses a **synchronous, pull-based API**. The platform controls timing via timer-driven `runFrame()` calls, not callbacks. This avoids thread synchronization complexity.

### EmulationContext Pattern

The `EmulationContext` singleton bridges legacy stubs and modern interfaces. Platforms inject implementations at startup:

```cpp
auto& ctx = cutie::EmulationContext::instance();
ctx.setAudioOutput(&myAudioBackend);
ctx.setSystemRomPath("/path/to/roms");
```

Legacy code calls stubs → stubs delegate to EmulationContext → context uses injected implementations.

### Stubs vs Interfaces

- `stubs.h` - Inline stubs for legacy VCC code compatibility (being replaced gradually)
- `interfaces.h` - Abstract C++ interfaces for new code (IVideoOutput, IAudioOutput, etc.)

## Critical Gotchas

### Qt HiDPI/Retina Displays

`resizeGL(int w, int h)` receives **logical pixels**. Multiply by `devicePixelRatio()` for `glViewport()`:

```cpp
void resizeGL(int w, int h) {
    qreal dpr = devicePixelRatio();
    glViewport(0, 0, static_cast<int>(w * dpr), static_cast<int>(h * dpr));
}
```

Symptom of getting this wrong: content renders at 1/4 size in corner.

### Emulator Initialization Order

Order matters. Incorrect order causes crashes:

1. `MmuInit()` - allocate RAM, load ROM
2. Set up `EmuState` (PTRsurface32, SurfacePitch, BitDepth=3, RamBuffer)
3. `GimeInit()`, `GimeReset()`, `mc6883_reset()` - GIME/SAM before CPU
4. `MC6809Init()`, `MC6809Reset()` - CPU reads reset vector from ROM
5. `MiscReset()` - clears audio timing
6. `SetAudioRate()` - MUST follow MiscReset or infinite loop occurs

**BitDepth is an index (0-3), not actual bits.** Use 3 for 32-bit rendering.

### CoCo Keyboard Matrix

The CoCo keyboard wiring is counterintuitive:
- Port B ($FF02) = Column STROBE **outputs**
- Port A ($FF00) = Row RETURN **inputs**

This is opposite of typical descriptions. The scan function must transpose the matrix.

```
         Col0  Col1  Col2  Col3  Col4  Col5  Col6  Col7
Row 0:    @     A     B     C     D     E     F     G
Row 1:    H     I     J     K     L     M     N     O
Row 2:    P     Q     R     S     T     U     V     W
Row 3:    X     Y     Z     UP   DOWN  LEFT  RIGHT SPACE
Row 4:    0     1     2     3     4     5     6     7
Row 5:    8     9     :     ;     ,     -     .     /
Row 6:   ENT   CLR   BRK   ALT   CTL   F1    F2   SHIFT
```

CoCo shift mappings differ from PC: `"` is Shift+2, `'` is Shift+7, etc. Use character-based mapping via `cutie::mapCharToCoco()`.

### CPU Testing

The CoCo 3 MMU maps $FF00-$FFFF to I/O ports, not RAM. Use `MC6809ForcePC(address)` to set PC directly instead of relying on the reset vector.

## Testing

```bash
cmake --build . --target cpu_tests
cmake --build . --target integration_tests
ctest --output-on-failure
```

Tests use `SKIP()` when system ROM isn't found (CI environments).

## Original VCC Reference

Original Windows code is on `dream-vcc/main` branch:
```bash
git show dream-vcc/main:config.cpp | head -200
```
