#pragma once
#include <Arduino.h>

class HeartbeatMonitor {
public:
    void begin(unsigned long periodSeconds, bool enabled);
    void noteActivity(unsigned long serverReportedSeconds, bool force = false);
    void loop();
    void toggleEnabled();
    void setPeriod(unsigned long seconds); // protocol may request heartbeat change

    bool enabled() const { return heartbeatCheckEnabled; }
    unsigned long period() const { return heartbeatPeriod; }
    unsigned long lastResponseSeconds() const { return lastServerResponseTime; }

    // Callback hooks (set by sketch)
    void setOnTimeout(void (*cb)());
    void setOnPeriodChange(void (*cb)(unsigned long));
private:
    unsigned long heartbeatPeriod { 10 }; // seconds
    unsigned long lastServerResponseTime { 0 }; // seconds since boot
    bool heartbeatCheckEnabled { true };

    void (*onTimeout)() { nullptr };   // invoked when timeout triggers
    void (*onPeriodChange)(unsigned long) { nullptr }; // invoked when period changes
};
