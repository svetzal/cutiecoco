// DREAM Core Emulation Library
// Platform-independent CoCo 3 emulation core

#include "dream/compat.h"

namespace dream {

// Global emulator state instance
SystemState g_emu_state;

} // namespace dream

// Legacy global EmuState for VCC compatibility
SystemState EmuState;
