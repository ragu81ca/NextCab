// PotThrottleInput.cpp

#include "PotThrottleInput.h"
#include "InputEvents.h"
#include <Arduino.h>
#include "WiTcontroller.h"

// Config globals (TODO: move to Config.h in future refactor)
extern int throttlePotPin;
extern bool throttlePotUseNotches;
extern int throttlePotNotchValues[];    // threshold raw values (size >=8)
extern int throttlePotNotchSpeeds[];    // corresponding speeds

void PotThrottleInput::begin() {
    // Nothing special yet.
}

void PotThrottleInput::poll() {
    // Emulate original throttlePot_loop(false) logic.
    const bool forceRead = false; // abstraction call doesn't currently force
    if ((millis() < _lastReadTime + 100) && (!forceRead)) {
        return; // rate limit
    }
    _lastReadTime = millis();

    int previousNotch = _currentNotch;
    int potValue = analogRead(throttlePotPin);
    // debug_print("Pot Value: "); debug_println(potValue);

    // Average with simple shift buffer
    const int noElements = 5;
    int avgPotValue = 0;
    for (int i = 1; i < noElements; i++) {
        _smoothingBuffer[i - 1] = _smoothingBuffer[i];
        avgPotValue += _smoothingBuffer[i - 1];
    }
    _smoothingBuffer[noElements - 1] = potValue;
    avgPotValue = (avgPotValue + potValue) / noElements;

    // Highest recent value (retained for possible calibration logic)
    _lastHighValue = -1;
    for (int i = 0; i < noElements; i++) {
        if (_smoothingBuffer[i] > _lastHighValue)
            _lastHighValue = _smoothingBuffer[i];
    }

    // Threshold: only react if significant change
    if ((avgPotValue < _lastValue - 5) || (avgPotValue > _lastValue + 5) || (forceRead)) {
        _lastValue = avgPotValue;
        // debug_print("Avg Pot Value: "); debug_println(avgPotValue);

        int newSpeedToSet = -1;
        if (throttlePotUseNotches) {
            _currentNotch = 0;
            for (int i = 0; i < 8; i++) {
                if (avgPotValue < throttlePotNotchValues[i]) {
                    _targetSpeed = throttlePotNotchSpeeds[i];
                    _currentNotch = i;
                    break;
                }
            }
            if (_currentNotch != previousNotch) {
                newSpeedToSet = _targetSpeed;
            }
        } else {
            double newSpeed = potValue - throttlePotNotchValues[0];
            double range = (double)(throttlePotNotchValues[7] - throttlePotNotchValues[0]);
            if (range <= 0) range = 1; // guard divide by zero
            newSpeed = newSpeed / range;
            newSpeed = newSpeed * 127.0;
            if (newSpeed < 0) newSpeed = 0; else if (newSpeed > 127) newSpeed = 127;
            newSpeedToSet = (int)newSpeed;
        }

        if (newSpeedToSet >= 0 && newSpeedToSet != _lastSpeed) {
            _lastSpeed = newSpeedToSet;
            if (_dispatch) {
                InputEvent gev; gev.type = InputEventType::SpeedAbsolute; gev.ivalue = newSpeedToSet; gev.cvalue = 0; gev.timestamp = millis();
                _dispatch(gev);
            }
        }
    }
}
