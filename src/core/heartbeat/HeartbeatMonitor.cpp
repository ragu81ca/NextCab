#include "HeartbeatMonitor.h"

void HeartbeatMonitor::begin(unsigned long periodSeconds, bool enabled) {
    heartbeatPeriod = periodSeconds; heartbeatCheckEnabled = enabled; lastServerResponseTime = 0; }

void HeartbeatMonitor::noteActivity(unsigned long serverReportedSeconds, bool force) {
    // Use serverReportedSeconds if valid, else millis() fallback
    lastServerResponseTime = serverReportedSeconds;
    if (lastServerResponseTime == 0 || force) {
        lastServerResponseTime = millis() / 1000; // convert ms to seconds
    }
}

void HeartbeatMonitor::loop() {
    if (!heartbeatCheckEnabled) return;
    unsigned long nowSecs = millis() / 1000;
    // Allow generous grace: 4x heartbeatPeriod (mirrors previous logic)
    if ((lastServerResponseTime + (heartbeatPeriod * 4)) < nowSecs) {
        if (onTimeout) onTimeout();
    }
}

void HeartbeatMonitor::toggleEnabled() { heartbeatCheckEnabled = !heartbeatCheckEnabled; }

void HeartbeatMonitor::setOnTimeout(void (*cb)()) { onTimeout = cb; }

void HeartbeatMonitor::setPeriod(unsigned long seconds) {
    heartbeatPeriod = seconds; if (onPeriodChange) onPeriodChange(seconds);
}

void HeartbeatMonitor::setOnPeriodChange(void (*cb)(unsigned long)) {
    onPeriodChange = cb;
}
