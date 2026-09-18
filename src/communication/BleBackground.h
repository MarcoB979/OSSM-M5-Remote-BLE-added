#pragma once

#include <Arduino.h>

// Background BLE scanner/connector.
//
// The ESP32 UI loop must never block on BLE scans or connects (each can take
// hundreds of ms to seconds), so the addon probes and the toy-hub scan are
// handed to a low-priority FreeRTOS task. The main loop only enqueues jobs;
// the task runs them. Results that need UI-loop processing (e.g. the toy-hub
// buttplug registration) are handed back through the owning module's queue.
enum class BleBgJob : uint8_t {
    EjectProbe,
    FistITProbe,
    CoyoteProbe,
    ToyScan,     // arg = scan duration in ms
};

void bleBackgroundInit();                                   // call once from setup()
bool bleBackgroundRequest(BleBgJob job, uint32_t arg = 0);  // non-blocking enqueue

// Serialize access to the shared NimBLE scanner between the UI loop (OSSM
// connect) and the background task. Non-blocking try; false = scanner busy.
bool bleScanTryEnter();
void bleScanExit();

// RAII helper for the above.
class BleScanGuard {
public:
    BleScanGuard() : held_(bleScanTryEnter()) {}
    ~BleScanGuard() { if (held_) bleScanExit(); }
    operator bool() const { return held_; }
private:
    bool held_;
};
