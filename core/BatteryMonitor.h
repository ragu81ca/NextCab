// BatteryMonitor.h
// Extracted from monolithic WiTcontroller.ino to improve modularity.
// Handles periodic battery level checking and exposes last read values.

#pragma once

#include <Arduino.h>
#include "static.h"      // for ShowBattery enum & config macros
#include "Pangodream_18650_CL.h"

class BatteryMonitor {
public:
    BatteryMonitor();
    void begin();
    // Call frequently (e.g., from loop). Will internally rate-limit reads.
    void loop();

    bool enabled() const { return useBatteryTest; }
    int percent() const { return lastBatteryTestValue; }
    int lastAnalogRaw() const { return lastBatteryAnalogReadValue; }
    double lastCheckMillis() const { return lastBatteryCheckTime; }
    ShowBattery displayMode() const { return showBatteryTest; }
    void toggleDisplayMode();

    // Shutdown decision helper
    bool shouldSleepForLowBattery() const;

private:
    // Configuration derived from macros
    bool useBatteryTest;
    bool useBatteryPercentAsWellAsIcon;
    ShowBattery showBatteryTest; // current display mode

    int lastBatteryTestValue; 
    int lastBatteryAnalogReadValue;
    double lastBatteryCheckTime; // millis of last check

#if USE_BATTERY_TEST
    Pangodream_18650_CL battery;
#endif
};
