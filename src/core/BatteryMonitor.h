// BatteryMonitor.h - relocated and cleaned
#pragma once
#include <Arduino.h>
#include "../../static.h"
#include "../../Pangodream_18650_CL.h"

class BatteryMonitor {
public:
	BatteryMonitor();
	void begin();
	void loop();
	bool enabled() const { return useBatteryTest; }
	int percent() const { return lastBatteryTestValue; }
	int lastAnalogRaw() const { return lastBatteryAnalogReadValue; }
	double lastCheckMillis() const { return lastBatteryCheckTime; }
	ShowBattery displayMode() const { return showBatteryTest; }
	void toggleDisplayMode();
	bool shouldSleepForLowBattery() const;
private:
	bool useBatteryTest;
	bool useBatteryPercentAsWellAsIcon;
	ShowBattery showBatteryTest;
	int lastBatteryTestValue;
	int lastBatteryAnalogReadValue;
	double lastBatteryCheckTime;
#if USE_BATTERY_TEST
	Pangodream_18650_CL battery;
#endif
};
