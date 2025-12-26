# CutieCoCo

A cross-platform Tandy Color Computer 3 (CoCo 3) emulator built with Qt.

> **Status:** Beta. The emulator boots to BASIC with keyboard, audio, and cartridge support. Some emulation glitches exist.

## About

CutieCoCo (pronounced "Cutie CoCo", a play on Qt) is a cross-platform CoCo 3 emulator. It aims to provide accurate emulation of the Tandy Color Computer 3 on macOS, Linux, and Windows.

### Features

- 128K-8192K CoCo 3 emulation
- Motorola 6809 and Hitachi 6309 CPU support
- Boots to Color BASIC
- Keyboard input with CoCo-correct key mapping
- Joystick emulation via keyboard numpad
- Audio output
- Cartridge loading (.rom, .ccc, .pak files)
- 640x480 video output via OpenGL
- Settings persistence

### Known Issues

- **No disk emulation:** Floppy disk drives are not yet implemented.
- **Cartridge auto-start:** Some cartridges may not auto-start after loading. If you see an `OK` prompt instead of the cartridge running, type `EXEC 49152` and press Enter to start it manually.

### Planned Features

- Native joystick/gamepad support
- Floppy disk drive emulation

## Building

**Requirements:** Qt 6.x, CMake 3.20+

```bash
# Configure (adjust Qt path for your system)
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$HOME/Qt/6.10.1/macos"

# Build
cmake --build .

# Run (macOS)
open platforms/qt/cutiecoco.app
```

You will need a CoCo 3 ROM file (`coco3.rom`) placed in `shared/system-roms/`.

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
