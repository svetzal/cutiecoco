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

// Current CPU type (0 = 6809, 1 = 6309)
unsigned char CurrentCPUType = 0;

// Joystick ramp clock counter (used by CPU for joystick timing)
int JS_Ramp_Clock = 0;

// CPU execution stub - will be replaced when CPU files are integrated
int CPUExecStub(int /*cycles*/) { return 0; }
CPUExecFuncPtr CPUExec = CPUExecStub;
