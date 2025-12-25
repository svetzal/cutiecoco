# Agent Instructions

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DREAM is a Tandy Color Computer 3 (CoCo 3) emulator for Windows, forked from VCC. The project aims to improve code quality and hardware emulation accuracy. It emulates a 128K CoCo 3 with support for memory expansion up to 8192K, both Motorola 6809 and Hitachi 6309 CPUs, and various expansion cartridges.

## Issue Tracking

This project uses **bd** (beads) for issue tracking. Run `bd onboard` to get started.

### Quick Reference

```bash
bd ready              # Find available work
bd show <id>          # View issue details
bd update <id> --status in_progress  # Claim work
bd close <id>         # Complete work
bd sync               # Sync with git
```

### Landing the Plane (Session Completion)

**When ending a work session**, you MUST complete ALL steps below. Work is NOT complete until `git push` succeeds.

**MANDATORY WORKFLOW:**

1. **File issues for remaining work** - Create issues for anything that needs follow-up
2. **Run quality gates** (if code changed) - Tests, linters, builds
3. **Update issue status** - Close finished work, update in-progress items
4. **PUSH TO REMOTE** - This is MANDATORY:
   ```bash
   git pull --rebase
   bd sync
   git push
   git status  # MUST show "up to date with origin"
   ```
5. **Clean up** - Clear stashes, prune remote branches
6. **Verify** - All changes committed AND pushed
7. **Hand off** - Provide context for next session

**CRITICAL RULES:**
- Work is NOT complete until `git push` succeeds
- NEVER stop before pushing - that leaves work stranded locally
- NEVER say "ready to push when you are" - YOU must push
- If push fails, resolve and retry until it succeeds

## Build Commands

**Requirements:** Visual Studio 2022 with C++ and MFC support, run from Developer Command Prompt.

```bash
# Full build (Release)
nuget restore
msbuild vcc.sln /m /p:Configuration=Release /p:Platform=x86

# Debug build
msbuild vcc.sln /m /p:Configuration=Debug /p:Platform=x86

# Build script (includes USE_LOGGING)
scripts\build-action.bat
```

**Output locations:**
- Main executable: `build/Win32/Release/bin/vcc.exe`
- Cartridge DLLs: `build/Win32/Release/bin/cartridges/*.dll`

## Testing

Tests use Google Test via NuGet package.

```bash
# Build and run tests
msbuild libcommon-test/libcommon-test.vcxproj /p:Configuration=Release /p:Platform=x86
build\Win32\Release\bin\libcommon-test.exe
```

## Architecture

### Plugin System

Cartridges are implemented as DLLs that plug into the emulator. Each cartridge:
1. Implements the `vcc::bus::cartridge` interface (libcommon/include/vcc/bus/cartridge.h)
2. Provides a `cartridge_driver` for hardware I/O operations
3. Exports standard entry points for the emulator to load

**Cartridge projects:** FD502 (floppy+Becker port), HardDisk, MPI (Multi-Pak), Ramdisk, SuperIDE, GMC (Game Master), acia, becker, orch90 (Orchestra-90)

### Core Components

| Component | Purpose |
|-----------|---------|
| `Vcc.cpp` | Main application, window management, emulation loop |
| `coco3.cpp` | System emulation, frame rendering coordination |
| `mc6809.cpp` / `hd6309.cpp` | CPU emulation (6809 and 6309) |
| `tcc1014graphics.cpp` | GIME video/graphics controller (TCC1014) |
| `tcc1014mmu.cpp` | Memory management unit |
| `config.cpp` | Configuration persistence |

### libcommon Shared Library

Contains reusable components used by both the main emulator and cartridges:
- `vcc/bus/` - Cartridge and expansion port interfaces
- `vcc/media/` - Disk image handling (JVC, VHD, DMK formats)
- `vcc/devices/` - ROM, PSG (SN76489), serial, RTC support
- `vcc/ui/` - Dialogs, menus, file selection helpers
- `vcc/utils/` - File operations, persistence, logging

### Build Configuration

Property files control common settings:
- `vcc-base.props` - Shared settings for all projects
- `vcc-debug.props` / `vcc-release.props` - Configuration-specific settings

Key preprocessor definitions: `DIRECTINPUT_VERSION=0x0800`, `_WIN32_WINNT=0x0500`

## Code Conventions

- Windows-specific APIs throughout (HWND, HINSTANCE, DirectX, DirectInput)
- Mix of C-style legacy code and modern C++ (gradual migration in progress)
- Cartridge DLLs must maintain C-compatible export interfaces
- Header-only includes in libcommon use `LIBCOMMON_EXPORT` macro for DLL visibility
