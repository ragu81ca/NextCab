// BatteryMonitor.cpp - relocated
// Dependencies (static.h, Pangodream_18650_CL) already pulled in via BatteryMonitor.h
#include "BatteryMonitor.h"

// Smoothing approach:
// Uses an exponential moving average (EMA) controlled by BATTERY_SMOOTHING_ALPHA (default 0.2).
// Displayed percentage comes from smoothedBatteryPercent if BATTERY_SMOOTHING_ENABLED is true.
// Raw percent (percentRaw()) is retained for low-battery sleep decisions to avoid masking critical drops.
// Initial sample seeds EMA to prevent a transient jump from 100 -> first reading.
BatteryMonitor::BatteryMonitor()
    : useBatteryTest(false),
      useBatteryPercentAsWellAsIcon(false),
      showBatteryTest(NONE),
      lastBatteryTestValue(100),
      lastBatteryAnalogReadValue(0),
    lastBatteryCheckTime(-10000),
    smoothedBatteryPercent(100) {}

void BatteryMonitor::begin() {
#if USE_BATTERY_TEST
    // Capture final macro values here (ensures include order in sketch does not affect runtime behavior)
    useBatteryTest = USE_BATTERY_TEST;
    useBatteryPercentAsWellAsIcon = USE_BATTERY_PERCENT_AS_WELL_AS_ICON;
    showBatteryTest = useBatteryTest ? (useBatteryPercentAsWellAsIcon ? ICON_AND_PERCENT : ICON_ONLY) : NONE;
    Pangodream_18650_CL temp(BATTERY_TEST_PIN, BATTERY_CONVERSION_FACTOR); battery = temp;
    initialSample(); // perform an immediate sample so lastCheckMillis() > 0 and icon can render right away
#endif
}

void BatteryMonitor::initialSample() {
#if USE_BATTERY_TEST
    if (!useBatteryTest) return;
    lastBatteryCheckTime = millis();
    lastBatteryTestValue = battery.getBatteryChargeLevel();
    lastBatteryAnalogReadValue = battery.getLastAnalogReadValue();
    smoothedBatteryPercent = lastBatteryTestValue; // seed EMA
#endif
}

void BatteryMonitor::loop() {
#if USE_BATTERY_TEST
    if (!useBatteryTest) return;
    // Sample every 10s; allow first sample done in begin()
    if (millis() - lastBatteryCheckTime < 10000) return;
    lastBatteryCheckTime = millis();
    int batteryTestValue = battery.getBatteryChargeLevel();
    lastBatteryAnalogReadValue = battery.getLastAnalogReadValue();
    if (batteryTestValue != lastBatteryTestValue) lastBatteryTestValue = batteryTestValue;
    // Exponential moving average smoothing
    if (BATTERY_SMOOTHING_ENABLED) {
        float alpha = BATTERY_SMOOTHING_ALPHA;
        float ema = (float)smoothedBatteryPercent * (1.0f - alpha) + (float)lastBatteryTestValue * alpha;
        smoothedBatteryPercent = (int)(ema + 0.5f); // round to nearest int
    } else {
        smoothedBatteryPercent = lastBatteryTestValue;
    }
#endif
}
void BatteryMonitor::toggleDisplayMode() { switch (showBatteryTest) { case ICON_ONLY: showBatteryTest = ICON_AND_PERCENT; break; case ICON_AND_PERCENT: showBatteryTest = NONE; break; case NONE: default: showBatteryTest = ICON_ONLY; break; } }
bool BatteryMonitor::shouldSleepForLowBattery() const {
#if USE_BATTERY_TEST
    return useBatteryTest && (lastBatteryTestValue < USE_BATTERY_SLEEP_AT_PERCENT);
#else
    return false;
#endif
}
