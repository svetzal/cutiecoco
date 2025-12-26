# Agent Instructions

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CutieCoCo is a cross-platform Tandy Color Computer 3 (CoCo 3) emulator built with Qt. It is derived from the VCC (Virtual Color Computer) emulator lineage: VCC → VCCE → DREAM-VCC → CutieCoCo. The emulator supports a 128K CoCo 3 with memory expansion up to 8192K, and both Motorola 6809 and Hitachi 6309 CPUs.

## Current State: Qt Port In Progress

The `qt` branch contains the cross-platform port. Key changes:
- CMake replaces Visual Studio build system
- Qt6 replaces Windows-specific UI/graphics/audio
- Windows-specific code has been removed; legacy emulation files are being cleaned

**What's Done:**
- CMake build system with Qt6 dependencies
- Monorepo structure: `emulation/`, `platforms/`, `shared/`
- Emulation library (`emulation/`) with public CocoEmulator API
- Qt application (`platforms/qt/`) boots to BASIC prompt
- Compatibility layer for legacy VCC types (`emulation/include/cutie/`)
- CocoEmulator concrete implementation (`emulation/src/cocoemu.cpp`)
- EmulatorWidget uses timer-driven synchronous API (not threaded callbacks)
- Stubs for removed Windows functionality (`emulation/include/cutie/stubs.h`)
- All legacy emulation files cleaned and integrated
- Keyboard input with character-based mapping
- Audio output via Qt using QAudioSink (`platforms/qt/src/qtaudiooutput.cpp`)
- Joystick input via keyboard numpad emulation
- Cartridge loading (.rom, .ccc, .pak files) with auto-start support
- Qt configuration system using QSettings (`platforms/qt/src/appconfig.cpp`)
- Qt settings dialog with CPU, Audio, Display tabs (`platforms/qt/src/settingsdialog.cpp`)
- Recent files menu with cartridge history
- Window geometry persistence
- Windows native platform skeleton (`platforms/windows/`)

**What Remains:**
- Native joystick/gamepad support (currently keyboard numpad only)
- Windows platform testing and refinement
- macOS native platform (planned)

## Issue Tracking

This project uses **bd** (beads) for issue tracking. Run `bd ready` to find available work.

```bash
bd ready              # Find available work (no blockers)
bd show <id>          # View issue details
bd update <id> --status=in_progress  # Claim work
bd close <id>         # Complete work
bd sync --from-main   # Sync beads (for ephemeral branches)
```

## Build Commands (Qt Port)

**Requirements:** Qt 6.x, CMake 3.20+

```bash
# Configure (adjust Qt path as needed)
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$HOME/Qt/6.10.1/macos"

# Build
cmake --build .

# Run (macOS)
open platforms/qt/cutiecoco.app
```

**Output:** `build/platforms/qt/cutiecoco.app` (macOS) or `build/platforms/qt/cutiecoco` (Linux)

## Architecture

### Directory Structure

```
emulation/               # Platform-independent emulation library
  include/cutie/
    types.h              # Core types (SystemState, Point, Size, Rect)
    compat.h             # VCC compatibility layer (includes types + debugger)
    emulator.h           # Public CocoEmulator API
    interfaces.h         # Platform abstraction interfaces
    context.h            # EmulationContext singleton for platform services
    keyboard.h           # Keyboard matrix types and Keyboard class
    keymapping.h         # Character-to-CoCo key translation (shared)
    framebuffer.h        # IFrameBuffer interface
    emulation.h          # EmulationThread class (legacy, still available)
    stubs.h              # Stubs for removed Windows functionality
  src/
    core.cpp             # Global EmuState definition
    context.cpp          # EmulationContext singleton implementation
    cocoemu.cpp          # CocoEmulator implementation (main entry point)
    emulation.cpp        # EmulationThread implementation
    keyboard.cpp         # Keyboard matrix handling
    keymapping.cpp       # Character-to-CoCo key translation
  # Legacy emulation files (cleaned, integrated):
  mc6809.cpp, hd6309.cpp # CPU emulation
  tcc1014*.cpp           # GIME graphics/MMU
  coco3.cpp              # System coordination
  mc6821.cpp             # PIA (I/O ports)
  iobus.cpp              # I/O bus
  libcommon/             # Shared utilities (logger, bus, media)

platforms/
  qt/                    # Qt application (cross-platform)
    include/
      mainwindow.h
      emulatorwidget.h   # OpenGL display widget + emulation integration
      appconfig.h        # QSettings-based configuration
      settingsdialog.h   # Settings dialog (CPU, Audio, Display tabs)
      qtaudiooutput.h    # QAudioSink audio backend
    src/
      main.cpp
      mainwindow.cpp
      emulatorwidget.cpp
      appconfig.cpp
      settingsdialog.cpp
      qtaudiooutput.cpp
  windows/               # Windows native application
    include/
      mainwindow.h       # Win32 main window
      win32renderer.h    # GDI renderer
      win32audio.h       # waveOut audio backend
      win32input.h       # Keyboard/joystick input handling
    src/
      main.cpp           # WinMain entry point
      mainwindow.cpp
      win32renderer.cpp
      win32audio.cpp
      win32input.cpp
    resources/
      cutiecoco.rc       # Version info, icon
      cutiecoco.manifest # DPI awareness, visual styles
  macos/                 # Future macOS-native application

shared/
  3rdparty/              # Third-party headers (GL, stb)
  resources/             # Icons, graphics resources
  system-roms/           # CoCo 3 system ROM files

tests/                   # Unit tests (Catch2)
  CMakeLists.txt         # Test target configuration
  cpu_test_harness.h     # CPUTestHarness class
  cpu_test_harness.cpp   # Test harness implementation
  mc6809_tests.cpp       # MC6809 CPU instruction tests
```

### Public Emulation API

The emulation library exposes a clean public API through two main headers:

**`emulation/include/cutie/emulator.h`** - Main emulator interface:
```cpp
namespace cutie {

// Note: Enum values use Mem prefix to avoid conflicts with legacy macros (_128K, _512K, etc.)
enum class MemorySize { Mem128K, Mem512K, Mem2M, Mem8M };
enum class CpuType { MC6809, HD6309 };

struct EmulatorConfig {
    MemorySize memorySize = MemorySize::Mem512K;
    CpuType cpuType = CpuType::MC6809;
    std::filesystem::path systemRomPath;
    uint32_t audioSampleRate = 44100;  // Note: Set to 0 until audio backend implemented
};

class CocoEmulator {
public:
    static std::unique_ptr<CocoEmulator> create(const EmulatorConfig& config = {});

    // Lifecycle
    virtual bool init() = 0;
    virtual void reset() = 0;
    virtual void shutdown() = 0;

    // Execution (synchronous - caller controls timing)
    virtual void runFrame() = 0;
    virtual int runCycles(int cycles) = 0;

    // Input
    virtual void setKeyState(int row, int col, bool pressed) = 0;
    virtual void setJoystickAxis(int joystick, int axis, int value) = 0;
    virtual void setJoystickButton(int joystick, int button, bool pressed) = 0;

    // Output (C++17 compatible - uses std::pair instead of std::span)
    virtual std::pair<const uint8_t*, size_t> getFramebuffer() const = 0;
    virtual std::pair<const int16_t*, size_t> getAudioSamples() const = 0;
};

} // namespace cutie
```

**Implementation:** `emulation/src/cocoemu.cpp` - Concrete `CocoEmulatorImpl` class that wraps legacy emulation code.

**`emulation/include/cutie/interfaces.h`** - Platform abstraction interfaces:
```cpp
namespace cutie {

// Video output - platform implements to receive frames
class IVideoOutput {
    virtual void onFrame(const uint8_t* pixels, int width, int height, int pitch) = 0;
    virtual void onModeChange(int width, int height) = 0;
};

// Audio output - platform implements to play audio
class IAudioOutput {
    virtual bool init(uint32_t sampleRate) = 0;
    virtual void submitSamples(const int16_t* samples, size_t count) = 0;
    virtual size_t getQueuedSampleCount() const = 0;
};

// Input provider - platform implements to provide keyboard/joystick
class IInputProvider {
    virtual uint8_t scanKeyboard(uint8_t colMask) const = 0;
    virtual int getJoystickAxis(int joystick, int axis) const = 0;
    virtual bool getJoystickButton(int joystick, int button) const = 0;
};

// Cartridge interface - for ROM paks and expansion devices
class ICartridge {
    virtual uint8_t read(uint16_t address) const = 0;
    virtual void write(uint16_t address, uint8_t value) = 0;
    virtual uint8_t readPort(uint8_t port) const = 0;
    virtual void writePort(uint8_t port, uint8_t value) = 0;
};

// Null implementations provided for testing: NullVideoOutput, NullAudioOutput, etc.

} // namespace cutie
```

### Compatibility Layer

The `emulation/include/cutie/compat.h` header provides VCC-compatible types so legacy code can compile with minimal changes:

```cpp
// Legacy code includes this:
#include "defines.h"      // Redirects to cutie/compat.h

// Provides:
// - SystemState struct with EmuState global
// - VCC::Point, VCC::Size, VCC::Rect
// - VCC::Debugger::Debugger (stub)
// - Legacy constants (FRAMEINTERVAL, COLORBURST, etc.)
```

### Core Emulation Files

| File | Purpose | Cleanup Status |
|------|---------|----------------|
| `mc6809.cpp/h` | Motorola 6809 CPU | Cleaned (not yet integrated) |
| `hd6309.cpp/h` | Hitachi 6309 CPU | Cleaned (not yet integrated) |
| `mc6821.cpp/h` | PIA (I/O ports) | Cleaned (has real implementations that conflict with stubs) |
| `coco3.cpp` | System emulation loop | Cleaned (not yet integrated) |
| `tcc1014graphics.cpp` | GIME video | Cleaned |
| `tcc1014mmu.cpp/h` | Memory management | Cleaned |
| `tcc1014registers.cpp` | GIME registers | Cleaned |
| `iobus.cpp` | I/O bus | Cleaned |
| `pakinterface.cpp` | Cartridge interface | Heavy Windows deps, needs major work |

### Emulation File Dependencies

The emulation files have deep interconnections. Understanding these is critical before integration:

```
coco3.cpp
├── CPUExec (function pointer to MC6809Exec or HD6309Exec)
├── mc6821.cpp (PIA - defines GetMuxState, GetDACSample, etc.)
├── tcc1014*.cpp (GIME)
└── pakinterface.cpp (cartridge)

mc6821.cpp
├── CPUAssertInterupt/CPUDeAssertInterupt (from CPU files)
├── vccJoystickStart* (joystick input - needs stubs)
├── Motor() (cassette - needs stub)
└── PackAudioSample() (from pakinterface)

pakinterface.cpp
├── vcc/ui/menu/* (deleted - needs major refactoring)
├── vcc/utils/cartridge_loader (may work)
└── Windows DLL loading (needs replacement)
```

## Cleaning Legacy Files

When cleaning a legacy file of Windows dependencies:

1. **Remove Windows includes and add stubs.h:**
   ```cpp
   // Remove these:
   #include <Windows.h>
   #include "ddraw.h"
   #include "Debugger.h"      // Deleted
   #include "Disassembler.h"  // Deleted
   #include "keyboard.h"      // Deleted
   #include "joystickinput.h" // Deleted
   #include "Cassette.h"      // Deleted
   #include "config.h"        // Deleted
   #include "audio.h"         // Old Windows version, deleted
   #include "DirectDrawInterface.h" // Deleted
   #include "Vcc.h"           // Deleted
   #include "throttle.h"      // Windows timing, deleted
   #include "pakinterface.h"  // Heavy deps, use stubs instead
   #include "OpDecoder.h"     // Debugger, not needed for emulation

   // Add this for stub implementations:
   #include "cutie/stubs.h"
   ```

2. **Keep defines.h** - it now redirects to `cutie/compat.h`

3. **Replace Windows types:**
   - `BOOL` → `bool`
   - `TRUE/FALSE` → `true/false`
   - `HANDLE` → `FILE*` or appropriate type
   - `HWND`, `HINSTANCE` → `void*` (in SystemState)
   - `_inline` → `inline` (MSVC-specific keyword)
   - `__fastcall` → remove (MSVC calling convention)

4. **Remove MSVC pragmas:**
   ```cpp
   // Remove:
   #pragma warning( disable : 4800 )
   ```

5. **Fix headers before cpp files:**
   Headers often need includes added. Common missing includes:
   ```cpp
   #include <vector>           // For std::vector in function signatures
   #include "cutie/compat.h"   // For VCC::CPUState
   ```

6. **Use stubs.h for removed functionality:**
   The `emulation/include/cutie/stubs.h` provides inline stubs for functions from deleted modules:
   - Cassette: `GetMotorState()`, `FlushCassetteBuffer()`, `GetCasSample()`, etc.
   - Audio: `GetFreeBlockCount()`, `FlushAudioBuffer()`, `GetDACSample()`, `AUDIO_RATE`
   - Display: `LockScreen()`, `UnlockScreen()`, `GetMem()`
   - Throttle: `CalculateFPS()`, `StartRender()`, `EndRender()`
   - Config: `GetUseCustomSystemRom()`, `GetCustomSystemRomPath()`
   - Pak: `PakGetSystemRomPath()`, `PackMem8Read()`, `PakReadPort()`, `PakWritePort()`, `PakTimer()`
   - CPU: `CPUExec` function pointer (defaults to stub, set to MC6809Exec/HD6309Exec)
   - Windows API: `MessageBox()` (prints to stderr)

   If you need a stub that doesn't exist, add it to stubs.h with a sensible default value.

7. **Use C++ standard headers:**
   - `<stdio.h>` → `<cstdio>`
   - `<stdlib.h>` → `<cstdlib>`
   - `<string.h>` → `<cstring>`
   - `<math.h>` → `<cmath>`

8. **Stub Windows-only features entirely:**
   Functions like clipboard paste (`PasteText`, `CopyText`) should be stubbed with TODO comments:
   ```cpp
   void PasteText() {
       // TODO: Implement with Qt clipboard
   }
   ```

### Stub Conflicts

**IMPORTANT:** Some files define real implementations that conflict with stubs. When integrating:

- `mc6821.cpp` defines `GetDACSample()`, `GetCasSample()`, `SetCassetteSample()`, `GetMuxState()`
- These conflict with inline stubs in `stubs.h`

**Solution options:**
1. Remove conflicting stubs from `stubs.h` when integrating the real file
2. Use `#ifndef` guards around stubs
3. Reorganize stubs into categories that can be selectively included

### VCC::CPUState vs cutie::CPUState

The `VCC::CPUState` is an alias to `cutie::CPUState` (defined in `types.h`). When adding CPU register fields:
- Add to `cutie::CPUState` in `emulation/include/cutie/types.h`
- Do NOT add a separate struct in `compat.h` (will conflict)

Current CPUState fields:
```cpp
struct CPUState {
    uint16_t PC, X, Y, U, S, DP, D;
    uint8_t A, B, CC, E, F;
    uint8_t MD;           // 6309 mode register
    uint16_t V;           // 6309 V register
    bool IsNative6309;    // True if in native 6309 mode
};
```

## libcommon

Platform-independent components (still usable):
- `vcc/bus/` - Cartridge interfaces
- `vcc/media/` - Disk image handling (JVC, VHD, DMK)
- `vcc/devices/` - ROM, PSG, RTC, serial
- `vcc/utils/logger.h` - Logging

Windows-specific components have been removed:
- `vcc/ui/` - Deleted (was Windows dialogs)
- `vcc/utils/winapi.h` - Deleted
- `vcc/utils/FileOps.h` - Deleted
- `vcc/utils/filesystem.h` - Deleted

## Code Conventions

- Legacy code uses `EmuState` global and `VCC::` namespace
- New code should use `cutie::` namespace
- Prefer `<cstdio>` over `<stdio.h>` style includes
- Use `bool` not `BOOL`
- Core emulation should have no platform dependencies

### Hardware Behavior Preservation

**CRITICAL:** This is an emulator of a retro computer. The behaviors codified in the emulation modules are **REQUIRED** because they reflect **ACTUAL HARDWARE BEHAVIORS**.

Do NOT:
- "Optimize" emulation code that seems inefficient
- Remove seemingly redundant operations
- Simplify timing loops or cycle counting
- Change instruction behavior to be "simpler"

The original hardware had specific timing, quirks, and behaviors that software depends on. What looks like inefficiency is often accurate hardware emulation. When in doubt, preserve the existing behavior and add a comment explaining why it might look unusual.

## Emulation Execution Models

There are two execution models available:

### 1. CocoEmulator (Recommended for Qt)

The `cutie::CocoEmulator` class provides a **synchronous, pull-based API**. The platform controls timing:

```cpp
// Create and initialize
auto emulator = cutie::CocoEmulator::create(config);
emulator->init();

// In Qt, use a QTimer to drive frame execution
QTimer* timer = new QTimer(this);
connect(timer, &QTimer::timeout, this, &EmulatorWidget::onEmulationTick);
timer->setInterval(16);  // ~60 Hz
timer->start();

void EmulatorWidget::onEmulationTick() {
    m_emulator->runFrame();

    auto [pixels, size] = m_emulator->getFramebuffer();
    std::memcpy(m_framebuffer.bits(), pixels, size);
    update();  // Trigger repaint
}
```

**Advantages:**
- No thread synchronization needed - runs on Qt main thread
- Simpler code, no mutex/signal complexity
- Platform controls timing (can easily pause, single-step, etc.)

### 2. EmulationThread (Legacy, Still Available)

The `cutie::EmulationThread` class runs emulation in a **separate thread with callbacks**:

```cpp
m_emulation->start([this](const uint8_t* pixels, int width, int height) {
    onEmulatorFrame(pixels, width, height);  // Called from emulation thread!
});
```

**Requires thread-safe frame passing** (see Qt Thread Safety below).

## Qt Thread Safety

If using `EmulationThread` (not recommended), Qt UI must be updated from the main thread:

```cpp
// Signal for cross-thread notification
signals:
    void frameReady();

// Connect with QueuedConnection for thread safety
connect(this, &EmulatorWidget::frameReady,
        this, &EmulatorWidget::onFrameReady, Qt::QueuedConnection);

// Callback from emulation thread - copy data and signal
void EmulatorWidget::onEmulatorFrame(const uint8_t* pixels, int w, int h) {
    {
        std::lock_guard<std::mutex> lock(m_framebufferMutex);
        std::memcpy(m_pendingFrame.data(), pixels, w * h * 4);
        m_frameUpdated = true;
    }
    emit frameReady();
}

// Slot on main thread - safe to update Qt objects
void EmulatorWidget::onFrameReady() {
    std::lock_guard<std::mutex> lock(m_framebufferMutex);
    if (m_frameUpdated) {
        std::memcpy(m_framebuffer.bits(), m_pendingFrame.data(), ...);
        m_frameUpdated = false;
    }
}
```

**Current implementation uses CocoEmulator with QTimer** - simpler and works well.

## Qt OpenGL and HiDPI Displays

**CRITICAL:** On HiDPI/Retina displays, Qt's `QOpenGLWidget` requires special handling for viewport calculations.

### The Problem

On a 2x Retina display:
- Widget logical size: 640×480
- Actual OpenGL framebuffer: 1280×960
- If you use logical dimensions for `glViewport()`, content renders at 1/4 size in the bottom-left corner

### The Solution

Despite what Qt documentation may suggest, `resizeGL(int w, int h)` receives **logical pixels**, not device pixels. You must multiply by `devicePixelRatio()`:

```cpp
void EmulatorWidget::resizeGL(int w, int h)
{
    // Qt passes logical pixels - multiply by DPR for actual framebuffer size
    qreal dpr = devicePixelRatio();
    int deviceWidth = static_cast<int>(w * dpr);
    int deviceHeight = static_cast<int>(h * dpr);

    // Use device dimensions for all viewport calculations
    glViewport(0, 0, deviceWidth, deviceHeight);
}
```

### Symptoms of Incorrect HiDPI Handling

| Symptom | Cause |
|---------|-------|
| Content at 1/4 size in bottom-left corner | Using logical pixels instead of device pixels for viewport |
| Content at 1/4 size in top-left corner | Same issue + Y-axis flip |
| Blurry/scaled content | Texture or framebuffer size mismatch |

### Debugging HiDPI Issues

Add temporary debug output to understand the actual dimensions:

```cpp
fprintf(stderr, "resizeGL: w=%d h=%d, devicePixelRatio=%.2f\n",
        w, h, devicePixelRatio());
```

On a 2x Retina display, you should see `devicePixelRatio=2.00`. If `w` and `h` match your expected logical size (e.g., 640×480), you need to multiply them by `devicePixelRatio()` for OpenGL calls.

### Key Points

1. Store device pixel dimensions calculated in `resizeGL()` for use in `paintGL()`
2. All `glViewport()` calls must use device pixels
3. Texture uploads (`glTexImage2D`) use the source image dimensions (unaffected by DPR)
4. The `width()` and `height()` methods always return logical pixels

## Integrating Legacy Emulation

To integrate a cleaned legacy file:

1. Add it to `emulation/CMakeLists.txt`:
   ```cmake
   add_library(cutie-emulation STATIC
       src/core.cpp
       src/audio.cpp
       src/emulation.cpp
       src/keyboard.cpp
       coco3.cpp  # Legacy files are directly in emulation/
       mc6809.cpp
       # ... etc.
   )
   ```

2. Ensure it includes `cutie/stubs.h` for missing dependencies

3. The legacy code uses globals (`EmuState`) defined in `emulation/src/core.cpp`

4. **Test compilation after EACH file** - errors cascade quickly

### Integration Order (Recommended)

Due to dependencies, integrate files in this order:

1. **CPU files first** (`mc6809.cpp`, `hd6309.cpp`)
   - These are relatively self-contained
   - Need: `CPUAssertInterupt` stubs (add to stubs.h or compat.h)

2. **GIME files** (`tcc1014mmu.cpp`, `tcc1014registers.cpp`, `tcc1014graphics.cpp`)
   - Depend on CPU for interrupt assertions
   - Graphics file is large but mostly self-contained rendering code

3. **I/O files** (`iobus.cpp`, `mc6821.cpp`)
   - PIA has joystick/cassette dependencies to stub
   - **WARNING:** mc6821.cpp conflicts with stubs - resolve before adding

4. **System loop** (`coco3.cpp`)
   - Depends on all of the above
   - Uses `CPUExec` function pointer (defined in core.cpp)

5. **Cartridge** (`pakinterface.cpp`) - last, needs major refactoring

### Common Integration Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `no template named 'vector'` | Header missing `#include <vector>` | Add include to the .h file |
| `redefinition of 'FunctionName'` | Stub conflicts with real impl | Remove stub or use guards |
| `use of undeclared identifier` | Missing stub or include | Add to stubs.h or add include |
| `no member named 'X' in 'CPUState'` | Missing field in types.h | Add to cutie::CPUState |
| `'__declspec' attributes not enabled` | Windows-only export macro | Check libcommon/include/vcc/detail/exports.h has platform guards |

## Emulator Initialization

**CRITICAL:** The emulator initialization order matters. Incorrect order causes crashes.

### Required Initialization Sequence

```cpp
// 1. Initialize memory (allocates RAM, loads ROM)
unsigned char* memory = MmuInit(_512K);  // _128K, _512K, or _2M

// 2. Set up SystemState BEFORE any emulation
EmuState.PTRsurface32 = framebuffer->pixels();
EmuState.SurfacePitch = framebuffer->pitch();
EmuState.BitDepth = 3;  // IMPORTANT: Index, not actual bits!
EmuState.RamBuffer = memory;
EmuState.EmulationRunning = 1;

// 3. Initialize GIME/SAM BEFORE CPU (sets up ROM pointer)
GimeInit();
GimeReset();
mc6883_reset();  // CRITICAL: Initializes ROM pointer for CPU reset

// 4. NOW initialize CPU (reads reset vector from ROM)
MC6809Init();
MC6809Reset();

// 5. Reset misc systems BEFORE audio setup
// IMPORTANT: MiscReset() clears audio timing variables!
MiscReset();

// 6. Enable audio (after MiscReset!)
SetAudioRate(44100);  // Or 0 to disable

// 7. Set CPU execution pointer
CPUExec = MC6809Exec;  // or HD6309Exec
```

### Common Initialization Pitfalls

| Mistake | Symptom | Fix |
|---------|---------|-----|
| CPU reset before `mc6883_reset()` | Crash in `sam_read()` (null ROM pointer) | Call `mc6883_reset()` before `MC6809Reset()` |
| `BitDepth = 32` instead of `3` | Crash calling null function pointer | Use index: 0=8bit, 1=16bit, 2=24bit, 3=32bit |
| `SetAudioRate()` before `MiscReset()` | No audio, emulator boots but silent | Call `MiscReset()` first, then `SetAudioRate()` |
| Missing `MmuInit()` | Null pointer crashes everywhere | Always call first |

### BitDepth Values

The `SystemState.BitDepth` field is an **index into function pointer arrays**, not the actual bit depth:

```cpp
// These arrays are indexed by BitDepth:
void (*UpdateScreen[4])(SystemState*) = {UpdateScreen8, UpdateScreen16, UpdateScreen24, UpdateScreen32};
void (*DrawTopBoarder[4])(SystemState*) = {...};
void (*DrawBottomBoarder[4])(SystemState*) = {...};

// Correct values:
// BitDepth = 0  →  8-bit rendering
// BitDepth = 1  →  16-bit rendering
// BitDepth = 2  →  24-bit rendering
// BitDepth = 3  →  32-bit rendering (use this for Qt)
```

### ROM Loading

The system ROM is loaded by `MmuInit()` → `LoadRom()`:

- Default path: `PakGetSystemRomPath() / "coco3.rom"`
- `PakGetSystemRomPath()` stub returns: `std::filesystem::current_path() / "system-roms"`
- **Result:** ROM expected at `./system-roms/coco3.rom` relative to working directory

For the Qt app running from `build/`:
```bash
mkdir -p build/system-roms
cp coco3.rom build/system-roms/
```

### Audio System

The audio system collects samples into a fixed-size buffer (`AudioBuffer[16384]`). Without a real audio backend:

1. `FlushAudioBuffer()` stub does nothing
2. `AudioIndex` keeps incrementing
3. Eventually writes past buffer end → crash

**Solution:** Call `SetAudioRate(0)` to disable audio collection until a Qt audio backend is implemented.

### Reset Sequence

**CRITICAL:** The `reset()` function must mirror the init sequence for audio timing. `MiscReset()` clears audio timing variables to 0, which causes the CPU timing loop to freeze if not followed by `SetAudioRate()`.

```cpp
void reset() {
    // Reset hardware
    GimeReset();
    mc6883_reset();

    // Reset CPU
    MC6809Reset();  // or HD6309Reset()

    // Reset misc - CLEARS AUDIO TIMING TO 0!
    MiscReset();

    // MUST restore audio timing after MiscReset!
    if (audioSampleRate > 0) {
        SetAudioRate(audioSampleRate);
    }
}
```

**Why this matters:** `MiscReset()` sets `SoundInterupt = 0` and `NanosToSoundSample = 0`. The CPU timing loop in `CPUCycle()` uses these values:

```cpp
while (NanosThisLine >= 1) {
    // Case 2: Audio sampling
    NanosThisLine -= NanosToSoundSample;  // If 0, never decreases!
    NanosToSoundSample = SoundInterupt;   // Stays 0!
    // Loop runs forever...
}
```

If `NanosToSoundSample` is 0, subtracting it doesn't change `NanosThisLine`, causing an infinite loop that freezes the emulator.

## CoCo Keyboard Emulation

**CRITICAL:** The CoCo keyboard matrix wiring is counterintuitive. Getting this wrong causes a perfect row↔column transposition of all keys.

### PIA Keyboard Wiring

The CoCo keyboard connects to PIA0:
- **Port B ($FF02) = Column STROBE outputs** (directly active low)
- **Port A ($FF00) = Row RETURN inputs** (directly active low)

This is the **opposite** of what you might expect! Many emulator docs describe it as "rows out, columns in" but the CoCo hardware is wired "columns out, rows in".

### How Keyboard Scanning Works

1. CoCo BASIC writes to $FF02 to select which column(s) to strobe (0 = selected)
2. CoCo BASIC reads $FF00 to see which rows have a key pressed in those columns (0 = pressed)

For example, to detect 'H' (row 1, column 0):
1. Output `0xFE` to $FF02 (bit 0 = 0, selecting column 0)
2. Read $FF00 and check if bit 1 is 0 (row 1 has a key in column 0)

### The Keyboard Matrix

```
         Col0  Col1  Col2  Col3  Col4  Col5  Col6  Col7
         ----  ----  ----  ----  ----  ----  ----  ----
Row 0:    @     A     B     C     D     E     F     G
Row 1:    H     I     J     K     L     M     N     O
Row 2:    P     Q     R     S     T     U     V     W
Row 3:    X     Y     Z     UP   DOWN  LEFT  RIGHT SPACE
Row 4:    0     1     2     3     4     5     6     7
Row 5:    8     9     :     ;     ,     -     .     /
Row 6:   ENT   CLR   BRK   ALT   CTL   F1    F2   SHIFT
```

### Scan Function Implementation

The scan function must **transpose** the keyboard matrix. We store keys as `m_matrix[row]` with column bits, but when a column is selected, we return which **rows** have that column pressed:

```cpp
uint8_t Keyboard::scan(uint8_t colMask) const
{
    uint8_t result = 0;
    for (uint8_t col = 0; col < 8; ++col) {
        if ((colMask & (1 << col)) == 0) {  // Column selected
            for (uint8_t row = 0; row < 7; ++row) {
                if (m_matrix[row] & (1 << col)) {
                    result |= (1 << row);  // Return ROW bits, not column
                }
            }
        }
    }
    return ~result;  // Active low
}
```

### Symptoms of Incorrect Row/Column Handling

If you implement the scan function wrong (returning columns instead of rows), you'll see a perfect transposition:
- 'H' (row 1, col 0) → 'A' (row 0, col 1)
- 'E' (row 0, col 5) → '8' (row 5, col 0)
- 'L' (row 1, col 4) → '1' (row 4, col 1)
- 'O' (row 1, col 7) → nothing (row 7 doesn't exist!)

### Character-Based Keyboard Mapping

The CoCo has different shift mappings than a PC keyboard. For example:
- PC: `"` is Shift+' (apostrophe)
- CoCo: `"` is Shift+2

Use **character-based mapping** instead of raw key codes for printable characters:

```cpp
// Map the actual character to CoCo key combination
switch (ch) {
    case '"': return {CocoKey::Key2, true};   // Shift+2
    case '\'': return {CocoKey::Key7, true};  // Shift+7
    case '*': return {CocoKey::Colon, true};  // Shift+:
    case '+': return {CocoKey::Semicolon, true}; // Shift+;
    // etc.
}
```

Key implementation points:
1. Use `event->text()` in Qt to get the actual character produced
2. Track which keys have shift "added" so you release shift when the key is released
3. Non-printable keys (arrows, F-keys, modifiers) still use direct key code mapping

### Files Involved

- `emulation/include/cutie/keyboard.h` - CocoKey enum, Keyboard class
- `emulation/src/keyboard.cpp` - Matrix storage and scan function
- `platforms/qt/src/emulatorwidget.cpp` - Qt key event → CoCo key mapping
- `emulation/mc6821.cpp` - PIA calls `vccKeyboardGetScan()` when $FF00 is read

## Original Windows VCC Code Reference

The original VCC/DREAM-VCC Windows code is preserved on the `dream-vcc/main` branch. Use `git show dream-vcc/main:<file>` to read files without switching branches.

### Salvageable Components

| Component | Location | Notes |
|-----------|----------|-------|
| **Menu Builder** | `libcommon/ui/menu/` | Already abstracted, needs Qt backend |
| **Config Structures** | `config.cpp` - `STRConfig` | Data layout for settings |
| **Keyboard Tables** | `keyboard.cpp`, `keyboardLayout.h` | Scancode → CoCo mappings |
| **Joystick Logic** | `joystickinput.cpp` | Hi-res ramp timing, DAC logic |
| **Debugger Architecture** | `Debugger.cpp` | Client registration pattern |

### Must Be Replaced

| Windows Tech | Qt Replacement |
|-------------|----------------|
| DirectSound | QAudioOutput / QAudioSink |
| DirectInput8 | QGamepad or SDL2 |
| GetOpenFileName | QFileDialog |
| MessageBox | QMessageBox |
| DirectDraw/D3D | Already using OpenGL |
| INI via Win32 | QSettings |

### Dialogs to Recreate (from resource.h)

- `IDD_CONFIG` - Main configuration tabbed dialog
- `IDD_CPU` - CPU type (6809/6309), speed/overclock
- `IDD_AUDIO` - Sound card selection, sample rate
- `IDD_DISPLAY` - Monitor type, scanlines, aspect ratio
- `IDD_INPUT` - Keyboard layout selection
- `IDD_JOYSTICK` - Joystick axis/button mapping
- `IDD_CASSETTE` - Tape player controls
- `IDD_BITBANGER` - Serial port configuration
- `IDD_ABOUTBOX` - Version and credits

### Reading Original Code

```bash
# List UI-related files
git ls-tree -r --name-only dream-vcc/main | grep -iE '(dialog|config|menu|ui)'

# Read a specific file
git show dream-vcc/main:config.cpp | head -200

# Find dialog callbacks
git show dream-vcc/main:config.cpp | grep -n "CALLBACK"
```

## Windows Native Platform

The `platforms/windows/` directory contains a native Windows application that uses Win32 API directly (no Qt dependency).

### Build Configuration

The root `CMakeLists.txt` supports conditional platform builds:

```cmake
# Build options (configure with -D flags)
option(CUTIECOCO_BUILD_QT "Build Qt cross-platform application" ON)
option(CUTIECOCO_BUILD_WINDOWS "Build Windows native application" OFF)

# Windows native build (no Qt required):
cmake .. -DCUTIECOCO_BUILD_QT=OFF -DCUTIECOCO_BUILD_WINDOWS=ON
```

### Windows Application Architecture

The Windows platform follows a component-based architecture matching the Qt platform:

| Component | File | Purpose |
|-----------|------|---------|
| `MainWindow` | `mainwindow.cpp` | Win32 window, message loop, menu handling |
| `Win32Renderer` | `win32renderer.cpp` | GDI-based rendering with StretchDIBits |
| `Win32Audio` | `win32audio.cpp` | waveOut audio backend (double-buffered) |
| `Win32Input` | `win32input.cpp` | Keyboard mapping, numpad joystick |

### Win32 Message Handling Pattern

The main window uses the standard Win32 pattern with `GWLP_USERDATA` for instance pointer:

```cpp
LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) return self->handleMessage(msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
```

### Windows Keyboard Input

Windows keyboard handling requires both `WM_KEYDOWN`/`WM_KEYUP` AND `WM_CHAR`:

- `WM_KEYDOWN/UP`: Virtual key codes for non-printable keys (arrows, F-keys, modifiers)
- `WM_CHAR`: Actual characters produced (handles shift state correctly)

```cpp
case WM_KEYDOWN:
    m_input->handleKeyDown(wParam, lParam);  // For arrows, F-keys, etc.
    return 0;
case WM_CHAR:
    m_input->handleChar(static_cast<wchar_t>(wParam));  // For printable chars
    return 0;
case WM_KILLFOCUS:
    m_input->reset();  // Release all keys when window loses focus
    return 0;
```

### GDI Rendering with Aspect Ratio

The `Win32Renderer` uses `StretchDIBits` with aspect ratio preservation:

```cpp
// Calculate destination rect preserving 4:3 aspect ratio
float srcAspect = static_cast<float>(srcWidth) / srcHeight;
float dstAspect = static_cast<float>(m_width) / m_height;

if (srcAspect > dstAspect) {
    // Letterbox (black bars top/bottom)
    destHeight = static_cast<int>(m_width / srcAspect);
    destY = (m_height - destHeight) / 2;
} else {
    // Pillarbox (black bars left/right)
    destWidth = static_cast<int>(m_height * srcAspect);
    destX = (m_width - destWidth) / 2;
}

StretchDIBits(hdc, destX, destY, destWidth, destHeight,
              0, 0, srcWidth, srcHeight, pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);
```

### Windows DPI Awareness

The manifest file (`cutiecoco.manifest`) declares DPI awareness:

```xml
<application xmlns="urn:schemas-microsoft-com:asm.v3">
  <windowsSettings>
    <dpiAware>true/pm</dpiAware>
    <dpiAwareness>PerMonitorV2</dpiAwareness>
  </windowsSettings>
</application>
```

This ensures proper scaling on high-DPI displays.

## Qt Configuration System

The Qt platform uses `QSettings` for persistent configuration via the `AppConfig` singleton.

### AppConfig Pattern

```cpp
// appconfig.h
class AppConfig : public QObject {
    Q_OBJECT
public:
    static AppConfig& instance();  // Singleton

    // Typed accessors with signals for change notification
    cutie::MemorySize memorySize() const;
    void setMemorySize(cutie::MemorySize size);

signals:
    void memorySizeChanged(cutie::MemorySize size);

private:
    AppConfig();  // Private constructor
};

// Usage
AppConfig::instance().setMemorySize(cutie::MemorySize::Mem512K);
QString lastDir = AppConfig::instance().lastCartridgeDir();
```

### Settings Storage

QSettings stores values based on organization/app name set in `main.cpp`:

```cpp
app.setOrganizationName("CutieCoCo");
app.setApplicationName("CutieCoCo");
// Settings stored at:
// - macOS: ~/Library/Preferences/com.cutiecoco.CutieCoCo.plist
// - Windows: HKEY_CURRENT_USER\Software\CutieCoCo\CutieCoCo
// - Linux: ~/.config/CutieCoCo/CutieCoCo.conf
```

### Enum Serialization

Store enums as human-readable strings or integers:

```cpp
// Store as string (readable in config files)
QString cpuTypeToString(cutie::CpuType type) {
    switch (type) {
        case cutie::CpuType::MC6809: return "6809";
        case cutie::CpuType::HD6309: return "6309";
    }
}

// Store as integer (simple for memory sizes)
int memorySizeToInt(cutie::MemorySize size) {
    switch (size) {
        case cutie::MemorySize::Mem128K: return 128;
        case cutie::MemorySize::Mem512K: return 512;
        // etc.
    }
}
```

### Recent Files Management

Track recent files with a size limit:

```cpp
void AppConfig::addRecentCartridge(const QString& path) {
    QSettings settings;
    QStringList recent = settings.value("recentFiles/cartridges").toStringList();

    recent.removeAll(path);  // Remove if exists (will re-add at front)
    recent.prepend(path);    // Add to front

    while (recent.size() > MAX_RECENT_FILES) {
        recent.removeLast();
    }

    settings.setValue("recentFiles/cartridges", recent);
}
```

### Window State Persistence

Save and restore window geometry:

```cpp
// Save on close
void MainWindow::closeEvent(QCloseEvent* event) {
    AppConfig::instance().setWindowGeometry(saveGeometry());
    AppConfig::instance().setWindowState(QMainWindow::saveState());
    AppConfig::instance().sync();
    QMainWindow::closeEvent(event);
}

// Restore on open
void MainWindow::restoreWindowState() {
    QByteArray geometry = AppConfig::instance().windowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        adjustSize();  // Default to widget's size hint
    }
}
```

## Qt Settings Dialog Pattern

Use a tabbed dialog for organized settings:

```cpp
SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    auto* tabs = new QTabWidget(this);
    createCpuTab(tabs);      // CPU type, memory size
    createAudioTab(tabs);    // Sample rate
    createDisplayTab(tabs);  // Aspect ratio, scaling

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::accept() {
    saveSettings();  // Persist before closing
    QDialog::accept();
}
```

### ComboBox with Data

Store associated values with combo items:

```cpp
// Add items with data
m_cpuTypeCombo->addItem(tr("Motorola MC6809"), static_cast<int>(CpuType::MC6809));
m_cpuTypeCombo->addItem(tr("Hitachi HD6309"), static_cast<int>(CpuType::HD6309));

// Read current selection
auto type = static_cast<CpuType>(m_cpuTypeCombo->currentData().toInt());

// Set selection by data value
int idx = m_cpuTypeCombo->findData(static_cast<int>(config.cpuType()));
if (idx >= 0) m_cpuTypeCombo->setCurrentIndex(idx);
```

## Adding a New Platform

When adding a new platform (e.g., macOS native):

1. **Create directory structure:**
   ```
   platforms/newplatform/
     CMakeLists.txt
     include/
     src/
   ```

2. **Implement required components:**
   - Main entry point and window
   - Renderer (Metal, OpenGL, or platform-specific)
   - Audio backend (CoreAudio, WASAPI, etc.)
   - Input handling with character-based keyboard mapping

3. **Link against emulation library:**
   ```cmake
   target_link_libraries(myplatform PRIVATE cutie-emulation)
   ```

4. **Use CocoEmulator API:**
   ```cpp
   auto emulator = cutie::CocoEmulator::create(config);
   emulator->init();
   // Timer-driven frame loop
   emulator->runFrame();
   auto [pixels, size] = emulator->getFramebuffer();
   ```

5. **Update root CMakeLists.txt:**
   ```cmake
   option(CUTIECOCO_BUILD_NEWPLATFORM "Build new platform" OFF)
   if(CUTIECOCO_BUILD_NEWPLATFORM)
       add_subdirectory(platforms/newplatform)
   endif()
   ```

## Troubleshooting

### Build Fails After Adding File

1. **Check for cascading errors** - fix the first error only, rebuild
2. **Look for conflicting definitions** - stubs.h vs real implementations
3. **Check header includes** - missing `<vector>`, `<array>`, etc.
4. **Verify platform guards** - `#ifdef _WIN32` around Windows-specific code

### Missing Function at Link Time

The function exists in a file not yet added to CMakeLists.txt. Either:
1. Add the source file to the build
2. Create a stub in stubs.h

### Stub Not Working

Ensure the stub is in stubs.h BEFORE the file that needs it includes stubs.h. Include order:
```cpp
#include "defines.h"      // First - redirects to compat.h
#include "cutie/stubs.h"  // Second - provides stubs
// ... other includes
```

## Shared Platform Code

When code is duplicated between platforms (Qt and Windows), extract it to the emulation library.

### Character-to-CoCo Key Mapping

The `cutie::mapCharToCoco()` function in `keymapping.h` translates Unicode characters to CoCo key combinations. This handles the different shift mappings between PC and CoCo keyboards:

```cpp
#include "cutie/keymapping.h"

// In Qt (from QKeyEvent::text())
auto combo = cutie::mapCharToCoco(static_cast<char32_t>(text[0].unicode()));

// In Windows (from WM_CHAR)
auto combo = cutie::mapCharToCoco(static_cast<char32_t>(wcharValue));

if (combo) {
    if (combo->withShift) {
        keyboard.keyDown(CocoKey::Shift);
    }
    keyboard.keyDown(combo->key);
}
```

**Key mappings handled:**
- Letters: a-z (unshifted), A-Z (shifted)
- Numbers: 0-9 (unshifted)
- CoCo-specific shifted symbols: `"` → Shift+2, `'` → Shift+7, `*` → Shift+:, etc.

## Architectural Notes

### Stubs vs Interfaces

The codebase has two abstraction mechanisms:

1. **`stubs.h`** - Inline stub functions for legacy code compatibility
   - Used by legacy VCC emulation files that call Windows-specific functions
   - Provides sensible defaults (e.g., `GetMotorState()` returns 0)
   - Gradually being replaced as code is refactored

2. **`interfaces.h`** - Abstract C++ interfaces for platform abstraction
   - `IVideoOutput`, `IAudioOutput`, `IInputProvider`, `ICartridge`
   - Null implementations provided for testing
   - Used by new code following dependency injection pattern

**Migration strategy:** Refactor legacy code incrementally to use interfaces instead of stubs. The `CocoEmulator` class already uses a pull model where platforms call `getFramebuffer()` and `getAudioSamples()` rather than receiving callbacks.

### EmulationContext Pattern

The `EmulationContext` singleton (`emulation/include/cutie/context.h`) bridges the gap between stubs and interfaces. It allows platforms to inject real implementations while legacy code continues to use simple function calls.

**Architecture:**
```cpp
// Platform sets up implementations once at startup
auto& ctx = cutie::EmulationContext::instance();
ctx.setAudioOutput(&myAudioBackend);
ctx.setSystemRomPath("/path/to/roms");
ctx.setMessageHandler([](const char* msg, const char* title) {
    QMessageBox::warning(nullptr, title, msg);
});

// Legacy code calls stubs, which delegate to EmulationContext
GetFreeBlockCount();  // → vccContextGetAudioFreeBlocks() → ctx.audioOutput()->...
MessageBox(...);      // → vccContextShowMessage() → ctx.messageHandler()
PakGetSystemRomPath(); // → vccContextGetSystemRomPath() → ctx.systemRomPath()
```

**C-compatible wrappers:** The context provides `extern "C"` functions for use by legacy C-style code:
- `vccContextGetAudioFreeBlocks()` - Returns audio buffer free space
- `vccContextShowMessage(msg, title)` - Displays message to user
- `vccContextGetSystemRomPath()` - Returns path to system ROMs

**Default behaviors:**
- Audio: Returns 0 free blocks (no audio backend)
- Messages: Prints to stderr
- ROM path: Returns empty string (falls back to working directory)

**Usage in stubs.h:**
```cpp
inline int GetFreeBlockCount() {
    return vccContextGetAudioFreeBlocks();  // Delegates to context
}
```

This pattern allows gradual migration: stubs can be updated one at a time to delegate to the context, and platforms can inject real implementations as they're developed.

### Menu System

**Qt platform:** Uses native `QMenu`/`QAction` directly. The menus are defined in `mainwindow.cpp::createMenus()`.

**Windows platform:** Could potentially use the VCC menu builder from `dream-vcc/main:libcommon/ui/menu/`, but this requires:
1. Removing Windows.h dependency from menu headers
2. Creating platform-agnostic icon type
3. Implementing visitor pattern for Win32 HMENU

**Note:** The menu builder is most relevant when cartridge plugins need their own dynamic menus. For static application menus, native platform APIs are simpler.

### Testing Infrastructure

The project uses Catch2 for unit testing, fetched via CMake FetchContent.

**Build and run tests:**
```bash
cd build
cmake --build . --target cpu_tests
ctest --output-on-failure
```

**Test structure:**
```
tests/
  CMakeLists.txt          # Test target configuration
  cpu_test_harness.h      # CPUTestHarness class, CCFlag enum
  cpu_test_harness.cpp    # Test harness implementation
  mc6809_tests.cpp        # MC6809 instruction tests (32 tests)
```

**Adding new tests:** Add new `*_tests.cpp` files to `tests/CMakeLists.txt`.

### CPU Testing Notes

**CRITICAL:** The CoCo 3 MMU maps addresses $FF00-$FFFF to I/O ports, NOT RAM. This means:
- The reset vector at $FFFE-$FFFF reads from `port_read()`, not memory
- Writing test code to $FFFE won't work - it goes to I/O space
- Use `MC6809ForcePC(address)` to set PC directly instead of the reset mechanism

**Test harness pattern:**
```cpp
// Load test program to RAM (below $FF00)
harness.loadProgram(0x1000, {0x86, 0x42, 0x12});  // LDA #$42, NOP

// Set PC directly (bypasses reset vector)
harness.setPC(0x1000);

// Execute minimal cycles (2 is enough for most instructions)
harness.step();  // Internally calls MC6809Exec(2)

// Check results
auto state = harness.getState();
REQUIRE(state.A == 0x42);
```

**Common CPU test issues:**

| Issue | Cause | Fix |
|-------|-------|-----|
| PC reads garbage | Using reset vector in I/O space | Use `MC6809ForcePC()` |
| Multiple instructions execute | Too many cycles in step() | Use `MC6809Exec(2)` |
| state.D always 0 | `MC6809GetState()` not populating D | Add `regs.D = D_REG;` |
| GimeInit undefined | Missing include | Add `#include "tcc1014graphics.h"` |

**CPU testing approach:** Use test vectors from known-good emulators or hardware tests to verify instruction execution. The CPU files are large (mc6809.cpp: 74KB, hd6309.cpp: 150KB) so comprehensive testing is valuable but time-consuming.

### Windows Platform Status

The `platforms/windows/` skeleton is in place but needs:
- **Audio backend** (DREAM-VCC-q2i): Implement using WASAPI, XAudio2, or waveOut
- **Gamepad support** (DREAM-VCC-8w7): Implement using XInput or DirectInput
- **Dialogs and menus** (DREAM-VCC-u8k): Settings dialog, file dialogs

These tasks require Windows testing environment to verify.
