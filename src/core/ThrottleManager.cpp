// ThrottleManager.cpp - relocated
//#include removed WiTcontroller.h direct dependency to shrink globals usage
#include "ThrottleManager.h"
#include "Renderer.h"
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
	// Initialize function arrays
	resetAllFunctionLabels();
	resetAllFunctionFollow();
	for (int i = 0; i < WIT_MAX_THROTTLES; i++) resetFunctionStates(i);

	momentum_.begin(this, &sound_); // Pass both ThrottleManager and SoundController to momentum
	// Note: sound_.begin() is called separately from setup() after LocoManager is available
}


void ThrottleManager::writeSpeedIfVisible(int throttle) {
	if (inputManager.isInOperationMode() && (!menuIsShowing) && (throttle==currentThrottleIndex)) {
		renderer.renderSpeed();
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
	writeSpeedIfVisible(throttle);
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
	writeSpeedIfVisible(currentThrottleIndex);
}

void ThrottleManager::speedEstopCurrent() {
	if (!proto) return;
	proto->emergencyStop(currentThrottleIndexChar);
	// Bypass momentum for emergency stop
	momentum_.emergencyStop(currentThrottleIndex);
	speedSet(currentThrottleIndex,0);
	writeSpeedIfVisible(currentThrottleIndex);
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
		writeSpeedIfVisible(throttle); // Update display to show new direction + braking indicator
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
			writeSpeedIfVisible(throttle);
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
	writeSpeedIfVisible(throttle);
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
	writeSpeedIfVisible(throttle);
	
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
	if (currentThrottleIndex!=was) writeSpeedIfVisible(currentThrottleIndex);
}

void ThrottleManager::selectThrottle(int idx) {
	if (idx<0 || idx>=maxThrottles) return;
	int was = currentThrottleIndex;
	currentThrottleIndex = idx;
	currentThrottleIndexChar = getMultiThrottleChar(currentThrottleIndex);
	if (currentThrottleIndex!=was) writeSpeedIfVisible(currentThrottleIndex);
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
	writeSpeedIfVisible(currentThrottleIndex);
}

void ThrottleManager::cycleSpeedStep() {
	currentSpeedStepIndex++;
	if (currentSpeedStepIndex >= 3) currentSpeedStepIndex = 0;
	globalSpeedStep = speedStepLevels[currentSpeedStepIndex];
	writeSpeedIfVisible(currentThrottleIndex);
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

// ── Function state management ──────────────────────────────────────────

bool ThrottleManager::getFunctionState(int throttle, int funcNum) const {
	if (throttle < 0 || throttle >= WIT_MAX_THROTTLES || funcNum < 0 || funcNum >= MAX_FUNCTIONS) return false;
	return functionStates_[throttle][funcNum];
}

const String& ThrottleManager::getFunctionLabel(int throttle, int funcNum) const {
	static const String empty;
	if (throttle < 0 || throttle >= WIT_MAX_THROTTLES || funcNum < 0 || funcNum >= MAX_FUNCTIONS) return empty;
	return functionLabels_[throttle][funcNum];
}

int ThrottleManager::getFunctionFollow(int throttle, int funcNum) const {
	if (throttle < 0 || throttle >= WIT_MAX_THROTTLES || funcNum < 0 || funcNum >= MAX_FUNCTIONS) return CONSIST_LEAD_LOCO;
	return functionFollow_[throttle][funcNum];
}

void ThrottleManager::setFunctionState(int throttle, int funcNum, bool state) {
	if (throttle < 0 || throttle >= WIT_MAX_THROTTLES || funcNum < 0 || funcNum >= MAX_FUNCTIONS) return;
	functionStates_[throttle][funcNum] = state;
}

void ThrottleManager::setFunctionLabelsFromRoster(int throttle, const String labels[MAX_FUNCTIONS]) {
	if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
	for (int i = 0; i < MAX_FUNCTIONS; i++) {
		functionLabels_[throttle][i] = labels[i];
	}
}

void ThrottleManager::resetFunctionStates(int throttle) {
	if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
	for (int i = 0; i < MAX_FUNCTIONS; i++) {
		functionStates_[throttle][i] = false;
	}
}

void ThrottleManager::resetFunctionLabels(int throttle) {
	if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
	for (int i = 0; i < MAX_FUNCTIONS; i++) {
		functionLabels_[throttle][i] = "";
	}
}

void ThrottleManager::resetAllFunctionLabels() {
	for (int i = 0; i < WIT_MAX_THROTTLES; i++) {
		resetFunctionLabels(i);
	}
}

void ThrottleManager::resetAllFunctionFollow() {
	for (int i = 0; i < WIT_MAX_THROTTLES; i++) {
		functionFollow_[i][0]  = CONSIST_FUNCTION_FOLLOW_F0;
		functionFollow_[i][1]  = CONSIST_FUNCTION_FOLLOW_F1;
		functionFollow_[i][2]  = CONSIST_FUNCTION_FOLLOW_F2;
		functionFollow_[i][3]  = CONSIST_FUNCTION_FOLLOW_F3;
		functionFollow_[i][4]  = CONSIST_FUNCTION_FOLLOW_F4;
		functionFollow_[i][5]  = CONSIST_FUNCTION_FOLLOW_F5;
		functionFollow_[i][6]  = CONSIST_FUNCTION_FOLLOW_F6;
		functionFollow_[i][7]  = CONSIST_FUNCTION_FOLLOW_F7;
		functionFollow_[i][8]  = CONSIST_FUNCTION_FOLLOW_F8;
		functionFollow_[i][9]  = CONSIST_FUNCTION_FOLLOW_F9;
		functionFollow_[i][10] = CONSIST_FUNCTION_FOLLOW_F10;
		functionFollow_[i][11] = CONSIST_FUNCTION_FOLLOW_F11;
		functionFollow_[i][12] = CONSIST_FUNCTION_FOLLOW_F12;
		functionFollow_[i][13] = CONSIST_FUNCTION_FOLLOW_F13;
		functionFollow_[i][14] = CONSIST_FUNCTION_FOLLOW_F14;
		functionFollow_[i][15] = CONSIST_FUNCTION_FOLLOW_F15;
		functionFollow_[i][16] = CONSIST_FUNCTION_FOLLOW_F16;
		functionFollow_[i][17] = CONSIST_FUNCTION_FOLLOW_F17;
		functionFollow_[i][18] = CONSIST_FUNCTION_FOLLOW_F18;
		functionFollow_[i][19] = CONSIST_FUNCTION_FOLLOW_F19;
		functionFollow_[i][20] = CONSIST_FUNCTION_FOLLOW_F20;
		functionFollow_[i][21] = CONSIST_FUNCTION_FOLLOW_F21;
		functionFollow_[i][22] = CONSIST_FUNCTION_FOLLOW_F22;
		functionFollow_[i][23] = CONSIST_FUNCTION_FOLLOW_F23;
		functionFollow_[i][24] = CONSIST_FUNCTION_FOLLOW_F24;
		functionFollow_[i][25] = CONSIST_FUNCTION_FOLLOW_F25;
		functionFollow_[i][26] = CONSIST_FUNCTION_FOLLOW_F26;
		functionFollow_[i][27] = CONSIST_FUNCTION_FOLLOW_F27;
		functionFollow_[i][28] = CONSIST_FUNCTION_FOLLOW_F28;
		functionFollow_[i][29] = CONSIST_FUNCTION_FOLLOW_F29;
		functionFollow_[i][30] = CONSIST_FUNCTION_FOLLOW_F30;
		functionFollow_[i][31] = CONSIST_FUNCTION_FOLLOW_F31;
	}
}

// ── Consist-aware function dispatch ────────────────────────────────────

void ThrottleManager::dispatchToConsist(int throttle, int funcNum, bool pressed, bool force) {
	if (!proto) return;
	char tChar = getMultiThrottleChar(throttle);
	if (functionFollow_[throttle][funcNum] == CONSIST_LEAD_LOCO) {
		proto->setFunction(tChar, "", funcNum, pressed, force);
	} else { // CONSIST_ALL_LOCOS
		proto->setFunction(tChar, "*", funcNum, pressed, force);
	}
}

void ThrottleManager::directFunction(int throttle, int funcNum, bool pressed) {
	directFunction(throttle, funcNum, pressed, false);
}

void ThrottleManager::directFunction(int throttle, int funcNum, bool pressed, bool force) {
	if (!proto) return;
	char tChar = getMultiThrottleChar(throttle);
	if (proto->getNumberOfLocomotives(tChar) > 0) {
		dispatchToConsist(throttle, funcNum, pressed, force);
		writeSpeedIfVisible(throttle);
	}
}

void ThrottleManager::toggleFunction(int throttle, int funcNum, bool pressed) {
	toggleFunction(throttle, funcNum, pressed, false);
}

void ThrottleManager::toggleFunction(int throttle, int funcNum, bool pressed, bool force) {
	if (!proto) return;
	char tChar = getMultiThrottleChar(throttle);
	if (proto->getNumberOfLocomotives(tChar) > 0) {
		if (force) {
			dispatchToConsist(throttle, funcNum, true, force);
			if (functionStates_[throttle][funcNum]) {
				dispatchToConsist(throttle, funcNum, false, force);
			}
		} else {
			dispatchToConsist(throttle, funcNum, pressed, false);
		}
		writeSpeedIfVisible(throttle);
	}
}