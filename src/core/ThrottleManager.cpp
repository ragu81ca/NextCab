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
	momentum_.begin();
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
	
	// Send actual speed (potentially different if momentum active)
	int actualSpeed = momentum_.getActualSpeed(throttle);
	proto->setSpeed(tChar, actualSpeed);
	
	lastSpeedSentPerThrottle[throttle] = actualSpeed;
	lastSpeedSentTime = millis();
	lastSpeedSent = actualSpeed;
	lastSpeedThrottleIndex = throttle;
	// Input layer devices now handle their own internal state; no synchronization callback needed.
	oledRenderer.renderSpeed();
}

void ThrottleManager::updateMomentum() {
	if (!momentum_.isActive() || !proto) return;
	
	// Update momentum calculations
	momentum_.update();
	
	// Send updated actual speeds to locos if they've changed
	for (int throttle = 0; throttle < maxThrottles; throttle++) {
		if (proto->getNumberOfLocomotives(getMultiThrottleChar(throttle)) == 0) continue;
		
		int actualSpeed = momentum_.getActualSpeed(throttle);
		
		// Send if this throttle's speed changed since last update
		if (actualSpeed == lastSpeedSentPerThrottle[throttle]) {
			continue; // No change for this throttle
		}
		
		// Speed changed - send update
		proto->setSpeed(getMultiThrottleChar(throttle), actualSpeed);
		lastSpeedSentPerThrottle[throttle] = actualSpeed;
		lastSpeedSentTime = millis();
		lastSpeedSent = actualSpeed; // For backward compatibility
		lastSpeedThrottleIndex = throttle;
		
		// Update display if this is the visible throttle
		if (throttle == currentThrottleIndex) {
			writeSpeedIfVisible(throttle);
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
