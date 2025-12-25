# CutieCoCo

A cross-platform Tandy Color Computer 3 (CoCo 3) emulator built with Qt.

> **Status:** Early development. The emulator boots to BASIC and accepts keyboard input, but many features are still in progress.

## About

CutieCoCo (pronounced "Cutie CoCo", a play on Qt) is a cross-platform CoCo 3 emulator. It aims to provide accurate emulation of the Tandy Color Computer 3 on macOS, Linux, and Windows.

### Features (Current)

- 128K CoCo 3 emulation
- Boots to Color BASIC
- Keyboard input
- 640x480 video output via OpenGL

### Planned Features

- Memory expansion up to 8192K
- Hitachi 6309 CPU support
- Floppy disk drive emulation
- Audio output
- Joystick support
- Cartridge/pak support

## Building

**Requirements:** Qt 6.x, CMake 3.20+

```bash
# Configure (adjust Qt path for your system)
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$HOME/Qt/6.10.1/macos"

# Build
cmake --build .

# Run (macOS)
open qt/cutiecoco.app
```

You will need a CoCo 3 ROM file (`coco3.rom`) placed in `build/system-roms/`.

## Heritage and Attribution

CutieCoCo is built on the work of many contributors to the CoCo emulation community:

- **VCC** (Virtual Color Computer) - The original Windows CoCo 3 emulator
- **VCCE** - VCC Extended, community improvements to VCC
- **DREAM-VCC** - Chet Simpson's fork focusing on code quality and accurate emulation

CutieCoCo uses the platform-independent emulation cores from DREAM-VCC, replacing all Windows-specific code with Qt for cross-platform support.

### Original Authors

The emulation core code retains copyright from the original VCC/VCCE/DREAM-VCC authors. See individual source files for specific attribution.

## License

CutieCoCo is licensed under the **GNU General Public License v3.0** (GPL-3), the same license used by VCC, VCCE, and DREAM-VCC. This ensures the emulator remains free software that anyone can use, study, modify, and share.

See the [COPYING](COPYING) file for the full license text.

## Contributing

This project is in early development. Contributions are welcome, but please be aware that the codebase is actively being restructured.
