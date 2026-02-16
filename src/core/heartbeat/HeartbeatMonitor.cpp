#include "HeartbeatMonitor.h"
#include "../protocol/WiThrottleDelegate.h" // For debug_printf

void HeartbeatMonitor::begin(unsigned long periodSeconds, bool enabled) {
    heartbeatPeriod = periodSeconds; 
    heartbeatCheckEnabled = enabled; 
    lastServerResponseTime = millis() / 1000; // Initialize to current time to prevent immediate timeout
    debug_printf("Heartbeat monitor started: period=%lus, timeout=%lus\n", periodSeconds, periodSeconds * 4);
}

void HeartbeatMonitor::noteActivity(unsigned long serverReportedSeconds, bool force) {
    // Use serverReportedSeconds if valid, else millis() fallback
    lastServerResponseTime = serverReportedSeconds;
    if (lastServerResponseTime == 0 || force) {
        lastServerResponseTime = millis() / 1000; // convert ms to seconds
    }
    // Uncomment for detailed activity logging:
    // debug_printf("Server activity noted at %lus\n", lastServerResponseTime);
}

void HeartbeatMonitor::loop() {
    if (!heartbeatCheckEnabled) return;
    unsigned long nowSecs = millis() / 1000;
    unsigned long timeSinceLastActivity = nowSecs - lastServerResponseTime;
    unsigned long timeoutThreshold = heartbeatPeriod * 4;
    
    // Log if we're getting close to timeout (within 10 seconds)
    if (timeSinceLastActivity > (timeoutThreshold - 10) && timeSinceLastActivity < timeoutThreshold) {
        debug_printf("Heartbeat warning: %lus since last activity (timeout at %lus)\n", 
                     timeSinceLastActivity, timeoutThreshold);
    }
    
    if (timeSinceLastActivity >= timeoutThreshold) {
        debug_printf("Heartbeat timeout! Last activity: %lus ago, threshold: %lus\n",
                     timeSinceLastActivity, timeoutThreshold);
        if (onTimeout) onTimeout();
    }
}

void HeartbeatMonitor::toggleEnabled() { heartbeatCheckEnabled = !heartbeatCheckEnabled; }

void HeartbeatMonitor::setEnabled(bool enabled) { heartbeatCheckEnabled = enabled; }

void HeartbeatMonitor::setOnTimeout(void (*cb)()) { onTimeout = cb; }

void HeartbeatMonitor::setPeriod(unsigned long seconds) {
    heartbeatPeriod = seconds; if (onPeriodChange) onPeriodChange(seconds);
}

void HeartbeatMonitor::setOnPeriodChange(void (*cb)(unsigned long)) {
    onPeriodChange = cb;
}
