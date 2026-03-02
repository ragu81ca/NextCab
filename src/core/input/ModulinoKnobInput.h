// ModulinoKnobInput.h – Arduino Modulino Knob (I2C rotary encoder + button)
// Lightweight driver that talks the Modulino I2C protocol directly,
// without pulling in the full Arduino_Modulino library and its many
// sensor dependencies.
//
// Protocol (same as ModulinoKnob class in official library):
//   Read 3 bytes: [posLSB, posMSB, pressed]
//   Write 2 bytes: [valueLSB, valueMSB]  (set position counter)
//
// Default I2C address: 0x3B (alternate 0x3A).
// The Modulino library stores these internally as 0x76/0x74 and divides by 2
// to get the actual 7-bit I2C bus address.
// Emits: SpeedDelta, EncoderClick, EncoderDoubleClick, EncoderHold, EncoderHoldRelease
#pragma once

#include <Wire.h>
#include "IInputDevice.h"
#include "InputEvents.h"

#ifndef MODULINO_KNOB_ADDRESS
  #define MODULINO_KNOB_ADDRESS 0x3B  // 0x76 / 2 — real 7-bit I2C address
#endif
#ifndef MODULINO_KNOB_ALT_ADDRESS
  #define MODULINO_KNOB_ALT_ADDRESS 0x3A  // 0x74 / 2
#endif

// Maximum encoder ticks to emit in a single poll cycle.
// Larger deltas are clamped to this value so speed ramps smoothly.
#ifndef MAX_DELTA_PER_POLL
  #define MAX_DELTA_PER_POLL 4
#endif

// Minimum milliseconds between I2C reads of the knob.
// The main loop runs at 1000+ Hz on ESP32-S3, but polling the Modulino
// that fast increases the chance of torn reads (the slave MCU's encoder
// ISR can update the position non-atomically mid-I2C-transfer).
// 10 ms → 100 Hz is more than responsive enough for a rotary encoder.
#ifndef KNOB_POLL_INTERVAL_MS
  #define KNOB_POLL_INTERVAL_MS 10
#endif

class ModulinoKnobInput : public IInputDevice {
public:
    explicit ModulinoKnobInput(IInputDevice::DispatchFn dispatchFn,
                               TwoWire &wire = Wire,
                               uint8_t address = MODULINO_KNOB_ADDRESS)
        : _dispatch(dispatchFn), _wire(wire), _address(address) {}

    void begin() override {
        // Discover the knob at both known 7-bit I2C addresses
        _ready = false;
        uint8_t addrs[] = { _address, MODULINO_KNOB_ALT_ADDRESS };
        Serial.println("[ModulinoKnob] Scanning for knob...");
        for (uint8_t a : addrs) {
            _wire.beginTransmission(a);
            uint8_t err = _wire.endTransmission();
            Serial.printf("[ModulinoKnob]   0x%02X -> %s\n", a, err == 0 ? "ACK" : "NACK");
            if (err == 0) {
                _address = a;
                _ready = true;
                break;
            }
        }
        if (_ready) {
            // Read initial position to establish baseline
            _lastPosition = readPosition();
            Serial.printf("[ModulinoKnob] Found at 0x%02X  pos=%d\n", _address, _lastPosition);
        } else {
            Serial.printf("[ModulinoKnob] NOT found at 0x%02X or 0x%02X – disabled\n",
                          MODULINO_KNOB_ADDRESS, MODULINO_KNOB_ALT_ADDRESS);
        }
    }

    void poll() override {
        if (!_ready) return;

        unsigned long now = millis();

        // Rate-limit I2C reads to reduce torn-read probability.
        if (now - _lastPollMs < KNOB_POLL_INTERVAL_MS) return;
        _lastPollMs = now;

        // ── Double-read validation ──
        // Read the knob twice in quick succession.  If both positions agree
        // (within ±1 tick) the data is clean.  If they disagree, one or both
        // reads caught the slave MCU mid-update — discard this entire cycle.
        int16_t pos1 = 0, pos2 = 0;
        bool pr1 = false, pr2 = false;
        if (!readState(pos1, pr1)) return;
        if (!readState(pos2, pr2)) return;

        int16_t readDiff = (int16_t)(pos2 - pos1);
        if (readDiff > 1 || readDiff < -1) {
            // Reads disagree — torn read detected, skip this cycle.
            // Do NOT update _lastPosition so next poll stays relative to
            // the last known-good baseline.
            Serial.printf("[Knob] TORN READ: read1=%d read2=%d diff=%d — skipped\n",
                          pos1, pos2, readDiff);
            return;
        }

        // Both reads agree — use the second (most recent) one
        int16_t pos = pos2;
        bool pressed = pr2;

        // ── Rotation ──
        int16_t rawDelta = (int16_t)(pos - _lastPosition);

        if (rawDelta != 0) {
            _lastPosition = pos; // update baseline only on validated reads

            // Filter rotation noise during / after button events
            if (now - _lastButtonEventMs > _bounceFilterMs) {
                // Clamp to ±MAX_DELTA_PER_POLL for smooth ramping
                int16_t emit = rawDelta;
                if (emit > MAX_DELTA_PER_POLL)  emit = MAX_DELTA_PER_POLL;
                if (emit < -MAX_DELTA_PER_POLL) emit = -MAX_DELTA_PER_POLL;

                // Buffer the delta instead of dispatching immediately.
                // This lets us discard it if the user is actually pressing
                // the button and the shaft wobbled right before contact.
                if (_pendingDelta == 0) _pendingDeltaMs = now;
                _pendingDelta += emit;
            }
            // else: discard movement during button-bounce window
        }

        // ── Dispatch buffered rotation after guard delay ──
        // Held for kPrePressGuardMs so a button press arriving in the
        // next poll cycle can cancel it.
        if (_pendingDelta != 0
            && now - _pendingDeltaMs >= _prePressGuardMs
            && now - _lastButtonEventMs > _bounceFilterMs) {
            Serial.printf("[Knob] pos=%d emit=%d\n", _lastPosition, _pendingDelta);
            if (_dispatch) {
                InputEvent ev;
                ev.type  = InputEventType::SpeedDelta;
                ev.ivalue = (int)_pendingDelta;
                ev.timestamp = now;
                _dispatch(ev);
            }
            _pendingDelta = 0;
        }

        // ── Button: detect press start ──
        if (pressed && !_buttonWasPressed) {
            _buttonWasPressed = true;
            _buttonPressStartMs = now;
            _longPressTriggered = false;
            _lastButtonEventMs = now;
            _pendingDelta = 0; // discard any buffered pre-press rotation
        }

        // ── Hold detection ──
        if (pressed && _buttonWasPressed && !_longPressTriggered) {
            if (now - _buttonPressStartMs >= _holdThresholdMs) {
                _longPressTriggered = true;
                _waitingForDoubleClick = false;
                if (_dispatch) {
                    InputEvent ev;
                    ev.type = InputEventType::EncoderHold;
                    ev.ivalue = 1;
                    ev.timestamp = now;
                    _dispatch(ev);
                }
            }
        }

        // ── Release ──
        if (!pressed && _buttonWasPressed) {
            _buttonWasPressed = false;
            unsigned long dur = now - _buttonPressStartMs;
            _lastButtonEventMs = now;
            _pendingDelta = 0; // discard any post-release rotation

            if (_longPressTriggered) {
                // Release after hold
                if (_dispatch) {
                    InputEvent ev;
                    ev.type = InputEventType::EncoderHoldRelease;
                    ev.ivalue = 0;
                    ev.timestamp = now;
                    _dispatch(ev);
                }
            } else if (dur >= _minClickMs && dur < _holdThresholdMs) {
                // Normal click — check double-click
                if (_waitingForDoubleClick && (now - _lastClickMs) < _doubleClickWindowMs) {
                    _waitingForDoubleClick = false;
                    if (_dispatch) {
                        InputEvent ev;
                        ev.type = InputEventType::EncoderDoubleClick;
                        ev.ivalue = 1;
                        ev.timestamp = now;
                        _dispatch(ev);
                    }
                } else {
                    _waitingForDoubleClick = true;
                    _lastClickMs = now;
                }
            }
        }

        // ── Delayed single-click (double-click window expired) ──
        if (_waitingForDoubleClick && (now - _lastClickMs) >= _doubleClickWindowMs) {
            _waitingForDoubleClick = false;
            if (_dispatch) {
                InputEvent ev;
                ev.type = InputEventType::EncoderClick;
                ev.ivalue = 1;
                ev.timestamp = now;
                _dispatch(ev);
            }
        }
    }

    const char* name() const override { return "ModulinoKnob"; }

private:
    // Read 4-byte state from the knob: [pinstrap, posLSB, posMSB, pressed]
    bool readState(int16_t &position, bool &pressed) {
        // The Modulino protocol: requestFrom returns [pinstrap, data...]
        // but the first byte is the internal pinstrap address — skip it.
        uint8_t bytesNeeded = 4; // 1 pinstrap + 2 position + 1 pressed
        uint8_t got = _wire.requestFrom(_address, bytesNeeded);
        if (got < bytesNeeded) {
            Serial.printf("[Knob] I2C short read: got %d/%d\n", got, bytesNeeded);
            // flush whatever we got
            while (_wire.available()) _wire.read();
            return false;
        }
        uint8_t pin = _wire.read(); // discard pinstrap byte
        uint8_t lo = _wire.read();
        uint8_t hi = _wire.read();
        uint8_t pr = _wire.read();
        position = (int16_t)(lo | (hi << 8));
        pressed = (pr != 0);
        return true;
    }

    int16_t readPosition() {
        int16_t pos = 0;
        bool pr = false;
        readState(pos, pr);
        return pos;
    }

    void writePosition(int16_t value) {
        _wire.beginTransmission(_address);
        _wire.write((uint8_t)(value & 0xFF));
        _wire.write((uint8_t)((value >> 8) & 0xFF));
        _wire.endTransmission();
    }

    IInputDevice::DispatchFn _dispatch { nullptr };
    TwoWire &_wire;
    uint8_t _address;
    bool _ready { false };

    // Rotation
    int16_t _lastPosition { 0 };
    unsigned long _lastPollMs { 0 };
    int     _pendingDelta { 0 };         // buffered rotation awaiting dispatch
    unsigned long _pendingDeltaMs { 0 }; // when the pending delta was first recorded

    // Button timing
    bool _buttonWasPressed { false };
    bool _longPressTriggered { false };
    bool _waitingForDoubleClick { false };
    unsigned long _buttonPressStartMs { 0 };
    unsigned long _lastButtonEventMs { 0 };
    unsigned long _lastClickMs { 0 };

    static constexpr unsigned long _holdThresholdMs = 500;
    static constexpr unsigned long _doubleClickWindowMs = 400;
    static constexpr unsigned long _minClickMs = 50;
    static constexpr unsigned long _bounceFilterMs = 120;     // post-press rotation filter
    static constexpr unsigned long _prePressGuardMs = 25;     // buffer rotation before dispatch
};
