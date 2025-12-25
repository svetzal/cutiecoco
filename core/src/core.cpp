// DREAM Core Emulation Library
// Platform-independent CoCo 3 emulation core

#include "dream/compat.h"
#include "dream/stubs.h"

namespace dream {

// Global emulator state instance
SystemState g_emu_state;

} // namespace dream

// Legacy global EmuState for VCC compatibility
SystemState EmuState;

// CPU execution stub - will be replaced when CPU files are integrated
int CPUExecStub(int /*cycles*/) { return 0; }
CPUExecFuncPtr CPUExec = CPUExecStub;
