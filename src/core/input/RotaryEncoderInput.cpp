// RotaryEncoderInput.cpp

#include "RotaryEncoderInput.h"
#include "InputEvents.h"
#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
// Optional debug gating (no acceleration logic needed now; using library call)
#ifndef INPUT_DEBUG
#define INPUT_DEBUG 0
#endif

// Reuse existing pins/macros from project root header.
#include "../ThrottleManager.h"
#include "WiTcontroller.h"

// We assume pin macros are already defined via project-wide config includes compiled before this TU.
// Provide safe fallbacks (will not be used if proper config headers already set them).
#ifndef ROTARY_ENCODER_A_PIN
#define ROTARY_ENCODER_A_PIN 12
#endif
#ifndef ROTARY_ENCODER_B_PIN
#define ROTARY_ENCODER_B_PIN 14
#endif
#ifndef ROTARY_ENCODER_BUTTON_PIN
#define ROTARY_ENCODER_BUTTON_PIN 13
#endif
#ifndef ROTARY_ENCODER_VCC_PIN
#define ROTARY_ENCODER_VCC_PIN -1
#endif
#ifndef ROTARY_ENCODER_STEPS
#define ROTARY_ENCODER_STEPS 2
#endif

// Local static hardware driver instance (encapsulated)
static AiEsp32RotaryEncoder rotaryEncoder(
    ROTARY_ENCODER_A_PIN,
    ROTARY_ENCODER_B_PIN,
    ROTARY_ENCODER_BUTTON_PIN,
    ROTARY_ENCODER_VCC_PIN,
    ROTARY_ENCODER_STEPS);

RotaryEncoderInput *RotaryEncoderInput::s_instance = nullptr;

void RotaryEncoderInput::begin() {
    s_instance = this;
    // Assume rotaryEncoder object already constructed in sketch; just configure
    rotaryEncoder.begin();
    // Disable acceleration by setting acceleration factor to 0 (library API)
    rotaryEncoder.setAcceleration(100); //Same as original WiTcontroller implementation
    rotaryEncoder.setup([](){ RotaryEncoderInput::handleISR(); });
    _lastEncoderValue = rotaryEncoder.readEncoder(); // baseline for delta only
}

void RotaryEncoderInput::poll() {
    // Poll for rotation
    if (rotaryEncoder.encoderChanged()) {
        unsigned long now = millis();
        const unsigned long buttonBounceFilterMs = 100; // Ignore rotation for 100ms after button events
        
        // Filter out mechanical bounce from button press/release
        if (now - _lastButtonEventMs > buttonBounceFilterMs) {
            long newValue = rotaryEncoder.readEncoder();
            long delta = newValue - _lastEncoderValue;
            _lastEncoderValue = newValue;
            if (delta != 0) {
                // Apply user-configured rotation direction: clockwise may be increase or decrease.
                // Legacy macro ENCODER_ROTATION_CLOCKWISE_IS_INCREASE_SPEED is defined globally.
                // Positive hardware delta corresponds to one physical direction; invert if necessary.
                #ifdef ENCODER_ROTATION_CLOCKWISE_IS_INCREASE_SPEED
                if (!encoderRotationClockwiseIsIncreaseSpeed) {
                    delta = -delta;
                }
                #endif
                #if WITCONTROLLER_DEBUG == 0
                Serial.print("[Rotary] raw delta:"); Serial.print(newValue - (_lastEncoderValue - delta)); Serial.print(" applied:"); Serial.println(delta);
                #endif
                if (_dispatch) {
                    InputEvent gev; gev.type = InputEventType::SpeedDelta; gev.ivalue = (int)delta; gev.cvalue = 0; gev.timestamp = now;
                    _dispatch(gev);
                }
            }
        } else {
            // Update the encoder value to stay in sync, but don't dispatch the event
            _lastEncoderValue = rotaryEncoder.readEncoder();
            #if INPUT_DEBUG
            Serial.println("[Rotary] rotation ignored - too soon after button event (mechanical bounce)");
            #endif
        }
    }

    // Button handling: detect double-click, hold, and single click
    unsigned long now = millis();
    bool buttonPressed = rotaryEncoder.isEncoderButtonDown();
    
    const unsigned long holdThresholdMs = 500; // 500ms hold for braking
    const unsigned long doubleClickWindowMs = 400;
    const unsigned long minClickDurationMs = 50; // debounce: ignore very short clicks
    
    // Detect button press start (for hold detection)
    if (buttonPressed && !_buttonWasPressed) {
        _buttonPressStartMs = now;
        _buttonWasPressed = true;
        _longPressTriggered = false;
        _lastButtonEventMs = now; // Track button event for rotation filtering
        #if INPUT_DEBUG
        Serial.println("[Rotary] button pressed");
        #endif
    }
    
    // Detect long press (hold) - trigger after holdThresholdMs
    if (buttonPressed && _buttonWasPressed && !_longPressTriggered) {
        if (now - _buttonPressStartMs >= holdThresholdMs) {
            _longPressTriggered = true;
            _waitingForDoubleClick = false; // Cancel any pending single-click
            _lastDoubleClickMs = 0;
            if (_dispatch) {
                InputEvent gev; gev.type = InputEventType::EncoderHold; gev.ivalue = 1; gev.cvalue = 0; gev.timestamp = now;
                _dispatch(gev);
            }
            #if INPUT_DEBUG
            Serial.println("[Rotary] hold detected - braking activated");
            #endif
        }
    }
    
    // Detect button release
    if (!buttonPressed && _buttonWasPressed) {
        _buttonWasPressed = false;
        unsigned long pressDuration = now - _buttonPressStartMs;
        _lastButtonEventMs = now; // Track button event for rotation filtering
        
        #if INPUT_DEBUG
        Serial.print("[Rotary] button released after "); Serial.print(pressDuration); Serial.println("ms");
        #endif
        
        // Release after long press
        if (_longPressTriggered) {
            if (_dispatch) {
                InputEvent gev; gev.type = InputEventType::EncoderHoldRelease; gev.ivalue = 0; gev.cvalue = 0; gev.timestamp = now;
                _dispatch(gev);
            }
            #if INPUT_DEBUG
            Serial.println("[Rotary] hold released - braking deactivated");
            #endif
        }
        // Normal click (not a long press) - but must be longer than debounce threshold
        else if (pressDuration >= minClickDurationMs && pressDuration < holdThresholdMs) {
            // Check if this is the second click of a double-click
            if (_waitingForDoubleClick && (now - _lastDoubleClickMs) < doubleClickWindowMs) {
                // Double-click confirmed!
                _waitingForDoubleClick = false;
                _lastDoubleClickMs = 0;
                if (_dispatch) {
                    InputEvent gev; gev.type = InputEventType::EncoderDoubleClick; gev.ivalue = 1; gev.cvalue = 0; gev.timestamp = now;
                    _dispatch(gev);
                }
                #if INPUT_DEBUG
                Serial.println("[Rotary] DOUBLE-CLICK confirmed - cycling momentum");
                #endif
            } else {
                // First click - start waiting for potential second click
                _waitingForDoubleClick = true;
                _lastDoubleClickMs = now;
                #if INPUT_DEBUG
                Serial.println("[Rotary] first click - waiting for potential double-click");
                #endif
            }
        }
        #if INPUT_DEBUG
        else if (pressDuration < minClickDurationMs) {
            Serial.println("[Rotary] click too short - ignored (debounce)");
        }
        #endif
    }
    
    // Delayed single-click dispatch (after double-click window expires without second click)
    if (_waitingForDoubleClick && (now - _lastDoubleClickMs) >= doubleClickWindowMs) {
        _waitingForDoubleClick = false;
        _lastDoubleClickMs = 0;
        if (_dispatch) {
            InputEvent gev; gev.type = InputEventType::EncoderClick; gev.ivalue = 1; gev.cvalue = 0; gev.timestamp = now;
            _dispatch(gev);
        }
        #if INPUT_DEBUG
        Serial.println("[Rotary] SINGLE-CLICK dispatched (double-click window expired)");
        #endif
    }
}

void RotaryEncoderInput::handleISR() {
    if (s_instance) {
        rotaryEncoder.readEncoder_ISR();
    }
}
