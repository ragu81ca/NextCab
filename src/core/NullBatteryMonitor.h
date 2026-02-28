// NullBatteryMonitor.h — No-op stub when no battery hardware is available.
//
// Always reports disabled / 100% / no sleep needed.  Used as the default
// when neither MAX17048 nor an analog voltage divider is configured.
#pragma once

#include "IBatteryMonitor.h"

class NullBatteryMonitor : public IBatteryMonitor {
public:
    bool begin()                     override { return false; }
    void loop()                      override {}
    bool enabled()           const   override { return false; }
    int  percent()           const   override { return 100; }
    int  percentRaw()        const   override { return 100; }
    float voltage()          const   override { return 0.0f; }
    double lastCheckMillis() const   override { return 0; }
    ShowBattery displayMode()const   override { return NONE; }
    void toggleDisplayMode()         override {}
    bool shouldSleepForLowBattery() const override { return false; }
    const char* name()       const   override { return "None"; }
};
