// BatteryMonitor.cpp
#include "BatteryMonitor.h"

BatteryMonitor::BatteryMonitor()
  : useBatteryTest(USE_BATTERY_TEST),
    useBatteryPercentAsWellAsIcon(USE_BATTERY_PERCENT_AS_WELL_AS_ICON),
#if USE_BATTERY_TEST
    showBatteryTest(
      USE_BATTERY_TEST ? (USE_BATTERY_PERCENT_AS_WELL_AS_ICON ? ICON_AND_PERCENT : ICON_ONLY) : NONE
    ),
#else
    showBatteryTest(NONE),
#endif
    lastBatteryTestValue(100),
    lastBatteryAnalogReadValue(0),
    lastBatteryCheckTime(-10000)
{
}

void BatteryMonitor::begin() {
#if USE_BATTERY_TEST
    // Construct battery monitor with configured pin & conversion factor
    Pangodream_18650_CL temp(BATTERY_TEST_PIN, BATTERY_CONVERSION_FACTOR);
    battery = temp; // copy
#endif
}

void BatteryMonitor::loop() {
#if USE_BATTERY_TEST
    if (!useBatteryTest) return;
    if (millis() - lastBatteryCheckTime < 10000) return; // every 10s
    lastBatteryCheckTime = millis();
    int batteryTestValue = battery.getBatteryChargeLevel();
    lastBatteryAnalogReadValue = battery.getLastAnalogReadValue();
    if (batteryTestValue != lastBatteryTestValue) {
        lastBatteryTestValue = batteryTestValue;
    }
#endif
}

void BatteryMonitor::toggleDisplayMode() {
    switch (showBatteryTest) {
        case ICON_ONLY: showBatteryTest = ICON_AND_PERCENT; break;
        case ICON_AND_PERCENT: showBatteryTest = NONE; break;
        case NONE: default: showBatteryTest = ICON_ONLY; break;
    }
}

bool BatteryMonitor::shouldSleepForLowBattery() const {
#if USE_BATTERY_TEST
    return useBatteryTest && (lastBatteryTestValue < USE_BATTERY_SLEEP_AT_PERCENT);
#else
    return false;
#endif
}
