// ThrottleManager.h - relocated and cleaned
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>
#include "../../static.h"

class ThrottleManager {
public:
	ThrottleManager();
	void begin(WiThrottleProtocol *proto);
	void speedUp(int throttle, int amt);
	void speedDown(int throttle, int amt);
	void speedSet(int throttle, int value);
	int getCurrentSpeed(int throttle) const; // raw 0-126 value
	int getDisplaySpeed(int throttle) const;
	void speedEstopAll();
	void speedEstopCurrent();
	void changeDirection(int throttle, Direction direction);
	void toggleDirection(int throttle);
	void nextThrottle();
	void selectThrottle(int idx);
	void changeNumberOfThrottles(bool increase);
	void toggleAdditionalMultiplier();
private:
	WiThrottleProtocol *proto { nullptr };
	void writeSpeedIfVisible(int throttle);
};
