# DREAM Core Emulation Library
# Platform-independent CoCo 3 emulation

add_library(dream-core STATIC
    src/core.cpp
    src/audio.cpp
    src/emulation.cpp
    src/keyboard.cpp
    # Legacy emulation files - cleaned of Windows dependencies
    ${CMAKE_SOURCE_DIR}/mc6809.cpp
    ${CMAKE_SOURCE_DIR}/hd6309.cpp
    ${CMAKE_SOURCE_DIR}/tcc1014mmu.cpp
    ${CMAKE_SOURCE_DIR}/tcc1014registers.cpp
    ${CMAKE_SOURCE_DIR}/tcc1014graphics.cpp
    ${CMAKE_SOURCE_DIR}/iobus.cpp
    ${CMAKE_SOURCE_DIR}/mc6821.cpp
    ${CMAKE_SOURCE_DIR}/coco3.cpp
)

target_include_directories(dream-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}  # For legacy headers (defines.h, etc.)
    ${CMAKE_SOURCE_DIR}/libcommon/include  # For vcc/utils/logger.h
)

target_compile_features(dream-core PUBLIC cxx_std_17)
