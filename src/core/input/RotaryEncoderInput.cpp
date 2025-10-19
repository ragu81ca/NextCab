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
                InputEvent gev; gev.type = InputEventType::SpeedDelta; gev.ivalue = (int)delta; gev.cvalue = 0; gev.timestamp = millis();
                _dispatch(gev);
            }
        }
    }

    // Button (debounced short press)
    if (rotaryEncoder.isEncoderButtonClicked()) {
        unsigned long now = millis();
        const unsigned long debounceMs = 200; // matches legacy ROTARY_ENCODER_DEBOUNCE_TIME
        if (now - _lastClickMs >= debounceMs) {
            _lastClickMs = now;
            if (_dispatch) {
                InputEvent gev; gev.type = InputEventType::EncoderClick; gev.ivalue = 1; gev.cvalue = 0; gev.timestamp = now;
                _dispatch(gev);
            }
            #if INPUT_DEBUG
            Serial.println("[Rotary] button click dispatched");
            #endif
        }
    }
}

void RotaryEncoderInput::handleISR() {
    if (s_instance) {
        rotaryEncoder.readEncoder_ISR();
    }
}
