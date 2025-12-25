# Agent Instructions

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DREAM is a Tandy Color Computer 3 (CoCo 3) emulator, forked from VCC. The project is currently being ported from Windows to cross-platform Qt. It emulates a 128K CoCo 3 with support for memory expansion up to 8192K, both Motorola 6809 and Hitachi 6309 CPUs.

## Current State: Qt Port In Progress

The `qt` branch contains the cross-platform port. Key changes:
- CMake replaces Visual Studio build system
- Qt6 replaces Windows-specific UI/graphics/audio
- Windows-specific code has been removed; legacy emulation files are being cleaned

**What's Done:**
- CMake build system with Qt6 dependencies
- Core library structure (`core/`)
- Qt application skeleton (`qt/`)
- Compatibility layer for legacy VCC types (`core/include/dream/`)
- Stub audio system and debugger
- EmulationThread with std::chrono timing (~60 FPS)
- Thread-safe frame callback to Qt display widget
- Stubs for removed Windows functionality (`core/include/dream/stubs.h`)
- coco3.cpp cleaned of Windows dependencies

**What Remains:**
- Clean remaining legacy files (tcc1014*.cpp, iobus.cpp, pakinterface.cpp)
- Integrate cleaned emulation files into core library
- Wire up GIME output to Qt framebuffer
- Implement keyboard input translation

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
open qt/dream-vcc.app
```

**Output:** `build/qt/dream-vcc.app` (macOS) or `build/qt/dream-vcc` (Linux)

## Architecture

### Directory Structure

```
core/                    # Platform-independent emulation library
  include/dream/
    types.h              # Core types (SystemState, Point, Size, Rect)
    compat.h             # VCC compatibility layer (includes types + debugger)
    debugger.h           # Stub debugger (no-op implementation)
    audio.h              # Audio interface + NullAudioSystem
    emulation.h          # EmulationThread class for frame timing
    stubs.h              # Stubs for removed Windows functionality
  src/
    core.cpp             # Global EmuState definition
    audio.cpp            # Audio factory
    emulation.cpp        # EmulationThread implementation

qt/                      # Qt application
  include/
    mainwindow.h
    emulatorwidget.h     # OpenGL display widget + emulation integration
  src/
    main.cpp
    mainwindow.cpp
    emulatorwidget.cpp

# Legacy files (being cleaned/integrated):
mc6809.cpp, hd6309.cpp   # CPU emulation (cleaned)
tcc1014*.cpp             # GIME graphics/MMU (needs cleanup)
coco3.cpp                # System coordination (cleaned)
mc6821.cpp               # PIA (I/O ports, cleaned)
```

### Compatibility Layer

The `core/include/dream/compat.h` header provides VCC-compatible types so legacy code can compile with minimal changes:

```cpp
// Legacy code includes this:
#include "defines.h"      // Redirects to dream/compat.h

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
   #include "dream/stubs.h"
   ```

2. **Keep defines.h** - it now redirects to `dream/compat.h`

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
   #include "dream/compat.h"   // For VCC::CPUState
   ```

6. **Use stubs.h for removed functionality:**
   The `core/include/dream/stubs.h` provides inline stubs for functions from deleted modules:
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

### VCC::CPUState vs dream::CPUState

The `VCC::CPUState` is an alias to `dream::CPUState` (defined in `types.h`). When adding CPU register fields:
- Add to `dream::CPUState` in `core/include/dream/types.h`
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
- New code should use `dream::` namespace
- Prefer `<cstdio>` over `<stdio.h>` style includes
- Use `bool` not `BOOL`
- Core emulation should have no platform dependencies

## EmulationThread Pattern

The `dream::EmulationThread` class (`core/include/dream/emulation.h`) runs the emulation loop in a separate thread:

```cpp
// Start emulation with a frame callback
m_emulation->start([this](const uint8_t* pixels, int width, int height) {
    onEmulatorFrame(pixels, width, height);
});

// Control emulation
m_emulation->pause();
m_emulation->resume();
m_emulation->reset();
m_emulation->stop();
```

**Key design points:**
- Uses `std::chrono::steady_clock` for precise frame timing (~59.923 Hz)
- Frame callback is invoked from the emulation thread
- Use mutex + signal to safely pass frame data to Qt main thread

## Qt Thread Safety

Qt UI must be updated from the main thread. The `EmulatorWidget` uses this pattern:

```cpp
// In emulatorwidget.h - signal for cross-thread notification
signals:
    void frameReady();

// In constructor - connect with QueuedConnection for thread safety
connect(this, &EmulatorWidget::frameReady,
        this, &EmulatorWidget::onFrameReady, Qt::QueuedConnection);

// Callback from emulation thread
void EmulatorWidget::onEmulatorFrame(const uint8_t* pixels, int w, int h) {
    {
        std::lock_guard<std::mutex> lock(m_framebufferMutex);
        std::memcpy(m_pendingFrame.data(), pixels, w * h * 4);
        m_frameUpdated = true;
    }
    emit frameReady();  // Signal main thread
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

## Integrating Legacy Emulation

To integrate a cleaned legacy file:

1. Add it to `core/CMakeLists.txt`:
   ```cmake
   add_library(dream-core STATIC
       src/core.cpp
       src/audio.cpp
       src/emulation.cpp
       ${CMAKE_SOURCE_DIR}/coco3.cpp  # Legacy file
   )
   ```

2. Ensure it includes `dream/stubs.h` for missing dependencies

3. The legacy code uses globals (`EmuState`) defined in `core/src/core.cpp`

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
| `no member named 'X' in 'CPUState'` | Missing field in types.h | Add to dream::CPUState |
| `'__declspec' attributes not enabled` | Windows-only export macro | Check libcommon/include/vcc/detail/exports.h has platform guards |

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
#include "dream/stubs.h"  // Second - provides stubs
// ... other includes
```
