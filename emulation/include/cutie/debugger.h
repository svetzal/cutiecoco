#ifndef CUTIE_DEBUGGER_H
#define CUTIE_DEBUGGER_H

#include "types.h"

namespace cutie {

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

    // Screen event tracing - all no-ops
    void TraceCaptureScreenEvent(int /*event*/, int /*param*/) {}
    void TraceEmulatorCycle(int /*event*/, int /*state*/, double /*nanos*/, double /*p1*/, double /*p2*/, double /*p3*/, double /*p4*/) {}

    // Debugger update - no-op
    void Update() {}

    // Break/halt conditions
    bool Break_Enabled() const { return false; }
    bool Halt_Enabled() const { return false; }
};

} // namespace cutie

// VCC namespace compatibility
namespace VCC {
    using CPUState = cutie::CPUState;

    // Trace event types (stubs)
    namespace TraceEvent {
        constexpr int ScreenTopBorder = 0;
        constexpr int ScreenRender = 1;
        constexpr int ScreenBottomBorder = 2;
        constexpr int ScreenVSYNCLow = 3;
        constexpr int ScreenVSYNCHigh = 4;
        constexpr int ScreenHSYNCLow = 5;
        constexpr int ScreenHSYNCHigh = 6;
        constexpr int EmulatorCycle = 7;
    }

    namespace Debugger {
        using Debugger = cutie::Debugger;
    }
}

#endif // CUTIE_DEBUGGER_H
