# CutieCoCo Emulation Library
# Platform-independent CoCo 3 emulation

add_library(cutie-emulation STATIC
    src/core.cpp
    src/audio.cpp
    src/cocoemu.cpp
    src/emulation.cpp
    src/keyboard.cpp
    src/keymapping.cpp
    src/joystick.cpp
    src/cartridge.cpp
    # Legacy emulation files - cleaned of Windows dependencies
    mc6809.cpp
    hd6309.cpp
    tcc1014mmu.cpp
    tcc1014registers.cpp
    tcc1014graphics.cpp
    iobus.cpp
    mc6821.cpp
    coco3.cpp
)

target_include_directories(cutie-emulation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}  # For legacy headers (defines.h, etc.)
    ${CMAKE_CURRENT_SOURCE_DIR}/libcommon/include  # For vcc/utils/logger.h
)

target_compile_features(cutie-emulation PUBLIC cxx_std_17)
