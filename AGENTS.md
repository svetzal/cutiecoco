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

**What Remains:**
- Clean remaining legacy files of Windows dependencies
- Integrate core emulation into Qt app
- Implement emulation loop, display, keyboard input

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
  src/
    core.cpp             # Global EmuState definition
    audio.cpp            # Audio factory

qt/                      # Qt application
  include/
    mainwindow.h
    emulatorwidget.h     # OpenGL display widget
  src/
    main.cpp
    mainwindow.cpp
    emulatorwidget.cpp

# Legacy files (being cleaned/integrated):
mc6809.cpp, hd6309.cpp   # CPU emulation
tcc1014*.cpp             # GIME graphics/MMU
coco3.cpp                # System coordination
mc6821.cpp               # PIA (keyboard, joystick, cassette I/O)
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
| `mc6809.cpp` | Motorola 6809 CPU | Cleaned |
| `hd6309.cpp` | Hitachi 6309 CPU | Cleaned |
| `mc6821.cpp/h` | PIA (I/O ports) | Cleaned |
| `coco3.cpp` | System emulation | Needs cleanup |
| `tcc1014graphics.cpp` | GIME video | Needs cleanup |
| `tcc1014mmu.cpp` | Memory management | Needs cleanup |
| `tcc1014registers.cpp` | GIME registers | Needs cleanup |
| `iobus.cpp` | I/O bus | Needs cleanup |
| `pakinterface.cpp` | Cartridge interface | Needs cleanup |

## Cleaning Legacy Files

When cleaning a legacy file of Windows dependencies:

1. **Remove Windows includes:**
   ```cpp
   // Remove these:
   #include <Windows.h>
   #include "Debugger.h"      // Deleted
   #include "Disassembler.h"  // Deleted
   #include "keyboard.h"      // Deleted
   #include "joystickinput.h" // Deleted
   #include "Cassette.h"      // Deleted
   #include "config.h"        // Deleted
   #include "audio.h"         // Old Windows version, deleted
   ```

2. **Keep defines.h** - it now redirects to `dream/compat.h`

3. **Replace Windows types:**
   - `BOOL` → `bool`
   - `TRUE/FALSE` → `true/false`
   - `HANDLE` → `FILE*` or appropriate type
   - `HWND`, `HINSTANCE` → `void*` (in SystemState)

4. **Remove MSVC pragmas:**
   ```cpp
   // Remove:
   #pragma warning( disable : 4800 )
   ```

5. **Stub removed functionality:**
   ```cpp
   // For removed keyboard/joystick/cassette:
   static unsigned char vccKeyboardGetScan(unsigned char) { return 0; }
   static unsigned short get_joyStick_input(int) { return 0; }
   ```

6. **Use C++ standard headers:**
   - `<stdio.h>` → `<cstdio>`
   - `<stdlib.h>` → `<cstdlib>`
   - `<string.h>` → `<cstring>`

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
