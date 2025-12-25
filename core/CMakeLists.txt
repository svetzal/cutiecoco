# DREAM Core Emulation Library
# Platform-independent CoCo 3 emulation

add_library(dream-core STATIC
    src/core.cpp
    src/audio.cpp
    src/emulation.cpp
)

target_include_directories(dream-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}  # For legacy headers (defines.h, etc.)
)

target_compile_features(dream-core PUBLIC cxx_std_17)
