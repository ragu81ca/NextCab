// ThrottleManager.cpp - relocated
//#include removed WiTcontroller.h direct dependency to shrink globals usage
#include "ThrottleManager.h"
#include "OledRenderer.h"
#include "input/InputManager.h"
#include "../../WiTcontroller.h" // still needed for some macros/functions

extern InputManager inputManager;

ThrottleManager::ThrottleManager() {}

void ThrottleManager::begin(WiThrottleProtocol *p) {
	proto = p;
	currentThrottleIndex = 0;
	currentThrottleIndexChar = '0';
	for (int i=0; i<WIT_MAX_THROTTLES; i++) {
		currentSpeed[i] = 0;
		currentDirection[i] = Forward;
	}
	momentum_.begin(this, &sound_); // Pass both ThrottleManager and SoundController to momentum
	sound_.begin(this); // Initialize sound controller with reference to ThrottleManager
}


void ThrottleManager::writeSpeedIfVisible(int throttle) {
	if (inputManager.isInOperationMode() && (!menuIsShowing) && (throttle==currentThrottleIndex)) {
		oledRenderer.renderSpeed();
	}
}

void ThrottleManager::speedUp(int throttle, int amt) {
	if (!proto) return;
	if (proto->getNumberOfLocomotives(getMultiThrottleChar(throttle)) > 0) {
		speedSet(throttle, currentSpeed[throttle] + amt);
	}
}

void ThrottleManager::speedDown(int throttle, int amt) {
	if (!proto) return;
	if (proto->getNumberOfLocomotives(getMultiThrottleChar(throttle)) > 0) {
		speedSet(throttle, currentSpeed[throttle] - amt);
	}
}

void ThrottleManager::speedSet(int throttle, int value) {
	if (!proto) return;
	char tChar = getMultiThrottleChar(throttle);
	// If no locomotive yet, ignore request entirely (prevent overwriting future acquired speed)
	if (proto->getNumberOfLocomotives(tChar) == 0) { return; }
	int newSpeed = value;
	if (newSpeed > 126) newSpeed = 126;
	if (newSpeed < 0) newSpeed = 0;
	
	// Store as target speed (what user wants)
	currentSpeed[throttle] = newSpeed;
	momentum_.setTargetSpeed(throttle, newSpeed);
	
	// updateMomentum() handles all speed sending with rate limiting
	// When momentum is "off", it uses instant rates (999.0) for immediate response
	
	// Input layer devices now handle their own internal state; no synchronization callback needed.
	oledRenderer.renderSpeed();
}

void ThrottleManager::updateMomentum() {
	if (!proto) return;
	
	// Always update momentum - when "off", it uses instant rates (999.0)
	momentum_.update();
	
	// Update sound controller (handles non-blocking function pulses)
	sound_.update();
	
	// Send updated actual speeds to locos if they've changed
	for (int throttle = 0; throttle < maxThrottles; throttle++) {
		if (proto->getNumberOfLocomotives(getMultiThrottleChar(throttle)) == 0) continue;
		
		int actualSpeed = momentum_.getActualSpeed(throttle);
		
		// Check if train has stopped and there's a pending direction change
		// This handles the queued direction change after emergency braking to stop.
		if (actualSpeed == 0 && momentum_.hasPendingDirectionChange(throttle)) {
			// Get the pending target direction
			Direction newDir = momentum_.getPendingDirection(throttle);
			
			Serial.print("Applying queued direction change for T");
			Serial.print(throttle);
			Serial.print(" to ");
			Serial.println(newDir == Forward ? "Forward" : "Reverse");
			
			// Clear the pending flag and ensure brake is released
			momentum_.clearPendingDirectionChange(throttle);
			momentum_.setBraking(throttle, false);  // Explicitly release brake
			
			// Apply the direction change now that train is stopped
			changeDirection(throttle, newDir);
			
			// Note: changeDirection() already updates the display
		}
		
		// Rate-limited speed sending (prevents command station flooding while ensuring final speed is sent)
		unsigned long now = millis();
		bool shouldSend = false;
		
		// Check if speed has changed since last send
		if (actualSpeed != lastSpeedSentPerThrottle[throttle]) {
			// Speed changed — use tighter rate limit when momentum is off for snappier response
			unsigned long rateLimit = momentum_.isActive(throttle)
				? SPEED_RATE_LIMIT_MOMENTUM_MS : SPEED_RATE_LIMIT_DIRECT_MS;
			if (now - lastSpeedSetTime[throttle] >= rateLimit) {
				shouldSend = true;
			}
		}
		
		if (shouldSend) {
			// Send speed update to command station
			proto->setSpeed(getMultiThrottleChar(throttle), actualSpeed);
			lastSpeedSentPerThrottle[throttle] = actualSpeed;
			lastSpeedSetTime[throttle] = now; // Update rate limiting timestamp
			lastSpeedSentTime = millis();
			lastSpeedSent = actualSpeed; // For backward compatibility
			lastSpeedThrottleIndex = throttle;
			
			// Update display if this is the visible throttle
			if (throttle == currentThrottleIndex) {
				writeSpeedIfVisible(throttle);
			}
		}
	}
}

int ThrottleManager::getDisplaySpeed(int throttle) const {
	if (speedDisplayAsPercent) {
		float speed = currentSpeed[throttle];
		speed = speed / 126 * 100;
		int iSpeed = speed;
		if (iSpeed - speed >= 0.5) iSpeed++;
		return iSpeed;
	} else if (speedDisplayAs0to28) {
		float speed = currentSpeed[throttle];
		speed = speed / 126 * 28;
		int iSpeed = speed;
		if (iSpeed - speed >= 0.5) iSpeed++;
		return iSpeed;
	}
	return currentSpeed[throttle];
}

int ThrottleManager::getCurrentSpeed(int throttle) const { return currentSpeed[throttle]; }

void ThrottleManager::speedEstopAll() {
	if (!proto) return;
	proto->emergencyStop();
	// Bypass momentum for emergency stop
	momentum_.emergencyStopAll();
	for (int i=0; i<maxThrottles; i++) {
		speedSet(i,0);
		currentSpeed[i] = 0;
	}
	oledRenderer.renderSpeed();
}

void ThrottleManager::speedEstopCurrent() {
	if (!proto) return;
	proto->emergencyStop(currentThrottleIndexChar);
	// Bypass momentum for emergency stop
	momentum_.emergencyStop(currentThrottleIndex);
	speedSet(currentThrottleIndex,0);
	oledRenderer.renderSpeed();
}

void ThrottleManager::changeDirection(int throttle, Direction direction) {
	if (!proto) return;
	char tChar = getMultiThrottleChar(throttle);
	int locoCount = proto->getNumberOfLocomotives(tChar);
	if (locoCount <= 0) return;
	
	// Safety check: if momentum is active and train is moving, request the direction change.
	// This will queue it (brake to stop first) or cancel if already pending.
	// Returns true if the change was queued (train is moving).
	if (momentum_.isActive(throttle) && momentum_.requestDirectionChange(throttle, direction)) {
		// Direction change was queued - train will brake to stop first
		// Update the display direction immediately to show target direction
		currentDirection[throttle] = direction;
		
		Serial.print("Direction change queued for T");
		Serial.print(throttle);
		Serial.print(" to ");
		Serial.println(direction == Forward ? "Forward" : "Reverse");
		oledRenderer.renderSpeed(); // Update display to show new direction + braking indicator
		return;
	}
	
	// If we get here and there WAS a pending change, it was cancelled
	// Restore the original direction (direction train is actually still moving)
	if (momentum_.isActive(throttle) && !momentum_.hasPendingDirectionChange(throttle)) {
		// Check if we just cancelled (by detecting brake was just released)
		// If train is still moving, restore original direction for display
		int actualSpeed = momentum_.getActualSpeed(throttle);
		if (actualSpeed > 0) {
			currentDirection[throttle] = momentum_.getOriginalDirection(throttle);
			Serial.print("Direction change cancelled for T");
			Serial.print(throttle);
			Serial.print(" - restored to ");
			Serial.println(currentDirection[throttle] == Forward ? "Forward" : "Reverse");
			oledRenderer.renderSpeed();
			return;
		}
	}
	
	// Safe to change direction immediately (either momentum off, train stopped, or change was cancelled)
	currentDirection[throttle] = direction;
	if (locoCount == 1) {
		proto->setDirection(tChar, direction);
	} else {
		String leadLoco = proto->getLeadLocomotive(tChar);
		Direction leadDir = proto->getDirection(tChar, leadLoco);
		for (int i=1; i<locoCount; i++) {
			String loco = proto->getLocomotiveAtPosition(tChar, i);
			Direction locoDir = proto->getDirection(tChar, loco);
			if (locoDir == leadDir) proto->setDirection(tChar, loco, direction);
			else proto->setDirection(tChar, loco, (direction==Reverse)?Forward:Reverse);
		}
		proto->setDirection(tChar, leadLoco, direction);
	}
	oledRenderer.renderSpeed();
}

void ThrottleManager::toggleDirection(int throttle) {
	if (!proto) return;
	if (proto->getNumberOfLocomotives(getMultiThrottleChar(throttle)) > 0)
		changeDirection(throttle, (currentDirection[throttle]==Forward)?Reverse:Forward);
}

void ThrottleManager::setFunction(int throttle, int funcNum, bool state) {
	setFunction(throttle, funcNum, state, false); // Default: don't force
}

void ThrottleManager::setFunction(int throttle, int funcNum, bool state, bool force) {
	if (!proto) return;
	char tChar = getMultiThrottleChar(throttle);
	if (proto->getNumberOfLocomotives(tChar) == 0) return; // No loco on this throttle
	
	// Set function on the lead locomotive (first in consist)
	String leadLoco = proto->getLocomotiveAtPosition(tChar, 0);
	proto->setFunction(tChar, leadLoco, funcNum, state, force);
	
	// Update OLED to reflect function state change
	oledRenderer.renderSpeed();
	
	#ifdef MOMENTUM_DEBUG
	Serial.print("[ThrottleManager] T");
	Serial.print(throttle);
	Serial.print(" F");
	Serial.print(funcNum);
	Serial.print(state ? " ON" : " OFF");
	if (force) Serial.print(" (FORCED)");
	Serial.println();
	#endif
}

void ThrottleManager::nextThrottle() {
	int was = currentThrottleIndex;
	currentThrottleIndex++;
	if (currentThrottleIndex >= maxThrottles) currentThrottleIndex = 0;
	currentThrottleIndexChar = getMultiThrottleChar(currentThrottleIndex);
	if (currentThrottleIndex!=was) oledRenderer.renderSpeed();
}

void ThrottleManager::selectThrottle(int idx) {
	if (idx<0 || idx>=maxThrottles) return;
	int was = currentThrottleIndex;
	currentThrottleIndex = idx;
	currentThrottleIndexChar = getMultiThrottleChar(currentThrottleIndex);
	if (currentThrottleIndex!=was) oledRenderer.renderSpeed();
}

void ThrottleManager::changeNumberOfThrottles(bool increase) {
	if (increase) {
		maxThrottles++;
		if (maxThrottles>WIT_MAX_THROTTLES) maxThrottles = WIT_MAX_THROTTLES;
	} else {
		maxThrottles--;
		if (maxThrottles<1) maxThrottles = 1;
		if (currentThrottleIndex>=maxThrottles) nextThrottle();
	}
	oledRenderer.renderSpeed();
}

void ThrottleManager::cycleSpeedStep() {
	currentSpeedStepIndex++;
	if (currentSpeedStepIndex >= 3) currentSpeedStepIndex = 0;
	globalSpeedStep = speedStepLevels[currentSpeedStepIndex];
	oledRenderer.renderSpeed();
}


bool ThrottleManager::hasLocomotive(int throttle) const {
	if (!proto) return false;
	char tChar = getMultiThrottleChar(throttle);
	return proto->getNumberOfLocomotives(tChar) > 0;
}

// wrappers
void ThrottleManager::setMaxThrottles(int value) {
	maxThrottles = value;
	if (maxThrottles<1) maxThrottles=1;
	if (maxThrottles>WIT_MAX_THROTTLES) maxThrottles=WIT_MAX_THROTTLES;
	if (currentThrottleIndex>=maxThrottles) nextThrottle();
}

void ThrottleManager::setCurrentThrottleIndex(int idx) { selectThrottle(idx); }
void ThrottleManager::cycleNextThrottle() { nextThrottle(); }
// obsolete methods removed (resetSpeedStepMultiplier/applyAdditionalMultiplier/toggleAdditionalMultiplier)
