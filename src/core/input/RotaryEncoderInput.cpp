// RotaryEncoderInput.cpp

#include "RotaryEncoderInput.h"
#include "InputEvents.h"
#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>

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

RotaryEncoderInput::RotaryEncoderInput(ThrottleInputEventHandler handler)
    : _handler(handler) {}

void RotaryEncoderInput::begin() {
    s_instance = this;
    // Assume rotaryEncoder object already constructed in sketch; just configure
    rotaryEncoder.begin();
    rotaryEncoder.setup([](){ RotaryEncoderInput::handleISR(); });
    _lastEncoderValue = rotaryEncoder.readEncoder(); // baseline for delta only
}

void RotaryEncoderInput::loop() {
    // Poll for rotation
    if (rotaryEncoder.encoderChanged()) {
        long newValue = rotaryEncoder.readEncoder();
        long delta = newValue - _lastEncoderValue;
        _lastEncoderValue = newValue;
        if (delta != 0) {
            if (_genericHandler) {
                InputEvent gev; gev.type = InputEventType::SpeedDelta; gev.ivalue = (int)delta; gev.cvalue = 0; gev.timestamp = millis();
                _genericHandler(gev);
            } else if (_handler) {
                ThrottleInputEvent evt{ThrottleInputEventType::SpeedDelta, (int)delta};
                _handler(evt);
            }
        }
    }

    // Button (debounced short press)
    if (rotaryEncoder.isEncoderButtonClicked()) {
        unsigned long now = millis();
        const unsigned long debounceMs = 200; // matches legacy ROTARY_ENCODER_DEBOUNCE_TIME
        if (now - _lastClickMs >= debounceMs) {
            _lastClickMs = now;
            if (_genericHandler) {
                InputEvent gev; gev.type = InputEventType::EncoderClick; gev.ivalue = 1; gev.cvalue = 0; gev.timestamp = now;
                _genericHandler(gev);
            } else if (_handler) {
                ThrottleInputEvent evt{ThrottleInputEventType::ButtonShortPress, 1};
                _handler(evt);
            }
        }
    }
}

void RotaryEncoderInput::handleISR() {
    if (s_instance) {
        rotaryEncoder.readEncoder_ISR();
    }
}
