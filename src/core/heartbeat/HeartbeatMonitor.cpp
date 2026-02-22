#include "HeartbeatMonitor.h"
#include "../protocol/WiThrottleDelegate.h" // For debug_printf
#include "../../../static.h" // For DEFAULT_HEARTBEAT_PERIOD

void HeartbeatMonitor::begin(unsigned long periodSeconds, bool enabled) {
    heartbeatPeriod = periodSeconds; 
    heartbeatCheckEnabled = enabled; 
    timeoutInProgress = false; // Reset timeout guard
    lastServerResponseTime = millis() / 1000; // Initialize to current time to prevent immediate timeout
    debug_printf("Heartbeat monitor started: period=%lus, timeout=%lus\n", periodSeconds, periodSeconds * TIMEOUT_MULTIPLIER);
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
    if (timeoutInProgress) return; // Prevent multiple timeout firings
    
    unsigned long nowSecs = millis() / 1000;
    unsigned long timeSinceLastActivity = nowSecs - lastServerResponseTime;
    unsigned long timeoutThreshold = heartbeatPeriod * TIMEOUT_MULTIPLIER;
    
    // Log once per second if we're getting close to timeout (within 15 seconds)
    if (timeSinceLastActivity > (timeoutThreshold - 15) && timeSinceLastActivity < timeoutThreshold) {
        if (timeSinceLastActivity != lastWarningSecs_) {
            lastWarningSecs_ = timeSinceLastActivity;
            debug_printf("Heartbeat warning: %lus since last activity (timeout at %lus)\n", 
                         timeSinceLastActivity, timeoutThreshold);
        }
    }
    
    if (timeSinceLastActivity >= timeoutThreshold) {
        debug_printf("Heartbeat timeout! Last activity: %lus ago, threshold: %lus\n",
                     timeSinceLastActivity, timeoutThreshold);
        timeoutInProgress = true; // Guard against re-entry
        heartbeatCheckEnabled = false; // Disable monitoring immediately
        if (onTimeout) onTimeout();
    }
}

void HeartbeatMonitor::toggleEnabled() { heartbeatCheckEnabled = !heartbeatCheckEnabled; }

void HeartbeatMonitor::setEnabled(bool enabled) { 
    heartbeatCheckEnabled = enabled; 
    if (enabled) {
        timeoutInProgress = false; // Reset guard when re-enabling
        lastServerResponseTime = millis() / 1000; // Reset activity timer
    }
}

void HeartbeatMonitor::setOnTimeout(void (*cb)()) { onTimeout = cb; }

void HeartbeatMonitor::setPeriod(unsigned long seconds) {
    // Validate period - use DEFAULT_HEARTBEAT_PERIOD if server sends 0 or unreasonable values
    if (seconds == 0) {
        debug_printf("Server sent heartbeat period 0, using default %d seconds\n", DEFAULT_HEARTBEAT_PERIOD);
        seconds = DEFAULT_HEARTBEAT_PERIOD;
    } else if (seconds > 300) {
        // More than 5 minutes is probably an error, cap it
        debug_printf("Server sent unusually long heartbeat period %lu, capping at 300 seconds\n", seconds);
        seconds = 300;
    }
    // Only log and notify when the period actually changes (server echoes it with every heartbeat)
    if (seconds != heartbeatPeriod) {
        heartbeatPeriod = seconds;
        if (onPeriodChange) onPeriodChange(seconds);
    }
}

void HeartbeatMonitor::setOnPeriodChange(void (*cb)(unsigned long)) {
    onPeriodChange = cb;
}
