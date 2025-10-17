// PotThrottleInput.cpp

#include "PotThrottleInput.h"
#include "InputEvents.h"
#include <Arduino.h>
#include "WiTcontroller.h"

// Reuse existing globals defined in sketch (we intentionally do not duplicate state here):
extern unsigned long lastThrottlePotReadTime;
extern int throttlePotPin;
extern int throttlePotNotch;            // current notch
extern bool throttlePotUseNotches;
extern int throttlePotNotchValues[];    // threshold raw values (size >=8)
extern int throttlePotNotchSpeeds[];    // corresponding speeds
extern int throttlePotTargetSpeed;
extern int lastThrottlePotValue;        // averaged value last applied
extern int lastThrottlePotHighValue;    // highest recent raw value
extern int lastThrottlePotValues[];     // smoothing buffer
extern int currentThrottleIndex;

PotThrottleInput::PotThrottleInput(ThrottleInputEventHandler legacyHandler)
    : _handler(legacyHandler) {}

void PotThrottleInput::begin() {
    // Nothing special yet.
}

void PotThrottleInput::loop() {
    // Emulate original throttlePot_loop(false) logic.
    const bool forceRead = false; // abstraction call doesn't currently force
    if ((millis() < lastThrottlePotReadTime + 100) && (!forceRead)) {
        return; // rate limit
    }
    lastThrottlePotReadTime = millis();

    int currentThrottlePotNotch = throttlePotNotch;
    int potValue = analogRead(throttlePotPin);
    // debug_print("Pot Value: "); debug_println(potValue);

    // Average with simple shift buffer
    // Original code used an inline array of 5 ints: {0,0,0,0,0}
    const int noElements = 5;
    int avgPotValue = 0;
    for (int i = 1; i < noElements; i++) {
        lastThrottlePotValues[i - 1] = lastThrottlePotValues[i];
        avgPotValue += lastThrottlePotValues[i - 1];
    }
    lastThrottlePotValues[noElements - 1] = potValue;
    avgPotValue = (avgPotValue + potValue) / noElements;

    // Highest recent value (retained for possible calibration logic)
    lastThrottlePotHighValue = -1;
    for (int i = 0; i < noElements; i++) {
        if (lastThrottlePotValues[i] > lastThrottlePotHighValue)
            lastThrottlePotHighValue = lastThrottlePotValues[i];
    }

    // Threshold: only react if significant change
    if ((avgPotValue < lastThrottlePotValue - 5) || (avgPotValue > lastThrottlePotValue + 5) || (forceRead)) {
        lastThrottlePotValue = avgPotValue;
        // debug_print("Avg Pot Value: "); debug_println(avgPotValue);

        int newSpeedToSet = -1;
        if (throttlePotUseNotches) {
            throttlePotNotch = 0;
            for (int i = 0; i < 8; i++) {
                if (avgPotValue < throttlePotNotchValues[i]) {
                    throttlePotTargetSpeed = throttlePotNotchSpeeds[i];
                    throttlePotNotch = i;
                    break;
                }
            }
            if (throttlePotNotch != currentThrottlePotNotch) {
                newSpeedToSet = throttlePotTargetSpeed;
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
            if (_genericHandler) {
                InputEvent gev; gev.type = InputEventType::SpeedAbsolute; gev.ivalue = newSpeedToSet; gev.cvalue = 0; gev.timestamp = millis();
                _genericHandler(gev);
            } else if (_handler) {
                ThrottleInputEvent evt{ThrottleInputEventType::SpeedSetAbsolute, newSpeedToSet};
                _handler(evt);
            }
        }
    }
}
