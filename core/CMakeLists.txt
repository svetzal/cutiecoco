# DREAM Core Emulation Library
# Platform-independent CoCo 3 emulation

add_library(dream-core STATIC
    src/core.cpp
    src/audio.cpp
    src/emulation.cpp
    # Legacy emulation files - cleaned of Windows dependencies
    # TODO: Add these once inter-file dependencies are resolved:
    # - mc6809.cpp, hd6309.cpp (CPUs - need CPUAssertInterupt stubs)
    # - coco3.cpp (system - needs joystick stubs)
    # - mc6821.cpp (PIA - conflicts with stubs, needs resolution)
    # - iobus.cpp (I/O)
    # - tcc1014mmu.cpp (MMU)
    # - tcc1014registers.cpp (GIME registers)
    # - tcc1014graphics.cpp (GIME graphics)
)

target_include_directories(dream-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}  # For legacy headers (defines.h, etc.)
    ${CMAKE_SOURCE_DIR}/libcommon/include  # For vcc/utils/logger.h
)

target_compile_features(dream-core PUBLIC cxx_std_17)
