// ThrottleManager.h - relocated and cleaned
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>
#include "../../static.h"

// Define maximum number of throttles (can be overridden at compile time)
#ifndef WIT_MAX_THROTTLES
#define WIT_MAX_THROTTLES 6
#endif

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

	// Newly encapsulated scalar throttle state
	int getCurrentThrottleIndex() const { return currentThrottleIndex; }
	char getCurrentThrottleChar() const { return currentThrottleIndexChar; }
	int getMaxThrottles() const { return maxThrottles; }
	int getSpeedStepCurrentMultiplier() const { return speedStepCurrentMultiplier; }
	void setMaxThrottles(int value); // clamp 1..WIT_MAX_THROTTLES
	void setCurrentThrottleIndex(int idx); // safe select
	void cycleNextThrottle(); // alias for nextThrottle
	void resetSpeedStepMultiplier();
	void applyAdditionalMultiplier(); // toggle logic already exists; for clarity wrappers

	// last sent speed tracking accessors
	unsigned long getLastSpeedSentTime() const { return lastSpeedSentTime; }
	int getLastSpeedSent() const { return lastSpeedSent; }
	int getLastSpeedThrottleIndex() const { return lastSpeedThrottleIndex; }

	// Newly migrated arrays (with size up to 6) accessors
	int * speeds() { return currentSpeed; }
	Direction * directions() { return currentDirection; }
	int * speedSteps() { return currentSpeedStep; }
	Direction getDirection(int throttle) const { return currentDirection[throttle]; }
	int getSpeedStep(int throttle) const { return currentSpeedStep[throttle]; }
	void setSpeedStep(int throttle, int step) { currentSpeedStep[throttle] = step; }
private:
	WiThrottleProtocol *proto { nullptr };
	void writeSpeedIfVisible(int throttle);

	// Moved former globals (defined in WiTcontroller.ino) into this manager
	int currentThrottleIndex { 0 }; // which multi throttle is active
	char currentThrottleIndexChar { '0' }; // cached char
	int maxThrottles { MAX_THROTTLES }; // configurable number of throttles (<= WIT_MAX_THROTTLES)
	int speedStepCurrentMultiplier { 1 }; // multiplier for speed step
	unsigned long lastSpeedSentTime { 0 }; // debounce speed changes
	int lastSpeedSent { 0 }; // last speed value sent
	int lastSpeedThrottleIndex { 0 }; // throttle index last speed belonged to
	// Arrays migrated from sketch
	int currentSpeed[WIT_MAX_THROTTLES];
	Direction currentDirection[WIT_MAX_THROTTLES];
	int currentSpeedStep[WIT_MAX_THROTTLES];
};
