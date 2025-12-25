#ifndef DREAM_DEBUGGER_H
#define DREAM_DEBUGGER_H

#include "types.h"

namespace dream {

// Stub debugger that does nothing - for initial Qt port
// Real debugger implementation will be added later
class Debugger {
public:
    // Breakpoint/stepping control
    bool IsHalted() const { return false; }
    bool IsStepping() const { return false; }
    void Halt() {}
    void Step() {}

    // Tracing control
    bool IsTracingEnabled() const { return false; }
    bool IsTracing() const { return false; }
    void TraceStart() {}
    void TraceStop() {}

    // Trace capture - all no-ops
    void TraceCaptureBefore(int /*cycles*/, const CPUState& /*state*/) {}
    void TraceCaptureAfter(int /*cycles*/, const CPUState& /*state*/) {}
    void TraceCaptureInterruptServicing(int /*type*/, int /*cycles*/, const CPUState& /*state*/) {}
    void TraceCaptureInterruptExecuting(int /*type*/, int /*cycles*/, const CPUState& /*state*/) {}
    void TraceCaptureInterruptMasked(int /*type*/, int /*cycles*/, const CPUState& /*state*/) {}
    void TraceCaptureInterruptRequest(unsigned char /*type*/, int /*cycles*/, const CPUState& /*state*/) {}

    // Break/halt conditions
    bool Break_Enabled() const { return false; }
    bool Halt_Enabled() const { return false; }
};

} // namespace dream

// VCC namespace compatibility
namespace VCC {
    using CPUState = dream::CPUState;

    namespace Debugger {
        using Debugger = dream::Debugger;
    }
}

#endif // DREAM_DEBUGGER_H
