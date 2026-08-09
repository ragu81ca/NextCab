#include "StemmaRotaryInput.h"

#ifdef USE_STEMMA_ROTARY_ENCODER

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_seesaw.h>

#include "../../../WiTcontroller.h"

#ifndef STEMMA_ROTARY_I2C_ADDRESS
#define STEMMA_ROTARY_I2C_ADDRESS 0x36
#endif

#ifndef STEMMA_ROTARY_BUTTON_PIN
#define STEMMA_ROTARY_BUTTON_PIN 24
#endif

#ifndef STEMMA_ROTARY_COUNTS_PER_DETENT
#define STEMMA_ROTARY_COUNTS_PER_DETENT 1
#endif

#ifndef STEMMA_ROTARY_POLL_INTERVAL_MS
#define STEMMA_ROTARY_POLL_INTERVAL_MS 2
#endif

static Adafruit_seesaw stemmaEncoder;

void StemmaRotaryInput::debugProbeAddresses() {
    const uint8_t kMinAddr = 0x36;
    const uint8_t kMaxAddr = 0x3F;

    Serial.printf("[StemmaRotary] Probe seesaw addresses 0x%02X..0x%02X\n", kMinAddr, kMaxAddr);
    int found = 0;

    for (uint8_t addr = kMinAddr; addr <= kMaxAddr; ++addr) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.printf("[StemmaRotary] I2C device at 0x%02X\n", addr);
            found++;
        }
    }

    if (found == 0) {
        Serial.println("[StemmaRotary] No I2C devices detected in seesaw range");
    } else {
        Serial.println("[StemmaRotary] Probe complete");
    }
}

void StemmaRotaryInput::begin() {
    Serial.printf("[StemmaRotary] Starting, configured address=0x%02X\n", STEMMA_ROTARY_I2C_ADDRESS);

#if defined(USE_MAX17048) && (USE_MAX17048 == 1)
#if (STEMMA_ROTARY_I2C_ADDRESS == 0x36)
    Serial.println("[StemmaRotary] WARNING: STEMMA address 0x36 conflicts with MAX17048 on Feather S3.");
    Serial.println("[StemmaRotary] Move encoder to another address (for example 0x37 via A0 jumper).");
#endif
#endif

    if (!stemmaEncoder.begin(STEMMA_ROTARY_I2C_ADDRESS)) {
        Serial.printf("[StemmaRotary] No seesaw encoder at configured address 0x%02X\n", STEMMA_ROTARY_I2C_ADDRESS);
        Serial.println("[StemmaRotary] Tip: call StemmaRotaryInput::debugProbeAddresses() while troubleshooting");
        _ready = false;
        return;
    }

    stemmaEncoder.pinMode(STEMMA_ROTARY_BUTTON_PIN, INPUT_PULLUP);
    _lastRawPosition = stemmaEncoder.getEncoderPosition();
    _rawRemainder = 0;
    _ready = true;

    Serial.printf("[StemmaRotary] Ready addr=0x%02X startPos=%ld\n",
                  STEMMA_ROTARY_I2C_ADDRESS, _lastRawPosition);
}

void StemmaRotaryInput::poll() {
    if (!_ready) return;

    unsigned long now = millis();
    if (now - _lastPollMs < STEMMA_ROTARY_POLL_INTERVAL_MS) return;
    _lastPollMs = now;

    const unsigned long buttonBounceFilterMs = 120;
    const unsigned long prePressGuardMs = 25;
    const unsigned long holdThresholdMs = 500;
    const unsigned long doubleClickWindowMs = 400;
    const unsigned long minClickDurationMs = 50;

    long rawPosition = stemmaEncoder.getEncoderPosition();
    long rawDelta = rawPosition - _lastRawPosition;
    _lastRawPosition = rawPosition;

    if (rawDelta != 0) {
        _rawRemainder += (int)rawDelta;

        int detentDelta = 0;
        while (_rawRemainder >= STEMMA_ROTARY_COUNTS_PER_DETENT) {
            detentDelta++;
            _rawRemainder -= STEMMA_ROTARY_COUNTS_PER_DETENT;
        }
        while (_rawRemainder <= -STEMMA_ROTARY_COUNTS_PER_DETENT) {
            detentDelta--;
            _rawRemainder += STEMMA_ROTARY_COUNTS_PER_DETENT;
        }

        if (detentDelta != 0) {
            if (!encoderRotationClockwiseIsIncreaseSpeed) {
                detentDelta = -detentDelta;
            }

            if (now - _lastButtonEventMs > buttonBounceFilterMs) {
                if (_pendingDelta == 0) _pendingDeltaMs = now;
                _pendingDelta += detentDelta;
            }
        }
    }

    if (_pendingDelta != 0
        && now - _pendingDeltaMs >= prePressGuardMs
        && now - _lastButtonEventMs > buttonBounceFilterMs) {
        #if DEBUG
        Serial.print("[StemmaRotary] emit SpeedDelta="); Serial.println(_pendingDelta);
        #endif
        if (_dispatch) {
            InputEvent ev;
            ev.type = InputEventType::SpeedDelta;
            ev.ivalue = _pendingDelta;
            ev.timestamp = now;
            _dispatch(ev);
        }
        _pendingDelta = 0;
    }

    bool buttonPressed = (stemmaEncoder.digitalRead(STEMMA_ROTARY_BUTTON_PIN) == LOW);

    if (buttonPressed && !_buttonWasPressed) {
        #if DEBUG
        Serial.println("[StemmaRotary] button press");
        #endif
        _buttonPressStartMs = now;
        _buttonWasPressed = true;
        _longPressTriggered = false;
        _lastButtonEventMs = now;
        _pendingDelta = 0;
    }

    if (buttonPressed && _buttonWasPressed && !_longPressTriggered) {
        if (now - _buttonPressStartMs >= holdThresholdMs) {
            _longPressTriggered = true;
            _waitingForDoubleClick = false;
            _lastDoubleClickMs = 0;
            #if DEBUG
            Serial.println("[StemmaRotary] emit EncoderHold");
            #endif
            if (_dispatch) {
                InputEvent ev;
                ev.type = InputEventType::EncoderHold;
                ev.ivalue = 1;
                ev.timestamp = now;
                _dispatch(ev);
            }
        }
    }

    if (!buttonPressed && _buttonWasPressed) {
        _buttonWasPressed = false;
        unsigned long pressDuration = now - _buttonPressStartMs;
        _lastButtonEventMs = now;
        _pendingDelta = 0;
        #if DEBUG
        Serial.print("[StemmaRotary] button release ms="); Serial.println(pressDuration);
        #endif

        if (_longPressTriggered) {
            #if DEBUG
            Serial.println("[StemmaRotary] emit EncoderHoldRelease");
            #endif
            if (_dispatch) {
                InputEvent ev;
                ev.type = InputEventType::EncoderHoldRelease;
                ev.ivalue = 0;
                ev.timestamp = now;
                _dispatch(ev);
            }
        } else if (pressDuration >= minClickDurationMs && pressDuration < holdThresholdMs) {
            if (_waitingForDoubleClick && (now - _lastDoubleClickMs) < doubleClickWindowMs) {
                _waitingForDoubleClick = false;
                _lastDoubleClickMs = 0;
                #if DEBUG
                Serial.println("[StemmaRotary] emit EncoderDoubleClick");
                #endif
                if (_dispatch) {
                    InputEvent ev;
                    ev.type = InputEventType::EncoderDoubleClick;
                    ev.ivalue = 1;
                    ev.timestamp = now;
                    _dispatch(ev);
                }
            } else {
                _waitingForDoubleClick = true;
                _lastDoubleClickMs = now;
            }
        }
    }

    if (_waitingForDoubleClick && (now - _lastDoubleClickMs) >= doubleClickWindowMs) {
        _waitingForDoubleClick = false;
        _lastDoubleClickMs = 0;
        #if DEBUG
        Serial.println("[StemmaRotary] emit EncoderClick");
        #endif
        if (_dispatch) {
            InputEvent ev;
            ev.type = InputEventType::EncoderClick;
            ev.ivalue = 1;
            ev.timestamp = now;
            _dispatch(ev);
        }
    }
}

#endif
