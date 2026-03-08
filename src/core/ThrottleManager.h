// ThrottleManager.h - relocated and cleaned
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>
#include "../../static.h"
#include "MomentumController.h"
#include "SoundController.h"

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
	void setFunction(int throttle, int funcNum, bool state); // Set function on/off
	void setFunction(int throttle, int funcNum, bool state, bool force); // Set function with force option
	void nextThrottle();
	void selectThrottle(int idx);
	void changeNumberOfThrottles(bool increase);
	void toggleAdditionalMultiplier();
	// Simplified: cycle through three predefined speed steps (base, second, third)
	void cycleSpeedStep();
	// Query whether a locomotive (or consist) is present on the given throttle
	bool hasLocomotive(int throttle) const;
	
	// Momentum control
	MomentumController& momentum() { return momentum_; }
	void updateMomentum(); // Called from main loop

	// Sound control
	SoundController& sound() { return sound_; }

	// Newly encapsulated scalar throttle state
	int getCurrentThrottleIndex() const { return currentThrottleIndex; }
	char getCurrentThrottleChar() const { return currentThrottleIndexChar; }
	int getMaxThrottles() const { return maxThrottles; }
	//int getSpeedStepCurrentMultiplier() const { return currentSpeedStepIndex+1; } // 1..3 logical level
	void setMaxThrottles(int value); // clamp 1..WIT_MAX_THROTTLES
	void setCurrentThrottleIndex(int idx); // safe select
	void cycleNextThrottle(); // alias for nextThrottle
	// Removed reset/apply; single cycling method now

	// last sent speed tracking accessors
	unsigned long getLastSpeedSentTime() const { return lastSpeedSentTime; }
	int getLastSpeedSent() const { return lastSpeedSent; }
	int getLastSpeedThrottleIndex() const { return lastSpeedThrottleIndex; }

	// Newly migrated arrays (with size up to 6) accessors
	int * speeds() { return currentSpeed; }
	Direction * directions() { return currentDirection; }
	// Global speed step (user preference) rather than per throttle
	Direction getDirection(int throttle) const { return currentDirection[throttle]; }
	int getSpeedStep() const { return globalSpeedStep; }
	void setGlobalSpeedStep(int step) { if (step<1) step=1; globalSpeedStep = step; }

	// ── Function state management ──────────────────────────────────────────
	// Accessors
	bool getFunctionState(int throttle, int funcNum) const;
	const String& getFunctionLabel(int throttle, int funcNum) const;
	int getFunctionFollow(int throttle, int funcNum) const;

	// Mutation (used by protocol callbacks)
	void setFunctionState(int throttle, int funcNum, bool state);
	void setFunctionLabelsFromRoster(int throttle, const String labels[MAX_FUNCTIONS]);

	// Reset helpers
	void resetFunctionStates(int throttle);
	void resetFunctionLabels(int throttle);
	void resetAllFunctionLabels();
	void resetAllFunctionFollow();

	// Consist-aware function dispatch
	// toggleFunction: for latching controls (keypad/menu) — checks current state to toggle
	void toggleFunction(int throttle, int funcNum, bool pressed);
	void toggleFunction(int throttle, int funcNum, bool pressed, bool force);
	// directFunction: for momentary controls (hard buttons) — passes press/release through
	void directFunction(int throttle, int funcNum, bool pressed);
	void directFunction(int throttle, int funcNum, bool pressed, bool force);

private:
	WiThrottleProtocol *proto { nullptr };
	void writeSpeedIfVisible(int throttle);
	
	// Momentum controller
	MomentumController momentum_;
	
	// Sound controller
	SoundController sound_;

	// Moved former globals (defined in WiTcontroller.ino) into this manager
	int currentThrottleIndex { 0 }; // which multi throttle is active
	char currentThrottleIndexChar { '0' }; // cached char
	int maxThrottles { MAX_THROTTLES }; // configurable number of throttles (<= WIT_MAX_THROTTLES)
	// New fixed step levels (configured from base macro and multipliers)
	int speedStepLevels[3] { speedStep, speedStep * speedStepAdditionalMultiplier, speedStep * speedStepAdditionalMultiplier * 2 };
	int currentSpeedStepIndex { 0 }; // 0..2
	unsigned long lastSpeedSentTime { 0 }; // debounce speed changes
	int lastSpeedSent { 0 }; // last speed value sent (for backward compatibility)
	int lastSpeedThrottleIndex { 0 }; // throttle index last speed belonged to
	
	// Rate limiting for when momentum is disabled (prevents command station flooding)
	unsigned long lastSpeedSetTime[WIT_MAX_THROTTLES] { 0 }; // Per-throttle rate limiting
	// Rate limits tuned to outbound command buffer drain rate (50ms per command).
	// Momentum ON:  100ms — smooth ramp, one command per drain cycle
	// Momentum OFF:  50ms — snappy response, minimal pipeline latency
	static constexpr unsigned long SPEED_RATE_LIMIT_MOMENTUM_MS = 100;
	static constexpr unsigned long SPEED_RATE_LIMIT_DIRECT_MS = 50;
	
	// Per-throttle tracking for momentum system
	int lastSpeedSentPerThrottle[WIT_MAX_THROTTLES] { 0 }; // Track last sent speed per throttle
	// Arrays migrated from sketch
	int currentSpeed[WIT_MAX_THROTTLES];
	Direction currentDirection[WIT_MAX_THROTTLES];
	int globalSpeedStep { speedStep }; // base step from macro; modified by multiplier logic

	// Function arrays (migrated from sketch globals)
	bool functionStates_[WIT_MAX_THROTTLES][MAX_FUNCTIONS];
	String functionLabels_[WIT_MAX_THROTTLES][MAX_FUNCTIONS];
	int functionFollow_[WIT_MAX_THROTTLES][MAX_FUNCTIONS];

	// Internal consist-aware dispatch helper
	void dispatchToConsist(int throttle, int funcNum, bool pressed, bool force);
};
