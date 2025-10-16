// BatteryMonitor.h - relocated and cleaned
// Ensure user configuration macros (e.g. USE_BATTERY_TEST) are visible in this translation unit.
#pragma once
#include <Arduino.h>
// Pull in user config first so any overrides apply before static defaults.
#include "../../config_buttons.h"
#include "../../static.h"
#include "../../Pangodream_18650_CL.h"

class BatteryMonitor {
public:
	BatteryMonitor();
	void begin();
	void loop();
	bool enabled() const { return useBatteryTest; }
	// Raw percent (unsmoothed) used for critical decisions like sleep threshold
	int percentRaw() const { return lastBatteryTestValue; }
	// Display percent (smoothed if enabled)
	int percent() const { return (BATTERY_SMOOTHING_ENABLED ? smoothedBatteryPercent : lastBatteryTestValue); }
	int lastAnalogRaw() const { return lastBatteryAnalogReadValue; }
	double lastCheckMillis() const { return lastBatteryCheckTime; }
	ShowBattery displayMode() const { return showBatteryTest; }
	void toggleDisplayMode();
	bool shouldSleepForLowBattery() const;
private:
	void initialSample();
	bool useBatteryTest;
	bool useBatteryPercentAsWellAsIcon;
	ShowBattery showBatteryTest;
	int lastBatteryTestValue;
	int lastBatteryAnalogReadValue;
	double lastBatteryCheckTime;
	int smoothedBatteryPercent; // EMA output
#if USE_BATTERY_TEST
	Pangodream_18650_CL battery;
#endif
};
