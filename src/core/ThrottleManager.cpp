// ThrottleManager.cpp - relocated
//#include removed WiTcontroller.h direct dependency to shrink globals usage
#include "ThrottleManager.h"
#include "OledRenderer.h"
#include "input/ThrottleInputManager.h" // need full type for method call
#include "../../WiTcontroller.h" // still needed for some macros/functions

ThrottleManager::ThrottleManager() {}

void ThrottleManager::begin(WiThrottleProtocol *p) {
	proto = p;
	currentThrottleIndex = 0;
	currentThrottleIndexChar = '0';
	for (int i=0; i<WIT_MAX_THROTTLES; i++) {
		currentSpeed[i] = 0;
		currentDirection[i] = Forward;
	}
}

void ThrottleManager::setInputManager(ThrottleInputManager *mgr) {
	inputMgr = mgr;
}

void ThrottleManager::writeSpeedIfVisible(int throttle) {
	if ((keypadUseType == KEYPAD_USE_OPERATION) && (!menuIsShowing) && (throttle==currentThrottleIndex)) {
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
	if (proto->getNumberOfLocomotives(tChar) == 0) return;
	int newSpeed = value;
	if (newSpeed > 126) newSpeed = 126;
	if (newSpeed < 0) newSpeed = 0;
	proto->setSpeed(tChar, newSpeed);
	currentSpeed[throttle] = newSpeed;
	lastSpeedSentTime = millis();
	lastSpeedSent = newSpeed;
	lastSpeedThrottleIndex = throttle;
	// Input layer no longer requires synchronization for rotary; pot absolute still cached.
	if (inputMgr) inputMgr->notifySpeedExternallySet(newSpeed); // retains pot cache (no rotary baseline)
	oledRenderer.renderSpeed();
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
	for (int i=0; i<maxThrottles; i++) {
		speedSet(i,0);
		currentSpeed[i] = 0;
	}
	oledRenderer.renderSpeed();
}

void ThrottleManager::speedEstopCurrent() {
	if (!proto) return;
	proto->emergencyStop(currentThrottleIndexChar);
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
